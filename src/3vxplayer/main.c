/*
  3vxplayer - custom 29.97fps 320x240 stereo video player for stock 3DO.

  Pipeline (single thread, polled):
    CD (async block reads) -> 3VX demux -> [VFRM slots -> 3VX decode into
    DRAM LR backbuffer -> CEL present (double-buffered screens)]
                                        -> [AUD0 ring -> SoundSpooler ->
    dcsqxdhalfstereo.dsp -> directout.dsp]

  Sync: audio-clock master with fractional samples-per-frame (735.735).
  Controls: P pause/resume, X restart from beginning; auto-loop at EOF.
*/
#include "vx_stream.h"
#include "vx_audio.h"
#include "vx_dec.h"
#include "vx_sync.h"

#include "controlpad.h"
#include "debug.h"
#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "hardware.h"
#include "mem.h"
#include "msgport.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"
#include "timerutils.h"

#define VIDEO_PATH "3vxplayer_data/video.3vx"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define ROWPAIRS      (SCREEN_HEIGHT / 2)
#define ROWPAIR_BYTES (SCREEN_WIDTH * 4)
#define BACKBUF_BYTES (ROWPAIRS * ROWPAIR_BYTES)

/* fixed-point samples-per-frame at 29.97: 22050*1001/30000 = 735.735 */
/* due_frames(audpos) = audpos * 30000 / (22050*1001) */

typedef struct Telemetry {
  uint32 tm_service, tm_fill, tm_decode, tm_misc, tm_n;
  uint32 presents;
  uint32 frame_drop_decodes;  /* decoded but not presented */
  uint32 vbl_errs;
  uint32 video_starves;       /* wanted a frame, slot empty */
  uint32 audio_starves;
  uint32 decode_errs;
  uint32 win_underflows;
  uint32 loops;
  uint32 vbl_start;           /* vbl count when playback began */
  uint32 last_present_vbl;
  uint32 present_vbl_gaps_max;
  char   msg[80];
} Telemetry;

static ScreenContext gSC;
static Item          gVbl;
static uint16       *gBackbuf;
static CCB           gPresentCel;

static VxStream      gST;
static VxAudio       gAU;
static VxDec         gDEC;
static Telemetry     gT;
uint32        gVblCount;
static int gPrepared;
static uint32 gPreparedFrame;
static uint32 gSubmittedField;
static int gSubmitted;
static int gDecoded;
static int gVblPending;
static uint32 gDecodedFrame;
static uint32 gPhaseField;
static uint32 gPhaseFrame;
static int gPhaseValid;
static uint32 gSubmittedTick;
#ifdef DEBUG
volatile uint32 dbg_avail, dbg_inbuf, dbg_nvfrm, dbg_naud0, dbg_ringfull,
                dbg_slotfull, dbg_pending, dbg_fillcalls, dbg_pullzero,
                dbg_subfail, dbg_eof, dbg_curfill, dbg_winoff,
                dbg_fillb, dbg_nextoff, dbg_wedged, dbg_fd, dbg_slotr;
#endif

#ifdef DEBUG
static void
telemetry_draw(void)
{
  GrafCon gc;
  char line[96];
  uint32 elapsed_vbls = gVblCount - gT.vbl_start;
  uint32 fps_milli;
  uint32 aud_smp = vx_audio_position_samples(&gAU);

  /* presents per 59.94 vbls/s -> mili-fps via one divide (once per frame) */
  fps_milli = elapsed_vbls ? (gT.presents * 1000 * 60) / (elapsed_vbls ? elapsed_vbls : 1)
                           : 0;
  /* correct unit: presents*1000*60/elapsed_vbls ~ milli-presents-per-second */
  sprintf(line, "v-blank %lu  presents %lu  rate %lu.%03lu  aud_smp %lu",
          (unsigned long)gVblCount, (unsigned long)gT.presents,
          (unsigned long)(fps_milli / 1000), (unsigned long)(fps_milli % 1000),
          (unsigned long)aud_smp);
  SetFGPen(&gc, MakeRGB15(0, 31, 0));
  MoveTo(&gc, 4, 4);
  DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)line);

  sprintf(line, "vs %lu as %lu de %lu lu %lu  fifo a:%lu f:%c%c  %s",
          (unsigned long)gT.video_starves, (unsigned long)gT.audio_starves,
          (unsigned long)gT.decode_errs, (unsigned long)gT.loops,
          (unsigned long)(gST.aud_head - gST.aud_tail),
          gST.frame_ready[0] ? 'F' : '.',
          gST.frame_ready[1] ? 'F' : '.',
          gT.msg);
  MoveTo(&gc, 4, 14);
  DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)line);
}
#endif

static void
setup_present_cel(CCB *ccb, uint16 *backbuf)
{
  memset(ccb, 0, sizeof(*ccb));
  ccb->ccb_Flags = CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_YOXY
                 | CCB_CCBPRE | CCB_ACW | CCB_ACCW | CCB_SPABS | CCB_NPABS
                 | CCB_PPABS | CCB_BGND | CCB_ACE | PMODE_ONE | CCB_LAST;
  ccb->ccb_NextPtr = NULL;
  ccb->ccb_SourcePtr = (void *)backbuf;
  ccb->ccb_PLUTPtr = NULL;
  ccb->ccb_XPos = 0;
  ccb->ccb_YPos = 0;
  ccb->ccb_HDX  = 1L << 20;
  ccb->ccb_HDY  = 0;
  ccb->ccb_VDX  = 0;
  ccb->ccb_VDY  = 1L << 16;
  ccb->ccb_HDDX = 0;
  ccb->ccb_HDDY = 0;
  ccb->ccb_PIXC = 0x1F001F81;
  ccb->ccb_Width  = SCREEN_WIDTH;
  ccb->ccb_Height = ROWPAIRS;
  ccb->ccb_PRE0 = (((ROWPAIRS - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)
                   | PRE0_LINEAR | PRE0_BPP_16);
  ccb->ccb_PRE1 = (((SCREEN_WIDTH - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT)
                   | PRE1_LRFORM
                   | ((SCREEN_WIDTH - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT));
}

/* pull adapter for vx_audio_fill over the stream's audio ring */
static uint32
st_pull(void *ctx, uint8 *dst, uint32 max, uint32 *first_sample)
{
  VxStream *st = (VxStream *)ctx;
  return vx_stream_audio_pull(st, dst, max, first_sample);
}

/* Error-exit teardown only (the success path never returns): release
   what main() acquired. Guards test Item/pointer validity, so this is
   safe from ANY error exit: statics are BSS-zero until initialized,
   failed acquisitions are NULL or negative, and valid Items are >= 16
   (0 is never assignable). Order: audio, then stream (the EZFlix
   DismantlePlayer pair), then backbuf, vbl, display, pad in reverse
   initialization order. */
static void
teardown(void)
{
  if(gVblPending)
    { WaitIO(gVbl); gVblPending = 0; }
  vx_audio_close(&gAU);          /* spooler+staging+instruments+folio;
                                    all no-ops if open failed/never ran */
  vx_stream_close(&gST);         /* idempotent after a failed open */
  if(gBackbuf)
    FreeMem(gBackbuf, BACKBUF_BYTES);
  if(gVbl > 0)
    DeleteVBLIOReq(gVbl);        /* DeleteItem(gVbl) */
  if(gSC.sc_ScreenGroup > 0)     /* CreateBasicDisplay succeeded; 0 is
                                    its memset value on failure (it also
                                    closes the graphics folio itself
                                    there, so no double-close) */
    DeleteBasicDisplay(&gSC);
  KillControlPad();              /* safe after failed InitControlPad:
                                    guards its own partial-init state */
}

/* Documented stop ladder (reference audio_play.c aud_rewind): settle pending
   completion signals, stop, reset; then re-anchor all clocks at zero. */
static void
stop_and_rewind(void)
{
  int32 sigs = (int32)GetCurrentSignals() & gAU.spooler->sspl_SignalMask;
  if(gVblPending)
    { WaitIO(gVbl); gVblPending = 0; }

  if(sigs)
    ssplProcessSignals(gAU.spooler, sigs, NULL);
  if(gAU.started)
    {
      if(ssplStopSpooler(gAU.spooler) < 0 || ssplReset(gAU.spooler, NULL) < 0)
        kprintf("audio rewind failed\n");
      gAU.started = 0;
    }
  else
    ssplReset(gAU.spooler, NULL);
  vx_audio_reset_clock(&gAU);
  vx_stream_rewind(&gST);
}

static void
present_backbuf(void)
{
#ifndef VX_PROBE_NOPRESENT
  int cur = gSC.sc_curScreen;
  uint32 gap;

#ifdef DEBUG
  telemetry_draw();
#endif
  /* The image was copied into this screen before its presentation deadline. */
  DisplayScreen(gSC.sc_Screens[cur], 0);
  gSC.sc_curScreen++;
  if(gSC.sc_curScreen == 3) gSC.sc_curScreen = 0;

  gap = GrafBase->gf_VBLNumber - gT.last_present_vbl;
  if(gT.presents != 0 && gap > gT.present_vbl_gaps_max)
    gT.present_vbl_gaps_max = gap;
  gT.last_present_vbl = GrafBase->gf_VBLNumber;
  gSubmittedField = gT.last_present_vbl;
  gSubmitted = 1;
  gSubmittedTick = gVblCount;
#endif
  gT.presents++;
#ifdef DEBUG
  if((gT.presents % 30) == 0)
    {
      kprintf("presents %lu", (unsigned long)gT.presents);
      kprintf(" vbl %lu", (unsigned long)gVblCount);
      kprintf(" vs %lu", (unsigned long)gT.video_starves);
      kprintf(" as %lu\n", (unsigned long)gT.audio_starves);
      if(gT.tm_n)
        {
          kprintf("  svc %lu", (unsigned long)(gT.tm_service / gT.tm_n));
          kprintf(" fill %lu", (unsigned long)(gT.tm_fill / gT.tm_n));
          kprintf(" dec %lu", (unsigned long)(gT.tm_decode / gT.tm_n));
          kprintf(" misc %lu", (unsigned long)(gT.tm_misc / gT.tm_n));
          kprintf(" n %lu\n", (unsigned long)gT.tm_n);
          gT.tm_service = gT.tm_fill = gT.tm_decode = gT.tm_misc = gT.tm_n = 0;
        }
    }
#endif
}

/* Decoder overload must not monopolize the thread that feeds audio. */
static void
service_video(int *recovering, uint32 *next_frame_index)
{
  uint32 scans = 0, decodes = 0;
  /* DRAM decoding can overlap the pending screen switch; VRAM reuse cannot. */
  if(gDecoded)
    return;
  while(scans++ < VX_SCAN_BUDGET && decodes < VX_DECODE_BUDGET)
    {
      const uint8 *payload;
      uint32 size, frame, due;
      int action, was_recovering = *recovering;
      uint32 decode_error;

      vx_stream_service(&gST);
      vx_audio_fill(&gAU, gST.aud_head - gST.aud_tail, st_pull, &gST);
      if(gST.wedged || gAU.error < 0)
        return;
      due = vx_due_frame(vx_audio_position_samples(&gAU));
      if(!vx_stream_next_frame(&gST, &payload, &size, &frame))
        { gT.video_starves++; return; }
      action = vx_sync_action(frame, due, size >= 4 && (payload[1] & 1), recovering);
      /* One image can wait in VRAM while its successor is decoded in DRAM. */
      if(action == VX_SYNC_WAIT && !*recovering && frame <= due + 2)
        action = VX_SYNC_DECODE;
      /* A sub-word audio tail can freeze the sample clock while a full
         video queue hides the next AUD0. Discard prediction, not audio:
         free one slot so demux can reach that audio. Never advance the
         clock or present future frames to escape this backpressure. */
      if(action == VX_SYNC_WAIT && gAU.played_bytes == gAU.total_submitted
         && gST.aud_head - gST.aud_tail < 4
         && gST.frame_ready[0] && gST.frame_ready[1]
         && gST.frame_ready[2] && gST.frame_ready[3])
        {
          *recovering = 1;
          action = VX_SYNC_DROP;
        }
      if(!was_recovering && *recovering)
        kprintf("catchup: skipping to keyframe from %d\n", (int)frame);
      if(action == VX_SYNC_WAIT)
        return;
      if(action == VX_SYNC_DROP)
        {
          gDEC.state_valid = 0;
          vx_stream_frame_consumed(&gST);
          *next_frame_index = frame + 1;
          continue;
        }
      decodes++;
      decode_error = vx_dec_frame(&gDEC, payload, size, NULL);
      if(decode_error != VXE_OK)
        {
          gT.decode_errs++;
          gDEC.state_valid = 0;
          *recovering = 1;
          kprintf("decode error %d; waiting for keyframe\n", (int)decode_error);
        }
      else
        {
          if(was_recovering)
            kprintf("catchup: resumed at keyframe %d\n", (int)frame);
          due = vx_due_frame(vx_audio_position_samples(&gAU));
          if(due <= frame + 1)
            {
              gDecodedFrame = frame;
              gDecoded = 1;
            }
          else
            gT.frame_drop_decodes++;
        }
      vx_stream_frame_consumed(&gST);
      *next_frame_index = frame + 1;
      if(gDecoded)
        return;
    }
}

static void
stage_decoded(void)
{
  if(!gDecoded || gPrepared)
    return;
#ifndef VX_PROBE_NOPRESENT
  DrawCels(gSC.sc_BitmapItems[gSC.sc_curScreen], &gPresentCel);
#endif
  gDecoded = 0;
  gPreparedFrame = gDecodedFrame;
  gPrepared = 1;
}

static void
present_ready(void)
{
  uint32 due, field;
  if(!gPrepared)
    return;
  due = vx_due_frame(vx_audio_position_samples(&gAU));
  field = gVblCount;
  if(!gPhaseValid && gPreparedFrame <= due)
    {
      gPhaseField = field + 2;
      gPhaseFrame = gPreparedFrame;
      gPhaseValid = 1;
    }
  if(gPhaseValid && (int32)(field - (gPhaseField + 2 * (gPreparedFrame - gPhaseFrame))) < 0)
    return;
  if(gPreparedFrame > due)
    {
      /* A prepared future image must not prevent demux from reaching
         audio hidden behind a full compressed-frame queue. */
      if(gAU.played_bytes == gAU.total_submitted
         && gST.aud_head - gST.aud_tail < 4
         && gST.frame_ready[0] && gST.frame_ready[1]
         && gST.frame_ready[2] && gST.frame_ready[3])
        { gPrepared = 0; gT.frame_drop_decodes++; }
      return;
    }
  if(due <= gPreparedFrame + 2)
    {
      present_backbuf();
    }
  else
    { gT.frame_drop_decodes++; gPhaseValid = 0; }
  gPrepared = 0;
}

int
main(int argc, char **argv)
{
  Err err;
  u32 btns;
  int paused = 0;
  int recovering = 0;
  uint32 next_frame_index = 0;

  (void)argc; (void)argv;
  memset(&gT, 0, sizeof(gT));

  kprintf("3vxplayer\n");

  kprintf("init: pad\n");
#ifdef DEBUG
  kprintf("imgbase main %lx\n", (unsigned long)main);
  kprintf(" gT %lx\n", (unsigned long)&gT);
  kprintf(" gVbl %lx\n", (unsigned long)&gVblCount);
  kprintf(" gST %lx\n", (unsigned long)&gST);
  kprintf(" gAU %lx\n", (unsigned long)&gAU);
#endif
  err = InitControlPad(1);
  if(err < 0) { kprintf("InitControlPad err %lx\n", err); teardown(); return 1; }

  kprintf("init: display\n");
  err = CreateBasicDisplay(&gSC, DI_TYPE_DEFAULT, 3);
  if(err < 0) { kprintf("CreateBasicDisplay err %lx\n", err); teardown(); return 1; }
  gSC.sc_curScreen = 0;

  kprintf("init: vbl\n");
  gVbl = GetVBLIOReq();
#ifdef DEBUG
  kprintf("A avail %lx\n", (unsigned long)&dbg_avail);
  kprintf("A inbuf %lx\n", (unsigned long)&dbg_inbuf);
  kprintf("A nvfrm %lx\n", (unsigned long)&dbg_nvfrm);
  kprintf("A naud0 %lx\n", (unsigned long)&dbg_naud0);
  kprintf("A rf %lx\n", (unsigned long)&dbg_ringfull);
  kprintf("A sf %lx\n", (unsigned long)&dbg_slotfull);
  kprintf("A pd %lx\n", (unsigned long)&dbg_pending);
  kprintf("A vbl %lx\n", (unsigned long)&gVblCount);
  kprintf("A pres %lx\n", (unsigned long)&gT.presents);
  kprintf("A loops %lx\n", (unsigned long)&gT.loops);
  kprintf("A ply %lx\n", (unsigned long)&gAU.played_bytes);
  kprintf("A sub %lx\n", (unsigned long)&gAU.total_submitted);
  kprintf("A st %lx\n", (unsigned long)&gAU.started);
  kprintf("A fillcalls %lx\n", (unsigned long)&dbg_fillcalls);
  kprintf("A ncomp %lx\n", (unsigned long)&gAU.n_complete);
  kprintf("A nstart %lx\n", (unsigned long)&gAU.n_start);
  kprintf("A pullzero %lx\n", (unsigned long)&dbg_pullzero);
  kprintf("A subfail %lx\n", (unsigned long)&dbg_subfail);
  kprintf("A eof %lx\n", (unsigned long)&dbg_eof);
  kprintf("A cur %lx\n", (unsigned long)&dbg_curfill);
  kprintf("A win %lx\n", (unsigned long)&dbg_winoff);
  kprintf("A fill %lx\n", (unsigned long)&dbg_fillb);
  kprintf("A nxt %lx\n", (unsigned long)&dbg_nextoff);
  kprintf("A wdg %lx\n", (unsigned long)&dbg_wedged);
  kprintf("A fd %lx\n", (unsigned long)&dbg_fd);
  kprintf("A sr %lx\n", (unsigned long)&dbg_slotr);
#endif
  if(gVbl < 0) { kprintf("GetVBLIOReq failed\n"); teardown(); return 1; }

  kprintf("init: backbuf\n");
  gBackbuf = (uint16 *)AllocMem(BACKBUF_BYTES, MEMTYPE_DRAM | MEMTYPE_CEL | MEMTYPE_FILL);
  if(!gBackbuf) { kprintf("backbuf alloc failed\n"); teardown(); return 1; }
  setup_present_cel(&gPresentCel, gBackbuf);
  vx_dec_init(&gDEC, gBackbuf, SCREEN_WIDTH, SCREEN_HEIGHT);

  kprintf("init: audio\n");
  err = vx_audio_open(&gAU);
  if(err < 0) { kprintf("audio open err %lx\n", err); teardown(); return 1; }

  kprintf("init: stream %s\n", VIDEO_PATH);
  err = vx_stream_open(&gST, VIDEO_PATH);
  if(err < 0) { kprintf("stream open err %lx\n", err); teardown(); return 1; }

restart:;
  paused = 0;
  /* reset everything (fresh start, X-restart, or auto-loop) */
  vx_dec_init(&gDEC, gBackbuf, SCREEN_WIDTH, SCREEN_HEIGHT);
  next_frame_index = 0;
  recovering = 0;
  gPrepared = 0;
  gDecoded = 0;
  gSubmitted = 0;
  gPhaseValid = 0;

  kprintf("init: hdr wait\n");
  while(!gST.header_ok)
    {
      vx_stream_service(&gST);
      if(gST.eof && !gST.read_pending && !gST.header_ok)
        { kprintf("stream ended before header\n"); teardown(); return 1; }
      WaitVBL(gVbl, 1);
      gVblCount++;
      if(gST.wedged) { kprintf("stream wedge at header\n"); teardown(); return 1; }
    }
  kprintf("hdr ok w %u\n", gST.info.width);
  kprintf("hdr ok h %u\n", gST.info.height);
  kprintf("hdr ok fps %lu", gST.info.fps_num);
  kprintf("/%lu\n", gST.info.fps_den);

  /* PRIME: no decoding yet. With the audio-lead layout the first AUD0
     chunk precedes frame 0, so the ring fills without consuming VFRMs.
     Queue full spooler buffers (send-before-start officially supported),
     then start audio: sample position 0 == frame 0 due time. */
  vx_audio_reset_clock(&gAU); /* BEFORE priming: counters must cover the
                                 queued-from-zero buffers below */
  {
    uint32 target = 3 * VX_AUD_BUFLEN;
    uint32 prev_sum = 0;
    uint32 stall_vbls = 0;

    for(;;)
      {
        uint32 ring, sum;
        vx_stream_service(&gST);
        if(gST.wedged) { kprintf("wedge in prime\n"); teardown(); return 1; }
        vx_audio_fill(&gAU, gST.aud_head - gST.aud_tail, st_pull, &gST);
        ring = gST.aud_head - gST.aud_tail;
        sum = ring + gAU.total_submitted;
        if(sum >= target)
          break;
        /* everything readable is read; if buffering makes no progress
           (slot backpressure or a sub-buffer audio tail), the stream is
           fully primed by definition */
        if(sum == prev_sum
           && ((gST.eof && !gST.read_pending)
               || (gST.frame_ready[0] && gST.frame_ready[1]
                   && gST.frame_ready[2] && gST.frame_ready[3])))
          {
            if(++stall_vbls > 8)
              break;
          }
        else
          {
            prev_sum = sum;
            stall_vbls = 0;
          }
        WaitVBL(gVbl, 1);
        gVblCount++;
      }
    kprintf("primed %lu\n", (unsigned long)(gST.aud_head - gST.aud_tail + gAU.total_submitted));
  }
  if(vx_audio_start(&gAU) < 0)
    { kprintf("audio start failed\n"); teardown(); return 1; }
  gT.vbl_start = gVblCount;
  if(WaitVBLDefer(gVbl, 1) < 0)
    { kprintf("vbl arm failed\n"); teardown(); return 1; }
  gVblPending = 1;

  for(;;)
    {
      uint32 t0, t1, t2, t3, t4;
#ifndef DEBUG
      (void)t0; (void)t1; (void)t2; (void)t3; (void)t4;
#endif

#ifdef DEBUG
      t0 = GetAudioTime();
#endif
        {
          Err we = WaitIO(gVbl);
          gVblPending = 0;
          if(we < 0 && ((++gT.vbl_errs & 127) == 1))
            kprintf("waitvbl err %lx cnt %lu\n", (unsigned long)we,
                    (unsigned long)gT.vbl_errs);
        }
      /* The graphics count corrects field parity and may jump; the timer
         count is the monotonically elapsed field clock used for pacing. */
      {
        uint32 count = 0;
        int32 result = GetVBLTime(gVbl, NULL, &count);
        if(result < 0 && (uint32)result != count)
          { kprintf("vbl read failed\n"); teardown(); return 1; }
        gVblCount = count;
      }
      if(WaitVBLDefer(gVbl, 1) < 0)
        { kprintf("vbl arm failed\n"); teardown(); return 1; }
      gVblPending = 1;
#ifdef DEBUG
      if((gVblCount % 300) == 0)
        {
          kprintf("tick %lu\n", (unsigned long)gVblCount);
          kprintf("  atime %lu\n", (unsigned long)t0);
        }
#endif
      /* Submit a prepared picture before CD/demux work can delay it. */
      if(!paused)
        {
          stage_decoded();
          present_ready();
          stage_decoded();
        }
      vx_stream_service(&gST);
#ifdef DEBUG
      t1 = GetAudioTime();
#endif
      vx_audio_fill(&gAU, gST.aud_head - gST.aud_tail, st_pull, &gST);
#ifdef DEBUG
      t2 = GetAudioTime();
#endif
#ifdef DEBUG
      dbg_avail    = gST.aud_head - gST.aud_tail;
      dbg_inbuf    = gAU.total_submitted - gAU.played_bytes;
      dbg_nvfrm    = gST.n_vfrm;
      dbg_naud0    = gST.n_aud0;
      dbg_ringfull = gST.aud_ringfull;
      dbg_slotfull = gST.slotfull;
      dbg_pending  = (uint32)gST.read_pending;
      dbg_eof      = (uint32)gST.eof;
      dbg_curfill  = gST.cur;
      dbg_winoff   = gST.win_off;
      dbg_fillb    = gST.fill;
      dbg_nextoff  = gST.next_read_off;
      dbg_wedged   = (uint32)gST.wedged;
      dbg_fd       = gST.frames_delivered;
      dbg_slotr    = (uint32)gST.slot_r;
#endif

      if(gAU.started
         && gAU.total_submitted - gAU.played_bytes < VX_AUD_BUFLEN
         && (gST.aud_head - gST.aud_tail) < VX_AUD_BUFLEN)
        gT.audio_starves++;
#ifdef DEBUG
      if((gVblCount & 0x1FF) == 0x1FF)
        {
          uint32 el = gVblCount - gT.vbl_start;
          kprintf("vb %lu\n", (unsigned long)gVblCount);
          kprintf(" pr %lu\n", (unsigned long)gT.presents);
          kprintf(" rate_m %lu\n", el ? (unsigned long)(gT.presents * 60000u / el) : 0ul);
          kprintf(" vs %lu\n", (unsigned long)gT.video_starves);
          kprintf(" as %lu\n", (unsigned long)gT.audio_starves);
          kprintf(" de %lu\n", (unsigned long)gT.decode_errs);
          kprintf(" ring %lu\n", (unsigned long)(gST.aud_head - gST.aud_tail));
          kprintf(" sub %lu\n", (unsigned long)gAU.total_submitted);
          kprintf(" ply %lu\n", (unsigned long)gAU.played_bytes);
          kprintf(" onf %lu\n", (unsigned long)gAU.no_free);
        }
#endif

      /* controls */
      btns = 0;
      DoControlPad(1, &btns, 0);
      if(btns & ControlX)
        {
          stop_and_rewind();
          gT.loops++;
          goto restart;
        }
      if(btns & ControlStart)
        {
          paused = !paused;
          gPhaseValid = 0;
          err = paused ? vx_audio_pause(&gAU) : vx_audio_resume(&gAU);
          if(err < 0)
            { kprintf("audio pause/resume failed\n"); teardown(); return 1; }
        }
      if(paused)
        continue;

      service_video(&recovering, &next_frame_index);
      stage_decoded();
      if(gST.wedged || gAU.error < 0)
        { kprintf("playback IO/audio error\n"); teardown(); return 1; }
#ifdef DEBUG
      t3 = GetAudioTime();
#endif
      /* Wait for both media tracks, including the final audio buffer. */
      if(gST.eof
         && !gPrepared && !gDecoded
         && (!gSubmitted || gVblCount - gSubmittedTick >= 3u)
         && gST.cur == gST.fill && !gST.read_pending
         && !gST.frame_ready[0] && !gST.frame_ready[1]
         && !gST.frame_ready[2] && !gST.frame_ready[3]
         && next_frame_index != 0 && next_frame_index >= gST.info.frame_count
         && gST.aud_head == gST.aud_tail
         && gAU.played_bytes == gAU.total_submitted)
        {
          kprintf("clip done; looping\n");
          kprintf("decoded %d errors %d\n", (int)next_frame_index, (int)gT.decode_errs);
          gT.loops++;
          stop_and_rewind();
          goto restart;
        }
#ifdef DEBUG
      t4 = GetAudioTime();
      gT.tm_service += (uint32)(t1 - t0);
      gT.tm_fill    += (uint32)(t2 - t1);
      gT.tm_misc    += (uint32)(t4 - t3);
      gT.tm_n++;
#endif
    }

  return 0;
}

/*
  3vxplayer - profiled NTSC 15/20/30-rate 320x240 stereo video player.

  Pipeline (single thread, polled):
    CD (async block reads) -> 3VX demux -> [VFRM slots -> 3VX decode into
    DRAM LR backbuffer -> CEL present (triple-buffered screens)]
                                        -> [AUD0 ring -> SoundSpooler ->
    dcsqxdhalfstereo.dsp -> directout.dsp]

  Sync: exact sample-clock master at the selected file's frame rate.
  Controls: P pause/resume, X file menu; EOF summary: A replay, X file menu.
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
#include "directoryfunctions.h"

#define VIDEO_DIRECTORY "$boot/3vxplayer_data"

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

/* Release-build measurements: no drawing or logging in timed regions. */
typedef struct Timing {
  uint32 count, total_ms, remainder_us, maximum_us, maximum_frame, over;
  uint32 bins[21]; /* 5 ms buckets; last bucket is >= 100 ms */
} Timing;
typedef struct SlowFrame {
  uint32 frame, usec;
} SlowFrame;
static Timing gDecodeTime, gDrawTime;
static SlowFrame gSlow[8];
static uint32 gSkipped, gRecoveries, gEmptyPending, gLatePresents;
static uint32 gFrameBudget, gFieldsPerFrame, gTimerOverhead;
static Item gProfileTimer;
typedef struct DropTiming {
  uint32 count, within_field, max_us, streak, max_streak;
} DropTiming;
static DropTiming gDecodeDrops, gPresentDrops;
static uint32 gHiddenDrops, gPhaseResets, gPhaseStarts;
static uint32 gDrawWaitStart, gDrawWaitMax, gClockAgeMax;
/* Submission times, not graphics-latch/scanout times. 8 is the overflow bin. */
static uint32 gSubmitGaps[9], gSubmitLate[9];
static uint32 gLastSubmitField, gFreshField, gLateStreak, gMaxLateStreak;
static uint32 gMaxSubmitGap;
static int gHaveSubmitField;

static void
record_submission(uint32 ideal)
{
  uint32 gap, late = 0;
  if((int32)(gFreshField - ideal) > 0) late = gFreshField - ideal;
  gSubmitLate[late < 8 ? late : 8]++;
  if(late)
    {
      gLateStreak++;
      if(gLateStreak > gMaxLateStreak) gMaxLateStreak = gLateStreak;
    }
  else gLateStreak = 0;
  if(gHaveSubmitField)
    {
      gap = gFreshField - gLastSubmitField;
      gSubmitGaps[gap < 8 ? gap : 8]++;
      if(gap > gMaxSubmitGap) gMaxSubmitGap = gap;
    }
  gLastSubmitField = gFreshField;
  gHaveSubmitField = 1;
}

/* First audio sample at which frame f becomes due (ceil, no u64). */
static uint32
frame_sample(uint32 frame)
{
  uint32 period = gST.info.fps_num / 50u;
  return (frame / period) * 441441u
       + ((frame % period) * 441441u + period - 1u) / period;
}

static void
record_drop(DropTiming *d, uint32 samples, uint32 first_late_frame)
{
  uint32 boundary = frame_sample(first_late_frame);
  uint32 excess = samples > boundary ? samples - boundary : 0;
  uint32 us = (excess / 22050u) * 1000000u
            + ((excess % 22050u) * 20000u) / 441u;
  d->count++;
  if(us <= 16683u) d->within_field++;
  if(us > d->max_us) d->max_us = us;
  d->streak++;
  if(d->streak > d->max_streak) d->max_streak = d->streak;
}
static void teardown(void);

static uint32
profile_clock(void)
{
  uint32 seconds = 0, useconds = 0;
  if(GetUSecTime(gProfileTimer, &seconds, &useconds) < 0)
    { kprintf("profile timer failed\n"); teardown(); exit(1); }
  return seconds * 1000000u + useconds;
}

static void
profile_record(Timing *t, uint32 us, uint32 frame)
{
  uint32 bin = us / 5000u;
  t->count++;
  t->total_ms += us / 1000u;
  t->remainder_us += us % 1000u;
  if(t->remainder_us >= 1000u)
    { t->total_ms++; t->remainder_us -= 1000u; }
  if(us > t->maximum_us)
    { t->maximum_us = us; t->maximum_frame = frame; }
  if(us > gFrameBudget) t->over++;
  if(bin > 20) bin = 20;
  t->bins[bin]++;
}

static uint32
profile_average(const Timing *t)
{
  if(!t->count) return 0;
  return (t->total_ms / t->count) * 1000u
       + ((t->total_ms % t->count) * 1000u + t->remainder_us) / t->count;
}

/* Upper bound of the bucket containing p99; 105000 means >=100 ms. */
static uint32
profile_p99(const Timing *t)
{
  uint32 i, sum = 0, target = t->count - t->count / 100u;
  if(!t->count) return 0;
  for(i = 0; i < 21; i++)
    {
      sum += t->bins[i];
      if(sum >= target) return (i + 1u) * 5000u;
    }
  return 105000u;
}

static void
profile_line(GrafCon *gc, int row, const char *text)
{
  MoveTo(gc, 4, 4L + (int32)row * 12L);
  DrawText8(gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)text);
  kprintf("%s\n", text);
}

static int
profile_summary(void)
{
  GrafCon gc;
  char line[96];
  uint32 i, buttons = 0;
  int row = 0;
  if(gVblPending) { WaitIO(gVbl); gVblPending = 0; }
  /* All media has drained before this screen replaces the final picture. */
  memset(gBackbuf, 0, BACKBUF_BYTES);
  DrawCels(gSC.sc_BitmapItems[gSC.sc_curScreen], &gPresentCel);
  memset(&gc, 0, sizeof(gc));
  SetFGPen(&gc, MakeRGB15(31,31,31));
  sprintf(line, "3VX NTSC profile %lu/1001 fps", (unsigned long)gST.info.fps_num);
  profile_line(&gc, row++, line);
  sprintf(line, "Budget %lu us; clock %lu us", (unsigned long)gFrameBudget, (unsigned long)gTimerOverhead);
  profile_line(&gc, row++, line);
  sprintf(line, "Decoded %lu presented %lu", (unsigned long)gDecodeTime.count, (unsigned long)gT.presents);
  profile_line(&gc, row++, line);
  sprintf(line, "Skip %lu drop %lu err %lu", (unsigned long)gSkipped, (unsigned long)gT.frame_drop_decodes, (unsigned long)gT.decode_errs);
  profile_line(&gc, row++, line);
  sprintf(line, "Recovery %lu phase-late %lu", (unsigned long)gRecoveries, (unsigned long)gLatePresents);
  profile_line(&gc, row++, line);
  sprintf(line, "Empty polls %lu CD busy %lu", (unsigned long)gT.video_starves, (unsigned long)gEmptyPending);
  profile_line(&gc, row++, line);
  sprintf(line, "Decode avg/max us %lu/%lu", (unsigned long)profile_average(&gDecodeTime), (unsigned long)gDecodeTime.maximum_us);
  profile_line(&gc, row++, line);
  sprintf(line, "Dec over %lu p99 %s%lu us", (unsigned long)gDecodeTime.over,
          profile_p99(&gDecodeTime) > 100000u ? ">=" : "<",
          (unsigned long)(profile_p99(&gDecodeTime) > 100000u ? 100000u : profile_p99(&gDecodeTime)));
  profile_line(&gc, row++, line);
  sprintf(line, "Draw avg/max us %lu/%lu", (unsigned long)profile_average(&gDrawTime), (unsigned long)gDrawTime.maximum_us);
  profile_line(&gc, row++, line);
  sprintf(line, "Draw calls %lu", (unsigned long)gDrawTime.count);
  profile_line(&gc, row++, line);
  sprintf(line, "Drop decode %lu present %lu", (unsigned long)gDecodeDrops.count, (unsigned long)gPresentDrops.count);
  profile_line(&gc, row++, line);
  sprintf(line, "Within 1 field D %lu P %lu", (unsigned long)gDecodeDrops.within_field, (unsigned long)gPresentDrops.within_field);
  profile_line(&gc, row++, line);
  sprintf(line, "Late 0/1/2 %lu/%lu/%lu", (unsigned long)gSubmitLate[0], (unsigned long)gSubmitLate[1], (unsigned long)gSubmitLate[2]);
  profile_line(&gc, row++, line);
  sprintf(line, "Late 3+ %lu streak %lu", (unsigned long)(gT.presents - gSubmitLate[0] - gSubmitLate[1] - gSubmitLate[2]), (unsigned long)gMaxLateStreak);
  profile_line(&gc, row++, line);
  sprintf(line, "Gaps 0/1/2 %lu/%lu/%lu", (unsigned long)gSubmitGaps[0], (unsigned long)gSubmitGaps[1], (unsigned long)gSubmitGaps[2]);
  profile_line(&gc, row++, line);
  sprintf(line, "Gaps 3/4 %lu/%lu max %lu", (unsigned long)gSubmitGaps[3], (unsigned long)gSubmitGaps[4], (unsigned long)gMaxSubmitGap);
  profile_line(&gc, row++, line);
  sprintf(line, "Phase start/reset %lu/%lu", (unsigned long)gPhaseStarts, (unsigned long)gPhaseResets);
  profile_line(&gc, row++, line);
  profile_line(&gc, row, "A: replay   X: menu");
  DisplayScreen(gSC.sc_Screens[gSC.sc_curScreen], 0);
  /* Histogram is printed only after playback, never in a timed region. */
  for(i = 0; i < 21; i++)
    kprintf("decode bucket %d count %d\n", (int)(i * 5000u), (int)gDecodeTime.bins[i]);
  for(i = 0; i < 8 && i < gDecodeTime.count; i++)
    kprintf("slow frame %d us %d\n", (int)gSlow[i].frame, (int)gSlow[i].usec);
  for(i = 0; i < 9; i++)
    kprintf("submit fields %d gaps %d late %d\n", (int)i, (int)gSubmitGaps[i], (int)gSubmitLate[i]);
  kprintf("drop max us D %d P %d hidden %d\n", (int)gDecodeDrops.max_us, (int)gPresentDrops.max_us, (int)gHiddenDrops);
  kprintf("drop streak D %d P %d\n", (int)gDecodeDrops.max_streak, (int)gPresentDrops.max_streak);
  kprintf("stage wait us %d clock age fields %d\n", (int)gDrawWaitMax, (int)gClockAgeMax);
  do { WaitVBL(gVbl, 1); DoControlPad(1, &buttons, 0); }
  while(!(buttons & (ControlA | ControlX)));
  return (buttons & ControlX) != 0;
}

typedef struct VideoFile {
  struct VideoFile *next;
  char name[FILESYSTEM_MAX_NAME_LEN];
} VideoFile;

static VideoFile *gVideos;
static uint32 gVideoCount;

static int
scan_videos(void)
{
  Directory *dir = OpenDirectoryPath(VIDEO_DIRECTORY);
  DirectoryEntry entry;
  if(!dir) return 0;
  while(ReadDirectory(dir, &entry) >= 0)
    {
      VideoFile *file, **link;
      uint32 n;
      entry.de_FileName[FILESYSTEM_MAX_NAME_LEN - 1] = 0;
      n = strlen(entry.de_FileName);
      if((entry.de_Flags & FILE_IS_DIRECTORY) || n < 4
         || entry.de_FileName[n-4] != '.'
         || (entry.de_FileName[n-3] != '3')
         || (entry.de_FileName[n-2] != 'v' && entry.de_FileName[n-2] != 'V')
         || (entry.de_FileName[n-1] != 'x' && entry.de_FileName[n-1] != 'X')) continue;
      file = (VideoFile *)malloc(sizeof(*file));
      if(!file) { CloseDirectory(dir); return 0; }
      strcpy(file->name, entry.de_FileName);
      link = &gVideos;
      while(*link && strcmp((*link)->name, file->name) < 0) link = &(*link)->next;
      file->next = *link; *link = file;
      gVideoCount++;
    }
  CloseDirectory(dir);
  return 1;
}

static void
select_video(char *path)
{
  static uint32 selected;
  uint32 buttons, top, index;
  int redraw = 1;
  VideoFile *file;
  GrafCon gc;
  memset(gBackbuf, 0, BACKBUF_BYTES);
  memset(&gc, 0, sizeof(gc));
  SetFGPen(&gc, MakeRGB15(31,31,31));
  for(;;)
    {
      if(redraw)
      {
      /* Keep the completed menu visible until selection changes. */
      DrawCels(gSC.sc_BitmapItems[gSC.sc_curScreen], &gPresentCel);
      MoveTo(&gc, 8, 12); DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)"3VX VIDEO FILES");
      MoveTo(&gc, 8, 28); DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)"UP/DOWN: select  A/START: play");
      top = selected >= 10 ? selected - 9 : 0;
      file = gVideos;
      for(index = 0; file; file = file->next, index++)
        if(index >= top && index < top + 10)
          {
            MoveTo(&gc, 8, 52L + (int32)(index - top) * 16L);
            DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)(index == selected ? "* " : "  "));
            DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)file->name);
          }
      if(!gVideoCount)
        { MoveTo(&gc, 8, 52); DrawText8(&gc, gSC.sc_BitmapItems[gSC.sc_curScreen], (const uint8 *)"No .3vx files in data directory"); }
      DisplayScreen(gSC.sc_Screens[gSC.sc_curScreen], 0);
      gSC.sc_curScreen = (gSC.sc_curScreen + 1) % 3;
      redraw = 0;
      }
      WaitVBL(gVbl, 1);
      buttons = 0; DoControlPad(1, &buttons, 0);
      if(!gVideoCount) continue;
      if(buttons & ControlUp) { selected = selected ? selected - 1 : gVideoCount - 1; redraw = 1; }
      if(buttons & ControlDown) { selected = (selected + 1) % gVideoCount; redraw = 1; }
      if(buttons & (ControlA | ControlStart))
        {
          file = gVideos;
          for(index = 0; index < selected; index++) file = file->next;
          sprintf(path, "%s/%s", VIDEO_DIRECTORY, file->name);
          do { WaitVBL(gVbl, 1); DoControlPad(1, &buttons, ControlA | ControlStart); }
          while(buttons & (ControlA | ControlStart));
          return;
        }
    }
}
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
  if(gProfileTimer > 0) DeleteItem(gProfileTimer);
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
      uint32 size, frame, due, start, elapsed, rank, samples;
      int action, was_recovering = *recovering;
      uint32 decode_error;

      vx_stream_service(&gST);
      vx_audio_fill(&gAU, gST.aud_head - gST.aud_tail, st_pull, &gST);
      if(gST.wedged || gAU.error < 0)
        return;
      due = vx_due_frame(vx_audio_position_samples(&gAU), gST.info.fps_num);
      if(!vx_stream_next_frame(&gST, &payload, &size, &frame))
        { gT.video_starves++; if(gST.read_pending) gEmptyPending++; return; }
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
        gRecoveries++;
      if(action == VX_SYNC_WAIT)
        return;
      if(action == VX_SYNC_DROP)
        {
          gSkipped++;
          gDEC.state_valid = 0;
          vx_stream_frame_consumed(&gST);
          *next_frame_index = frame + 1;
          continue;
        }
      decodes++;
      start = profile_clock();
      decode_error = vx_dec_frame(&gDEC, payload, size, NULL);
      elapsed = profile_clock() - start;
      profile_record(&gDecodeTime, elapsed, frame);
      for(rank = 0; rank < 8; rank++)
        if(elapsed > gSlow[rank].usec)
          {
            uint32 j;
            for(j = 7; j > rank; j--)
              { gSlow[j].frame = gSlow[j-1].frame; gSlow[j].usec = gSlow[j-1].usec; }
            gSlow[rank].frame = frame; gSlow[rank].usec = elapsed;
            break;
          }
      if(decode_error != VXE_OK)
        {
          gT.decode_errs++;
          gDEC.state_valid = 0;
          if(!*recovering) gRecoveries++;
          *recovering = 1;
        }
      else
        {
          samples = vx_audio_position_samples(&gAU);
          due = vx_due_frame(samples, gST.info.fps_num);
          if(due <= frame + 1)
            {
              gDecodeDrops.streak = 0;
              gDrawWaitStart = profile_clock();
              gDecodedFrame = frame;
              gDecoded = 1;
            }
          else
            {
              gT.frame_drop_decodes++;
              record_drop(&gDecodeDrops, samples, frame + 2);
            }
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
  uint32 start;
  if(!gDecoded || gPrepared)
    return;
#ifndef VX_PROBE_NOPRESENT
  start = profile_clock();
  if(start - gDrawWaitStart > gDrawWaitMax) gDrawWaitMax = start - gDrawWaitStart;
  DrawCels(gSC.sc_BitmapItems[gSC.sc_curScreen], &gPresentCel);
  profile_record(&gDrawTime, profile_clock() - start, gDecodedFrame);
#endif
  gDecoded = 0;
  gPreparedFrame = gDecodedFrame;
  gPrepared = 1;
}

static void
present_ready(void)
{
  uint32 due, field, samples;
  if(!gPrepared)
    return;
  samples = vx_audio_position_samples(&gAU);
  due = vx_due_frame(samples, gST.info.fps_num);
  field = gVblCount;
  {
    uint32 fresh = 0;
    if(GetVBLTime(gProfileTimer, NULL, &fresh) < 0)
      { kprintf("profile field read failed\n"); teardown(); exit(1); }
    if(fresh - field > gClockAgeMax) gClockAgeMax = fresh - field;
    gFreshField = fresh;
  }
  if(!gPhaseValid && gPreparedFrame <= due)
    {
      gPhaseStarts++;
      /* A frame already behind audio must not gain a fresh two-field
         delay: that can repeatedly hold it past the discard deadline. */
      gPhaseField = field + 2 - gFieldsPerFrame * (due - gPreparedFrame);
      gPhaseFrame = gPreparedFrame;
      gPhaseValid = 1;
    }
  if(gPhaseValid && (int32)(field - (gPhaseField + gFieldsPerFrame * (gPreparedFrame - gPhaseFrame))) < 0)
    return;
  if(gPreparedFrame > due)
    {
      /* A prepared future image must not prevent demux from reaching
         audio hidden behind a full compressed-frame queue. */
      if(gAU.played_bytes == gAU.total_submitted
         && gST.aud_head - gST.aud_tail < 4
         && gST.frame_ready[0] && gST.frame_ready[1]
         && gST.frame_ready[2] && gST.frame_ready[3])
        { gPrepared = 0; gT.frame_drop_decodes++; gHiddenDrops++; }
      return;
    }
  if(due <= gPreparedFrame + 2)
    {
      gPresentDrops.streak = 0;
      record_submission(gPhaseField + gFieldsPerFrame * (gPreparedFrame - gPhaseFrame));
      if(gPhaseValid && (int32)(field - (gPhaseField + gFieldsPerFrame * (gPreparedFrame - gPhaseFrame))) > 0)
        gLatePresents++;
      present_backbuf();
    }
  else
    {
      gT.frame_drop_decodes++;
      record_drop(&gPresentDrops, samples, gPreparedFrame + 3);
      gPhaseResets++;
      gPhaseValid = 0;
    }
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
  char video_path[sizeof(VIDEO_DIRECTORY) + FILESYSTEM_MAX_NAME_LEN + 1];

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
  err = CreateBasicDisplay(&gSC, DI_TYPE_NTSC, 3);
  if(err < 0) { kprintf("CreateBasicDisplay err %lx\n", err); teardown(); return 1; }
  gSC.sc_curScreen = 0;

  kprintf("init: vbl\n");
  gVbl = GetVBLIOReq();
  gProfileTimer = GetTimerIOReq();
  if(gProfileTimer < 0) { kprintf("profile timer open failed\n"); teardown(); return 1; }
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

  if(!scan_videos()) { kprintf("video directory scan failed\n"); teardown(); return 1; }
select_movie:;
  select_video(video_path);
  kprintf("init: stream %s\n", video_path);
  err = vx_stream_open(&gST, video_path);
  if(err < 0) { kprintf("stream open err %lx\n", err); teardown(); return 1; }

restart:;
  paused = 0;
  memset(&gT, 0, sizeof(gT));
  memset(&gDecodeTime, 0, sizeof(gDecodeTime));
  memset(&gDrawTime, 0, sizeof(gDrawTime));
  memset(gSlow, 0, sizeof(gSlow));
  memset(&gDecodeDrops, 0, sizeof(gDecodeDrops));
  memset(&gPresentDrops, 0, sizeof(gPresentDrops));
  gHiddenDrops = gPhaseResets = gPhaseStarts = 0;
  gDrawWaitStart = gDrawWaitMax = gClockAgeMax = 0;
  memset(gSubmitGaps, 0, sizeof(gSubmitGaps));
  memset(gSubmitLate, 0, sizeof(gSubmitLate));
  gLastSubmitField = gFreshField = gLateStreak = gMaxLateStreak = gMaxSubmitGap = 0;
  gHaveSubmitField = 0;
  gSkipped = gRecoveries = gEmptyPending = gLatePresents = 0;
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
  gFieldsPerFrame = 60000u / gST.info.fps_num;
  gFrameBudget = 1001000000u / gST.info.fps_num;
  {
    uint32 start = profile_clock();
    gTimerOverhead = profile_clock() - start;
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
          vx_stream_close(&gST);
          goto select_movie;
        }
      if(btns & ControlStart)
        {
          paused = !paused;
          gHaveSubmitField = 0; /* Exclude intentional pauses from gap statistics. */
          gLateStreak = 0;
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
         && (!gSubmitted || gVblCount - gSubmittedTick >= gFieldsPerFrame + 1u)
         && gST.cur == gST.fill && !gST.read_pending
         && !gST.frame_ready[0] && !gST.frame_ready[1]
         && !gST.frame_ready[2] && !gST.frame_ready[3]
         && next_frame_index != 0 && next_frame_index >= gST.info.frame_count
         && gST.aud_head == gST.aud_tail
         && gAU.played_bytes == gAU.total_submitted)
        {
          if(profile_summary())
            {
              stop_and_rewind();
              vx_stream_close(&gST);
              goto select_movie;
            }
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

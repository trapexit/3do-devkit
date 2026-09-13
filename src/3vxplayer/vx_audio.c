/*
  3vx_audio.c - see header. Portfolio audio folio + sound spooler.
*/
#include "vx_audio.h"
#include "list.h"
#include "audio.h"
#include "kernel.h"
#include "debug.h"
#include "mem.h"
#include "string.h"
#include "stdio.h"

/* one silence staging pattern buffer (SDX2 silence = 0x00 bytes decode to
   quiet step-follow; DC offset is avoided by encoding 0x00: verify on real
   hw; for now DX_SILENCE byte 0x00) */
#define SDX2_SILENCE 0x00

static int32
buffer_cb(SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg)
{
  VxAudio *au = (VxAudio *)ssplGetUserData(sspl, sbn);

  if(au == NULL)
    return 0;

  switch(msg)
    {
    case SSPL_SBMSG_COMPLETE:
      au->n_complete++;
      if(au->played_bytes < au->total_submitted)
        au->played_bytes += (uint32)sbn->sbn_NumBytes;
      break;
    case SSPL_SBMSG_INITIAL_START:
    case SSPL_SBMSG_LINK_START:
    case SSPL_SBMSG_STARVATION_START:
      au->n_start++;
      break;
    default:
      break;
    }
  return 0;
}

Err
vx_audio_open(VxAudio *au)
{
  Err err;

  memset(au, 0, sizeof(*au));

  err = OpenAudioFolio();
  kprintf("audio: folio %ld\n", (long)err);
  if(err < 0)
    return err;
  au->folio_open = 1;

  au->out_ins = LoadInstrument("directout.dsp", 0, 100);
  kprintf("audio: directout %ld\n", (long)au->out_ins);
  if(au->out_ins < 0)
    return au->out_ins;
  au->ins = LoadInstrument("dcsqxdhalfstereo.dsp", 0, 100);
  kprintf("audio: dcsqxd %ld\n", (long)au->ins);
  if(au->ins < 0)
    return au->ins;

  err = ConnectInstruments(au->ins, "LeftOutput",  au->out_ins, "InputLeft");
  kprintf("audio: connect L %ld\n", (long)err);
  if(err < 0)
    return err;
  err = ConnectInstruments(au->ins, "RightOutput", au->out_ins, "InputRight");
  kprintf("audio: connect R %ld\n", (long)err);
  if(err < 0)
    return err;

  au->spooler = ssplCreateSoundSpooler(VX_AUD_BUFS, au->ins);
  kprintf("audio: spooler %p\n", au->spooler);
  if(au->spooler == NULL)
    return -1;

  au->staging = (uint8(*)[VX_AUD_BUFLEN])
    AllocMem(VX_AUD_BUFS * VX_AUD_BUFLEN, MEMTYPE_DRAM | MEMTYPE_AUDIO);
  kprintf("audio: staging %p\n", au->staging);
  if(au->staging == NULL)
    return -1;

  /* start the output side; spooler start owns the sampler */
  err = StartInstrument(au->out_ins, NULL);
  kprintf("audio: out start %ld\n", (long)err);
  if(err < 0)
    return err;

  return 0;
}

static int32
cb_trampoline(SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg)
{
  return buffer_cb(sspl, sbn, msg); /* au arrives via per-node UserData */
}

/* Hand an ALREADY-REQUESTED node an ALREADY-FILLED staging row. The
   caller filled au->staging[au->submit_row] AFTER obtaining sbn with
   ssplRequestBuffer() — a granted node guarantees the row handed to the
   spooler VX_AUD_BUFS submits ago has completed, so writing the row is
   safe. On failure the row is discarded (submit_row not advanced),
   exactly like the old tmp-discard: the same row is retried by the next
   fill. */
static SoundBufferNode *
submit_staged(VxAudio *au, SoundBufferNode *sbn, uint32 bytes)
{
  Err err;

  /* Staging rows advance monotonically per submit. Deriving the row from
     total_submitted/VX_AUD_BUFLEN aliases two rows whenever a partial
     (sub-buffer) submit leaves the byte count off a slot boundary — the
     normal case with coarse AUD0 chunks — so the second submit overwrites
     a row the spooler may still be streaming from. */
  err = ssplSetBufferAddressLength(au->spooler, sbn,
                                   (char *)au->staging[au->submit_row],
                                   (int32)bytes);
  if(err < 0)
    { kprintf("spool address error %d\n", (int)err); ssplUnrequestBuffer(au->spooler, sbn); return NULL; }
  ssplSetUserData(au->spooler, sbn, au);
  err = ssplSendBuffer(au->spooler, sbn);
  if(err < 0)
    { kprintf("spool send error %d\n", (int)err); ssplUnrequestBuffer(au->spooler, sbn); return NULL; }

  au->submit_row++; if(au->submit_row == VX_AUD_BUFS) au->submit_row = 0;
  au->total_submitted += bytes;
  return sbn;
}

/* Reference idiom (stanton0arch/3do_video_player src/audio_play.c:239):
   peek at already-pending task signals belonging to the spooler and hand
   ONLY those to ssplProcessSignals — strictly non-blocking. */
void
vx_audio_service(VxAudio *au)
{
  int32 sigs;

  if(au->spooler == NULL)
    return;
  sigs = (int32)(GetCurrentSignals()) & au->spooler->sspl_SignalMask;
  if(sigs && ssplProcessSignals(au->spooler, sigs, NULL) < 0)
    au->error = -1;
}

int
vx_audio_fill(VxAudio *au, uint32 ring_bytes_available,
              uint32 (*pull)(void *ctx, uint8 *dst, uint32 max,
                             uint32 *first_sample),
              void *pull_ctx)
{
  int submitted = 0;

#ifdef DEBUG
  extern volatile uint32 dbg_fillcalls, dbg_pullzero, dbg_subfail;
  dbg_fillcalls++;
#endif
  vx_audio_service(au);

  /* Chunk-granular partial submits are REQUIRED for liveness: the demux
     emits audio in coarse (~10 KiB) AUD0 chunks, so the ring may sit
     just below a full buffer while video slots are waiting on the audio
     clock — refusing to submit the partial tail deadlocks A/V. */
  while(ring_bytes_available > 0)
    {
      uint32 fs;
      uint32 want = VX_AUD_BUFLEN;
      SoundBufferNode *sbn;
      uint32 got;

      /* Request BEFORE pulling: a granted node guarantees the row to be
         filled (staging[submit_row]) is no longer streaming — the row
         handed over VX_AUD_BUFS submits ago has completed. A full
         spooler fails here and no row is ever written. */
      sbn = ssplRequestBuffer(au->spooler);
      if(sbn == NULL)
        {
          au->no_free++;
#ifdef DEBUG
          dbg_subfail++;
#endif
          break;
        }

      /* Pull DIRECTLY into the staging row: no stack tmp, no second
         copy. A zero pull discards the requested node; the partially
         filled row is dead (submit_row not advanced) and is simply
         overwritten by the next fill. */
      got = pull(pull_ctx, au->staging[au->submit_row], want, &fs);
      if(got == 0)
        {
          ssplUnrequestBuffer(au->spooler, sbn);
#ifdef DEBUG
          dbg_pullzero++;
#endif
          break;
        }
      if(submit_staged(au, sbn, got) == NULL)
        {
          au->error = -1;
          au->no_free++;
#ifdef DEBUG
          dbg_subfail++;
#endif
          break;
        }
      submitted++;
      if(got >= ring_bytes_available)
        break;
      ring_bytes_available -= got;
    }
  return submitted;
}

Err
vx_audio_start(VxAudio *au)
{
  Err err;
  if(au->started)
    return 0;
  err = ssplSetSoundBufferFunc(au->spooler, cb_trampoline);
  if(err >= 0)
    err = ssplStartSpooler(au->spooler, 0x7FFF);
  if(err < 0)
    return au->error = err;
  au->started = 1;
  au->paused = 0;
  kprintf("audio started\n");
  return 0;
}

Err
vx_audio_pause(VxAudio *au)
{
  Err err;
  if(!au->started || au->paused)
    return 0;
  err = ssplPause(au->spooler);
  if(err < 0)
    return au->error = err;
  /* Read the stopped DMA cursor before freezing its cached value. */
  vx_audio_position_samples(au);
  au->paused = 1;
  return 0;
}

Err
vx_audio_resume(VxAudio *au)
{
  Err err;
  if(!au->started || !au->paused)
    return 0;
  err = ssplResume(au->spooler);
  if(err < 0)
    return au->error = err;
  au->paused = 0;
  return 0;
}

uint32
vx_audio_position_samples(VxAudio *au)
{
  SoundBufferNode *sbn;
  uint32 base, candidate;

  if(!au->started || au->paused)
    return au->position_samples;
  vx_audio_service(au);
  base = au->played_bytes;
  candidate = base >> 1;
  /* WhereAttachment returns a DMA byte offset, not sample frames.
     Completed-but-unserviced nodes can still be in this list. Only the
     disjoint staging range containing the hardware cursor is valid.
     DMA prefetch introduces bounded FIFO lead, never accumulating drift.
     No nominal 240 Hz tick or display-loop count enters this clock. */
  SCANLIST(&au->spooler->sspl_ActiveBuffers, sbn, SoundBufferNode)
    {
      int32 offset = WhereAttachment(sbn->sbn_Attachment);
      if(offset >= 0 && (uint32)offset < (uint32)sbn->sbn_NumBytes)
        {
          candidate = (base + (uint32)offset) >> 1;
          break;
        }
      base += (uint32)sbn->sbn_NumBytes;
    }
  /* At a DMA transition a read can miss both ranges. Hold rather than
     inventing elapsed media time; completion processing catches up. */
  if(candidate > au->position_samples)
    au->position_samples = candidate;
  return au->position_samples;
}

void
vx_audio_reset_clock(VxAudio *au)
{
  au->played_bytes = 0;
  au->total_submitted = 0;
  au->submit_row = 0;
  au->position_samples = 0;
  au->error = 0;
}

void
vx_audio_close(VxAudio *au)
{
  if(!au)
    return;
  if(au->spooler)
    {
      ssplAbort(au->spooler, NULL);
      ssplDeleteSoundSpooler(au->spooler);
      au->spooler = NULL;
    }
  if(au->staging)
    {
      FreeMem(au->staging, VX_AUD_BUFS * VX_AUD_BUFLEN);
      au->staging = NULL;
    }
  /* out_ins is the one instrument vx_audio_open starts directly
     (StartInstrument at open); stop it before unloading. ins (dcsqxd)
     is started by ssplStartSpooler and stopped by ssplAbort above.
     Instruments unload in reverse open order (dcsqxd, then directout);
     failed loads store negative error codes, so guard > 0. Freed
     fields are nulled so a second close is a full no-op. */
  if(au->out_ins > 0)
    StopInstrument(au->out_ins, NULL);
  if(au->ins > 0)
    UnloadInstrument(au->ins);
  if(au->out_ins > 0)
    UnloadInstrument(au->out_ins);
  au->ins = 0;
  au->out_ins = 0;
  if(au->folio_open)
    {
      CloseAudioFolio();
      au->folio_open = 0;
    }
}

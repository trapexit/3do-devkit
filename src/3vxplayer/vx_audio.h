/*
  3vx_audio.h - SDX2 stereo audio through the DSP, zero decode on the ARM.

  Chain: SoundSpooler -> dcsqxdhalfstereo.dsp (fixed 22050 Hz out) ->
  directout.dsp. The spooler's completion callback counts played bytes;
  played samples (per channel) = played_bytes>>1 at 22050 Hz.

  Clock: completed bytes plus the active attachment's DMA byte position.
  No conversion through the nominal 240 Hz audio clock or VBL counters.
*/
#ifndef VX_AUDIO_H
#define VX_AUDIO_H

#include "types.h"
#include "soundspooler.h"

#define VX_AUD_BUFS      6
#define VX_AUD_BUFLEN    2944      /* bytes; 1472 samples = 66.7 ms */
#define VX_AUD_RATE      22050
#define VX_AUD_SPF_NUM   (22050ul * 1001ul)   /* samples per frame = num/den */
#define VX_AUD_SPF_DEN   30000ul

typedef struct VxAudio {
  Item           ins;        /* dcsqxdhalfstereo instrument */
  Item           out_ins;    /* directout */
  SoundSpooler  *spooler;
  uint8        (*staging)[VX_AUD_BUFLEN];
  int           folio_open;  /* OpenAudioFolio succeeded (close must
                                CloseAudioFolio; not derivable from the
                                other fields when the first instrument
                                load fails) */

  uint32  played_bytes;
  uint32  no_free;               /* ssplRequestBuffer failures */      /* from completion callbacks */
  uint32  started;           /* spooler started (playing or primed) */
  uint32  total_submitted;   /* bytes handed to spooler */
  uint32  submit_row;     /* next staging row to hand to the spooler */
  uint32  underruns;
  uint32  n_complete;
  uint32  n_start;
  uint32  position_samples; /* monotonic DMA-derived sample cursor */
  Err     error;            /* sticky audio failure; caller stops playback */

  int     paused;
} VxAudio;

Err  vx_audio_open(VxAudio *au);
void vx_audio_close(VxAudio *au);
void vx_audio_service(VxAudio *au);

/* Pull from the stream ring into free spooler buffers. Submits whatever
   the ring holds (full or partial buffers) — partial submits are required
   for liveness given coarse AUD0 chunk granularity. */
int  vx_audio_fill(VxAudio *au, uint32 ring_bytes_available,
                   uint32 (*pull)(void *ctx, uint8 *dst, uint32 max,
                                  uint32 *first_sample),
                   void *pull_ctx);

/* Current clock in per-channel samples. Frozen while paused/starved. */
uint32 vx_audio_position_samples(VxAudio *au);

Err vx_audio_start(VxAudio *au);
Err vx_audio_pause(VxAudio *au);
Err vx_audio_resume(VxAudio *au);
void vx_audio_reset_clock(VxAudio *au);   /* loop wrap / seek */

#endif /* VX_AUDIO_H */

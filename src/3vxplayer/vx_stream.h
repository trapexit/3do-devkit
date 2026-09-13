/*
  3vx_stream.h - 3VX container streamer: async CD block reads + demux.

  One circular 384 KiB window over the file; a single outstanding async
  block read refills it; the parser walks complete chunks in order and:
    - copies each VFRM payload into one of 4 frame slots (~8-15 KiB/frame,
      bounded copy; preferred over a pin/straddle state machine),
    - appends AUD0 payloads into a 96 KiB ring the audio module pulls,
    - wraps reads and payload copies without moving unread data.
  Everything polled: vx_stream_service() once per frame slot.
  C89: no uint64. Header validation limits clips to 450000 frames (>4 h).
*/
#ifndef VX_STREAM_H
#define VX_STREAM_H

#include "types.h"
#include "blockfile.h"

#define VX_WIN_BYTES     (384 * 1024)
#define VX_READ_BYTES    (128 * 1024)
#define VX_FRAME_MAX     (56 * 1024)   /* hard cap per VFRM payload */
#define VX_FRAME_SLOTS   4
#define VX_AUDRING_BYTES (96 * 1024)

typedef struct VxStreamInfo {
  uint16 width, height;
  uint32 fps_num, fps_den;
  uint32 frame_count;
  uint32 audio_rate;
  uint16 audio_channels;
  uint16 audio_format;        /* 'S2' = 0x5332 */
  uint32 audio_bytes_per_sec;
  uint32 keyframe_interval;
} VxStreamInfo;

typedef struct VxStream {
  BlockFile bf;
  Item      ioreq;
  Item      port;
  int       bf_open;       /* BlockFile opened (close must unwind it) */
  int32     file_size;

  uint8    *win;             /* window base */
  uint32    win_off;         /* reserved; zero (no sliding-window relocation) */
  uint32    cur;             /* absolute parse offset */
  uint32    fill;            /* absolute valid-data end (>= cur) */
  uint32    next_read_off;   /* absolute offset of next read */
  uint32    pend_bytes;      /* latched size of in-flight read */
  int       read_pending;
  int       eof;             /* next_read_off reached file_size */

  uint8    *frames[VX_FRAME_SLOTS];
  uint32    frame_len[VX_FRAME_SLOTS];
  uint32    frame_index[VX_FRAME_SLOTS];
  int       frame_ready[VX_FRAME_SLOTS];
  int       slot_w, slot_r;

  uint8    *aud;             /* audio ring */
  uint32    aud_head;        /* total bytes ever written */
  uint32    aud_tail;        /* total bytes ever read */
  uint32    aud_tail_sample; /* per-channel sample index at aud_tail */

  int       header_ok;
  int       wedged;
  uint32    frames_delivered;
  uint32    audio_payload_bytes;
  uint32    n_vfrm, n_aud0, aud_ringfull, slotfull;
  int       eof_said;

  VxStreamInfo info;
  uint32 skip_remaining;     /* unknown chunk streamed without staging it whole */
} VxStream;

Err  vx_stream_open(VxStream *st, const char *path);
void vx_stream_close(VxStream *st);
void vx_stream_service(VxStream *st);

/* Peek next available video frame (slot pointer valid until consumed). */
int  vx_stream_next_frame(VxStream *st, const uint8 **payload,
                          uint32 *size, uint32 *frame_index);
void vx_stream_frame_consumed(VxStream *st);

/* Returns whole-word submissions, preserving partial samples in the ring.
   At EOF only, the final stereo sample is zero-padded to four bytes. */
uint32 vx_stream_audio_pull(VxStream *st, uint8 *dst, uint32 max,
                            uint32 *first_sample);
void vx_stream_rewind(VxStream *st);

#endif /* VX_STREAM_H */

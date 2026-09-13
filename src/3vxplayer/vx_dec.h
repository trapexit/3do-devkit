/*
  3vx_dec.h - 3VX frame decoder core (pure C89, no 3DO dependencies).

  Decodes 3VX VFRM payloads (codebook sub-chunks + VEC run commands)
  into a persistent LR-layout 16bpp backbuffer. The backbuffer is only
  ever mutated for blocks coded in the current frame; skipped blocks
  keep the previous frame's pixels (single-buffered delta simulation).

  LR layout: backbuffer holds ROWPAIRS = height/2 row-pair bands.
  Word (x, rp) sits at byte offset rp*width*4 + x*4 and holds pixel
  (x, 2*rp) in the first u16 (address +0/+1, big-endian value written
  as a u16) and pixel (x, 2*rp+1) in the second u16 (+2/+3).
  The buffer is stored big-endian-word-native: on the ARM60 (big-endian)
  a u32 store of (even<<16)|odd reproduces it exactly; helpers below
  abstract that for host testing.
*/
#ifndef VX_DEC_H
#define VX_DEC_H

#include "types.h" /* 3DO types; on host build we supply a shim */

/* Geometry: 4x4 blocks; width/height must be multiples of 4. */
typedef struct VxDec {
  uint16 *backbuf;      /* caller-owned, width*height u16, LR-ordered */
  uint16  width;        /* pixels */
  uint16  height;       /* pixels */
  uint16  blocks_w;     /* width/4 */
  uint16  blocks_h;     /* height/4 */
  int     state_valid;  /* 0 until first keyframe decode succeeded */
  /* Resident expanded codebooks, pre-shattered to LR words:
     V1: 4 u32 per entry {tl,tr,bl,br} each color|color<<16
     (block rowpairs are {tl,tl,tr,tr} / {bl,bl,br,br});
     V4: 2 native u32 words holding u16 pairs {t0,t1}, {t2,t3},
     even scanline first in memory (high halfword on the 3DO). */
  uint32  v1cb[256 * 4];
  uint32  v4cb[256 * 2];
} VxDec;

/* VFRM payload parse result stats */
typedef struct VxDecStats {
  uint32 keyframe;
  uint32 v1_runs, v1_blocks, v1rep_runs, v4_runs, v4_blocks, v4rep_runs;
  uint32 skip_runs, skip_blocks;
  uint32 v1_updates, v4_updates;
  uint32 coded_blocks;
  uint32 error; /* 0 ok; else VXE_* code */
} VxDecStats;

#define VXE_OK          0
#define VXE_SUBCHUNK    2  /* bad sub-chunk header/size */
#define VXE_CB_ID       3  /* unknown codebook chunk id */
#define VXE_CB_RANGE    4  /* codebook update out of range */
#define VXE_VEC_OVERRUN 6  /* vec data runs past chunk */
#define VXE_NOT_KEYFRAME 7 /* delta frame decoded on invalid state */

void   vx_dec_init(VxDec *dec, uint16 *backbuf, uint16 width, uint16 height);

/* Decode one VFRM payload (post 16-byte container header):
     u16be flags, u16be reserved, sub-chunks...
   The first frame after init or any error must carry the keyframe flag.
   Errors may partially mutate codebooks/backbuffer and invalidate state;
   the caller must not present that buffer and must seek a fresh keyframe.
   stats may be NULL to omit command-level accounting during playback.
*/
uint32 vx_dec_frame(VxDec *dec, const uint8 *payload, uint32 payload_bytes,
                    VxDecStats *stats);
#endif /* VX_DEC_H */

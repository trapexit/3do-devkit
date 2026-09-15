/*
  3vx_dec.c - 3VX VFRM payload decoder core.

  Grammar (see docs/3vx-design.md):
    payload := u16be flags, u16be reserved, sub-chunk*
    sub-chunk := u8 id, u24be size(incl header), payload, pad to 4
    ids: 0x20 K4F 256*8B | 0x24 K4R range | 0x21 K4S sparse
         0x22 K1F 256*8B packed | 0x25 K1R | 0x23 K1S
         0x40 VEC run bytecodes

  All parsing is byte-driven => bit-identical on ARM60 (BE) and host (LE).

  Backbuffer block addressing (LR):
    block (bx,by): rp0 band = (2*by)   * width*4 bytes, words x=4*bx..+3
                   rp1 band = (2*by+1) * width*4 bytes
    block byte offset within band = bx*16.
*/
#include "vx_dec.h"
#include "string.h"
#ifdef TARGET_3DO
#include "stddef.h"
typedef char VxAsmLayout[(sizeof(void *) == 4 && offsetof(VxDec, blocks_w) == 8
  && offsetof(VxDec, blocks_h) == 10 && offsetof(VxDec, v1cb) == 16
  && offsetof(VxDec, v4cb) == 4112 && offsetof(VxDec, dirty_first) == 6160
  && offsetof(VxDec, dirty_end) == 6164) ? 1 : -1];
extern uint32 vx_run_vec_asm(VxDec *dec, const uint8 *p, uint32 bytes);
#endif

#define KC4_FULL   0x20
#define KC4_SPARSE 0x21
#define KC1_FULL   0x22
#define KC1_SPARSE 0x23
#define KC4_RANGE  0x24
#define KC1_RANGE  0x25
#define KVEC       0x40

static uint16
rd_u16(const uint8 *p)
{
  return (uint16)(((uint16)p[0] << 8) | p[1]);
}

static uint32
rd_u24(const uint8 *p)
{
  return ((uint32)p[0] << 16) | ((uint32)p[1] << 8) | p[2];
}

void
vx_dec_init(VxDec *dec, uint16 *backbuf, uint16 width, uint16 height)
{
  dec->backbuf   = backbuf;
  dec->width     = width;
  dec->height    = height;
  dec->blocks_w  = (uint16)(width >> 2);
  dec->blocks_h  = (uint16)(height >> 2);
  dec->state_valid = 0;
  memset(dec->v1cb, 0, sizeof(dec->v1cb));
  memset(dec->v4cb, 0, sizeof(dec->v4cb));
}

/* Expand one packed V1 entry (TL,TR,BL,BR as 4 u16be) into the
   shattered 4-word tile {tl,tr,bl,br} with each color doubled into
   both halfwords. */
static void
expand_v1(VxDec *dec, uint32 index, const uint8 *p)
{
  uint32 *e = dec->v1cb + index * 4;
#ifdef TARGET_3DO
  if(((uint32)p & 3u) == 0)
    {
      uint32 top = ((const uint32 *)p)[0];
      uint32 bottom = ((const uint32 *)p)[1];
      uint32 tl = top >> 16, tr = top & 0xffffu;
      uint32 bl = bottom >> 16, br = bottom & 0xffffu;
      e[0] = tl | (tl << 16); e[1] = tr | (tr << 16);
      e[2] = bl | (bl << 16); e[3] = br | (br << 16);
      return;
    }
#endif
  {
  uint32 tl = rd_u16(p);
  uint32 tr = rd_u16(p + 2);
  uint32 bl = rd_u16(p + 4);
  uint32 br = rd_u16(p + 6);

  e[0] = tl | (tl << 16);
  e[1] = tr | (tr << 16);
  e[2] = bl | (bl << 16);
  e[3] = br | (br << 16);
  }
}

/* Load a V4 entry: 8 bytes = 4 u16be -> 2 LR words. */
static void
load_v4(VxDec *dec, uint32 index, const uint8 *p)
{
  uint32 *e = dec->v4cb + index * 2;
#ifdef TARGET_3DO
  if(((uint32)p & 3u) == 0)
    {
      e[0] = ((const uint32 *)p)[0];
      e[1] = ((const uint32 *)p)[1];
      return;
    }
#endif
  {
  uint32 a = rd_u16(p);
  uint32 b = rd_u16(p + 2);
  uint32 cc = rd_u16(p + 4);
  uint32 d = rd_u16(p + 6);

  /* LR words store the even scanline first in memory. ARM60 is
     big-endian; the host decoder must retain its native halfword order. */
#if defined(TARGET_3DO) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  e[0] = (a << 16) | b;
  e[1] = (cc << 16) | d;
#else
  e[0] = a | (b << 16);
  e[1] = cc | (d << 16);
#endif
  }
}

/* ---- block painters ------------------------------------------------ */

#ifndef TARGET_3DO
static void
put_v1(const VxDec *dec, uint16 *d0, uint16 *d1, uint32 index)
{
  const uint32 *t = dec->v1cb + index * 4;
  uint32 *w0 = (uint32 *)d0;
  uint32 *w1 = (uint32 *)d1;

  w0[0] = w0[1] = t[0];  w0[2] = w0[3] = t[1];
  w1[0] = w1[1] = t[2];  w1[2] = w1[3] = t[3];
}
#endif

/* Copy four V4 2x2 tiles (i0=TL,i1=TR,i2=BL,i3=BR). */
#ifndef TARGET_3DO
static void
put_v4(const VxDec *dec, uint16 *d0, uint16 *d1, const uint8 *idx)
{
  const uint32 *t;
  uint32 *w0 = (uint32 *)d0;
  uint32 *w1 = (uint32 *)d1;

  (void)dec;
  t = dec->v4cb + (uint32)idx[0] * 2;
  w0[0] = t[0]; w0[1] = t[1];
  t = dec->v4cb + (uint32)idx[1] * 2;
  w0[2] = t[0]; w0[3] = t[1];
  t = dec->v4cb + (uint32)idx[2] * 2;
  w1[0] = t[0]; w1[1] = t[1];
  t = dec->v4cb + (uint32)idx[3] * 2;
  w1[2] = t[0]; w1[3] = t[1];
}
#endif

/* ---- codebook sub-chunks ------------------------------------------- */

/* Load `count` codebook entries with consecutive indices starting at
   `first` from a stride-8 entry array at `p` (full chunks feed p, range
   chunks p+4). The caller has already validated the payload size and
   index range. Alignment and the v1/v4 shape are decided once per batch
   so the entry loops carry no per-entry test or call: an aligned V4 run
   is already the resident native words (one contiguous copy), while V1
   entries always need the doubling expansion. Unaligned sources (and
   every host build) fall back to the per-entry byte-wise forms. */
static void
load_entries(VxDec *dec, uint32 first, const uint8 *p, uint32 count,
             int is_v1)
{
  uint32 i;

  if(is_v1)
    {
      uint32 *e = dec->v1cb + first * 4;

#ifdef TARGET_3DO
      if(((uint32)p & 3u) == 0)
        {
          const uint32 *w = (const uint32 *)p;

          for(i = 0; i < count; i++)
            {
              uint32 top = w[0], bottom = w[1];
              uint32 tl = top >> 16, tr = top & 0xffffu;
              uint32 bl = bottom >> 16, br = bottom & 0xffffu;

              e[0] = tl | (tl << 16);
              e[1] = tr | (tr << 16);
              e[2] = bl | (bl << 16);
              e[3] = br | (br << 16);
              w += 2;
              e += 4;
            }
          return;
        }
#endif
      for(i = 0; i < count; i++)
        {
          uint32 tl = rd_u16(p);
          uint32 tr = rd_u16(p + 2);
          uint32 bl = rd_u16(p + 4);
          uint32 br = rd_u16(p + 6);

          e[0] = tl | (tl << 16);
          e[1] = tr | (tr << 16);
          e[2] = bl | (bl << 16);
          e[3] = br | (br << 16);
          p += 8;
          e += 4;
        }
      return;
    }

#ifdef TARGET_3DO
  if(((uint32)p & 3u) == 0)
    {
      /* Aligned V4: every 8-byte stream entry is exactly the pair of
         resident native words, so the whole run is one copy. */
      memcpy(dec->v4cb + first * 2, p, (size_t)(count * 8));
      return;
    }
#endif
  for(i = 0; i < count; i++)
    load_v4(dec, first + i, p + i * 8);
}

static uint32
load_codebook_chunk(VxDec *dec, const uint8 *p, uint32 bytes,
                    uint8 id)
{
  int is_v1 = (id == KC1_FULL || id == KC1_SPARSE || id == KC1_RANGE);

  if(id == KC4_FULL || id == KC1_FULL)
    {
      /* full: 256 entries, entry bytes = 8 (v4 raw / v1 packed) */
      if(bytes < 256 * 8)
        return VXE_SUBCHUNK;
      load_entries(dec, 0, p, 256, is_v1);
      return VXE_OK;
    }

  if(id == KC4_RANGE || id == KC1_RANGE)
    {
      uint32 first, last;

      if(bytes < 4)
        return VXE_SUBCHUNK;
      first = p[0];
      last  = p[1];
      if(last < first || last > 255 || bytes < 4 + (last - first + 1) * 8)
        return VXE_CB_RANGE;
      load_entries(dec, first, p + 4, last - first + 1, is_v1);
      return VXE_OK;
    }

  /* sparse: records of [u8 index][3 pad][8B entry] */
  {
    uint32 recs = bytes / 12;
    uint32 i;

    if(recs * 12 != bytes)
      return VXE_SUBCHUNK;
    for(i = 0; i < recs; i++)
      {
        const uint8 *r = p + i * 12;

        if(is_v1)
          expand_v1(dec, r[0], r + 4);
        else
          load_v4(dec, r[0], r + 4);
      }
    return VXE_OK;
  }
}

/* ---- VEC interpreter ------------------------------------------------ */

#ifndef TARGET_3DO
/* Portable implementation of the same validated-input contract. */
static uint32
run_vec(VxDec *dec, const uint8 *p)
{
  uint32 row, bx, n, k;
  uint32 stride = (uint32)dec->width * 2;
  for(row = 0; row < dec->blocks_h; row++)
    {
      uint16 *d0 = dec->backbuf + row * 2 * stride;
      uint16 *d1 = d0 + stride;
      bx = 0;
      while(bx < dec->blocks_w)
        {
          uint32 op = *p++;
          n = (op & 63u) + 1;
          if(op >= 64)
            {
              if(row < dec->dirty_first) dec->dirty_first = row;
              dec->dirty_end = row + 1;
            }
          if(op < 64)
            { d0 += n * 8; d1 += n * 8; }
          else if(op < 128)
            {
              for(k = 0; k < n; k++)
                { put_v1(dec, d0, d1, *p++); d0 += 8; d1 += 8; }
            }
          else if(op < 192)
            {
              for(k = 0; k < n; k++)
                { put_v4(dec, d0, d1, p); p += 4; d0 += 8; d1 += 8; }
            }
          else if(op < 255)
            {
              uint32 index = *p++;
              for(k = 0; k < n; k++)
                { put_v1(dec, d0, d1, index); d0 += 8; d1 += 8; }
            }
          else
            {
              n = (uint32)*p++ + 1;
              for(k = 0; k < n; k++)
                { put_v4(dec, d0, d1, p); d0 += 8; d1 += 8; }
              p += 4;
            }
          bx += n;
        }
    }
  return VXE_OK;
}
#endif

uint32
vx_dec_frame(VxDec *dec, const uint8 *payload, uint32 payload_bytes)
{
  uint32 pos = 4;  /* skip flags+reserved */
  uint16 flags;
  uint32 saw_vec = 0;
  uint32 err = VXE_OK;
  dec->dirty_first = dec->blocks_h;
  dec->dirty_end = 0;

  if(payload_bytes < 4)
    {
      dec->state_valid = 0;
      return VXE_SUBCHUNK;
    }


  flags = rd_u16(payload);
  flags &= 1;

  if(!flags && !dec->state_valid)
    return VXE_NOT_KEYFRAME;

  while(pos + 4 <= payload_bytes)
    {
      uint8 id = payload[pos];
      uint32 size = rd_u24(payload + pos + 1);
      uint32 cbytes;

      if(size < 4 || pos + size > payload_bytes)
        { err = VXE_SUBCHUNK; goto out; }
      cbytes = size - 4;

      switch(id)
        {
        case KC4_FULL:
        case KC4_SPARSE:
        case KC4_RANGE:
        case KC1_FULL:
        case KC1_SPARSE:
        case KC1_RANGE:
          if(saw_vec)
            { err = VXE_SUBCHUNK; goto out; }  /* codebooks precede VEC */
          err = load_codebook_chunk(dec, payload + pos + 4, cbytes, id);
          if(err)
            goto out;
          break;
        case KVEC:
#ifdef TARGET_3DO
          err = vx_run_vec_asm(dec, payload + pos + 4, cbytes);
#else
          err = run_vec(dec, payload + pos + 4);
#endif
          if(err)
            goto out;
          saw_vec = 1;
          break;
        default:
          err = VXE_CB_ID;
          goto out;
        }

      pos += ((size + 3) / 4) * 4;
    }

  if(pos != payload_bytes)
    { err = VXE_SUBCHUNK; goto out; }
  if(!saw_vec)
    { err = VXE_SUBCHUNK; goto out; }

  dec->state_valid = 1;
  return VXE_OK;

out:
  dec->state_valid = 0;
  return err;
}


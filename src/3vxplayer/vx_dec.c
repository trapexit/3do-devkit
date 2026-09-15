/*
  vx_dec.c - 3VX VFRM payload decoder core.

  One optimized validated-input decoder: vx_dec_frame(dec, payload, bytes).
  Codebook sub-chunks expand into the resident tables; the single VEC
  sub-chunk is interpreted by the fused ARMv3 interpreter on target
  (vx_vec_accel.s) or the portable twin run_vec on the host. Both share
  the same validated-input precondition; chunk framing, codebook bounds
  and prediction-state checks remain. The VEC pass also records the
  conservative coded block-row bounds (dirty_first/dirty_end) the player
  uses to crop its VRAM band draw.

  VxDec layout is pinned by the compile-time assert below because the
  assembly reaches these fields directly: v1cb@16, v4cb@4112,
  dirty_first@6160, dirty_end@6164.
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

/* ---- codebook entry bodies ---------------------------------------- */

/* One packed entry is 8 bytes: V1 = TL,TR,BL,BR u16be, expanded into
   the resident 4-word tile with each color doubled into both
   halfwords; V4 = 4 u16be forming the 2 resident LR words (even
   scanline first in memory on the 3DO, host keeps native halfword
   order). These are macros rather than static helpers because armcc
   2.51 does not inline C89 functions and the sparse path must not pay
   a call per record. Each macro reads exactly one entry and writes it
   through (e); callers supply a walking pointer. */

#define VX_V1_FROM_WORDS(e, w0, w1) \
  do { \
    uint32 vx_tt = (w0), vx_bb = (w1); \
    uint32 vxtl = vx_tt >> 16, vxtr = vx_tt & 0xffffu; \
    uint32 vxbl = vx_bb >> 16, vxbr = vx_bb & 0xffffu; \
    (e)[0] = vxtl | (vxtl << 16); \
    (e)[1] = vxtr | (vxtr << 16); \
    (e)[2] = vxbl | (vxbl << 16); \
    (e)[3] = vxbr | (vxbr << 16); \
  } while(0)

#define VX_V1_FROM_BYTES(e, q) \
  do { \
    uint32 vxtl = rd_u16(q); \
    uint32 vxtr = rd_u16((q) + 2); \
    uint32 vxbl = rd_u16((q) + 4); \
    uint32 vxbr = rd_u16((q) + 6); \
    (e)[0] = vxtl | (vxtl << 16); \
    (e)[1] = vxtr | (vxtr << 16); \
    (e)[2] = vxbl | (vxbl << 16); \
    (e)[3] = vxbr | (vxbr << 16); \
  } while(0)

#define VX_V4_FROM_WORDS(e, w0, w1) \
  do { (e)[0] = (w0); (e)[1] = (w1); } while(0)

/* Unaligned/byte-form V4 keeps the byte-order split of the original
   load_v4: the 3DO (big-endian) word is (first<<16)|second, the host
   decoder retains its native halfword order. */
#if defined(TARGET_3DO) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define VX_V4_FROM_BYTES(e, q) \
  do { \
    uint32 vxa = rd_u16(q), vxb = rd_u16((q) + 2); \
    uint32 vxc = rd_u16((q) + 4), vxd = rd_u16((q) + 6); \
    (e)[0] = (vxa << 16) | vxb; \
    (e)[1] = (vxc << 16) | vxd; \
  } while(0)
#else
#define VX_V4_FROM_BYTES(e, q) \
  do { \
    uint32 vxa = rd_u16(q), vxb = rd_u16((q) + 2); \
    uint32 vxc = rd_u16((q) + 4), vxd = rd_u16((q) + 6); \
    (e)[0] = vxa | (vxb << 16); \
    (e)[1] = vxc | (vxd << 16); \
  } while(0)
#endif

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
   entries always need the doubling expansion. The aligned V1 loop is
   unrolled once; unaligned sources (and every host build) fall back to
   the byte-wise forms. */
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
          /* Aligned V1 batch: two native source words per entry.
             Unrolled once; single post-increment loads and stores. */
          const uint32 *w = (const uint32 *)p;

          i = count >> 1;
          while(i--)
            {
              uint32 top = *w++;
              uint32 bottom = *w++;
              VX_V1_FROM_WORDS(e, top, bottom);
              e += 4;
              top = *w++;
              bottom = *w++;
              VX_V1_FROM_WORDS(e, top, bottom);
              e += 4;
            }
          if(count & 1u)
            {
              uint32 top = *w++;
              uint32 bottom = *w++;
              VX_V1_FROM_WORDS(e, top, bottom);
            }
          return;
        }
#endif
      {
        const uint8 *q = p;

        for(i = 0; i < count; i++)
          {
            VX_V1_FROM_BYTES(e, q);
            q += 8;
            e += 4;
          }
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
  {
    uint32 *e = dec->v4cb + first * 2;
    const uint8 *q = p;

    for(i = 0; i < count; i++)
      {
        VX_V4_FROM_BYTES(e, q);
        q += 8;
        e += 2;
      }
  }
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

  /* sparse: records of [u8 index][3 pad][8B entry]. The v1/v4 shape is
     decided once per chunk, and every record entry sits at p + i*12 + 4
     so a single alignment test covers the whole run: the record bodies
     are inlined with no per-record call, is_v1 test, alignment test or
     i*12 multiply. */
  {
    uint32 recs = bytes / 12;
    uint32 n;
    const uint8 *r;

    if(recs * 12 != bytes)
      return VXE_SUBCHUNK;

    if(is_v1)
      {
#ifdef TARGET_3DO
        if(((uint32)p & 3u) == 0)
          {
            n = recs;
            r = p;
            while(n--)
              {
                uint32 *e = dec->v1cb + (uint32)r[0] * 4;
                const uint32 *w = (const uint32 *)(r + 4);

                VX_V1_FROM_WORDS(e, w[0], w[1]);
                r += 12;
              }
            return VXE_OK;
          }
#endif
        n = recs;
        r = p;
        while(n--)
          {
            VX_V1_FROM_BYTES(dec->v1cb + (uint32)r[0] * 4, r + 4);
            r += 12;
          }
        return VXE_OK;
      }

#ifdef TARGET_3DO
    if(((uint32)p & 3u) == 0)
      {
        n = recs;
        r = p;
        while(n--)
          {
            uint32 *e = dec->v4cb + (uint32)r[0] * 2;
            const uint32 *w = (const uint32 *)(r + 4);

            VX_V4_FROM_WORDS(e, w[0], w[1]);
            r += 12;
          }
        return VXE_OK;
      }
#endif
    n = recs;
    r = p;
    while(n--)
      {
        VX_V4_FROM_BYTES(dec->v4cb + (uint32)r[0] * 2, r + 4);
        r += 12;
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

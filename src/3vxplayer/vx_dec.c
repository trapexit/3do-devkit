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

#ifdef TARGET_3DO
extern void vx_put_v1_asm(uint32 *rp0, uint32 *rp1, const uint32 *tile4);
extern void vx_put_v4_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
                          const uint8 *idx4);
extern void vx_v4_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
                          const uint8 *idxlist, uint32 n);
extern void vx_v1_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *v1cb,
                          const uint8 *idxlist, uint32 n);
extern void vx_v1rep_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *v1cb,
                             const uint8 *idx, uint32 n);
extern void vx_v4rep_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
                             const uint8 *entry, uint32 n);
#endif
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

static uint32
load_codebook_chunk(VxDec *dec, const uint8 *p, uint32 bytes,
                    uint8 id, VxDecStats *st)
{
  int is_v1 = (id == KC1_FULL || id == KC1_SPARSE || id == KC1_RANGE);

  if(id == KC4_FULL || id == KC1_FULL)
    {
      /* full: 256 entries, entry bytes = 8 (v4 raw / v1 packed) */
      uint32 i;

      if(bytes < 256 * 8)
        return VXE_SUBCHUNK;
      for(i = 0; i < 256; i++)
        {
          if(is_v1)
            expand_v1(dec, i, p + i * 8);
          else
            load_v4(dec, i, p + i * 8);
        }
      if(is_v1)
        st->v1_updates += 256;
      else
        st->v4_updates += 256;
      return VXE_OK;
    }

  if(id == KC4_RANGE || id == KC1_RANGE)
    {
      uint32 first, last, i;

      if(bytes < 4)
        return VXE_SUBCHUNK;
      first = p[0];
      last  = p[1];
      if(last < first || last > 255 || bytes < 4 + (last - first + 1) * 8)
        return VXE_CB_RANGE;
      for(i = first; i <= last; i++)
        {
          if(is_v1)
            expand_v1(dec, i, p + 4 + (i - first) * 8);
          else
            load_v4(dec, i, p + 4 + (i - first) * 8);
        }
      if(is_v1)
        st->v1_updates += (last - first + 1);
      else
        st->v4_updates += (last - first + 1);
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
    if(is_v1)
      st->v1_updates += recs;
    else
      st->v4_updates += recs;
    return VXE_OK;
  }
}

/* ---- VEC interpreter ------------------------------------------------ */

static uint32
run_vec(VxDec *dec, const uint8 *p, uint32 bytes, VxDecStats *st)
{
  uint32 bx = 0, by = 0;
  uint32 i = 0;
  uint32 bw = dec->blocks_w;
  uint32 bh = dec->blocks_h;
  uint32 w16 = (uint32)dec->width * 2;   /* u16s per rowpair band */
  uint16 *d0 = dec->backbuf;                 /* rp0 band ptr, cur block */
  uint16 *d1 = d0 + w16;                     /* rp1 band ptr */

  /* Chunk payloads pad with zeros to a 4-byte boundary, and 0x00 is a
     valid skip-1 opcode: STOP as soon as the block grid is complete and
     treat the remaining bytes as padding. */
#define VEC_GRID_DONE() (by == bh && bx == 0)
#define VEC_ADV() do { bx++; d0 += 8; d1 += 8; \
                       if(bx == bw) { bx = 0; by++; \
                                      if(by <= bh) { \
                                        d0 = dec->backbuf + (2 * by) * w16; \
                                        d1 = d0 + w16; \
                                      } } } while(0)
#define VEC_SKIPN(N) do { uint32 m = (N); \
                          while(m > 0) { \
                            uint32 kk = bw - bx; \
                            if(kk > m) kk = m; \
                            bx += kk; d0 += kk * 8; d1 += kk * 8; m -= kk; \
                            if(bx == bw) { bx = 0; by++; \
                                           if(by <= bh) { \
                                             d0 = dec->backbuf + (2 * by) * w16; \
                                             d1 = d0 + w16; \
                                           } } } } while(0)

  while(i < bytes && !VEC_GRID_DONE())
    {
      uint8 op = p[i++];
      uint32 n;
#ifndef TARGET_3DO
      uint32 k;
#endif

      if(op < 0x40)
        {
          /* SKIP run */
          n = (uint32)op + 1;
          if(by >= bh || bx + n > bw)
            return VXE_VEC_OVERRUN;
          if(st) st->skip_runs++;
          if(st) st->skip_blocks += n;
          VEC_SKIPN(n);
          continue;
        }
      if(op < 0x80)
        {
          /* V1 literal run */
          n = (uint32)(op - 0x40) + 1;
          if(by >= bh || bx + n > bw || i + n > bytes)
            return VXE_VEC_OVERRUN;
          if(st) st->v1_runs++;
          if(st) st->v1_blocks += n;
          if(st) st->coded_blocks += n;
#ifdef TARGET_3DO
          vx_v1_run_asm((uint32 *)d0, (uint32 *)d1, dec->v1cb, p + i, n);
          /* the dispatcher guard (bx + n > bw) proves the run cannot
             cross a block-row end: advance is straight-line with at
             most one row wrap */
          bx += n; d0 += n * 8; d1 += n * 8;
          if(bx == bw)
            {
              bx = 0; by++;
              if(by <= bh)
                {
                  d0 = dec->backbuf + (2 * by) * w16;
                  d1 = d0 + w16;
                }
            }
          i += n;
#else
          for(k = 0; k < n; k++)
            {
              put_v1(dec, d0, d1, p[i + k]);
              VEC_ADV();
            }
          i += n;
#endif
          continue;
        }
      if(op < 0xc0)
        {
          /* V4 literal run */
          n = (uint32)(op - 0x80) + 1;
          if(by >= bh || bx + n > bw || i + 4 * n > bytes)
            return VXE_VEC_OVERRUN;
          if(st) st->v4_runs++;
          if(st) st->v4_blocks += n;
          if(st) st->coded_blocks += n;
#ifdef TARGET_3DO
          vx_v4_run_asm((uint32 *)d0, (uint32 *)d1, dec->v4cb, p + i, n);
          bx += n; d0 += n * 8; d1 += n * 8;
          if(bx == bw)
            {
              bx = 0; by++;
              if(by <= bh)
                {
                  d0 = dec->backbuf + (2 * by) * w16;
                  d1 = d0 + w16;
                }
            }
          i += 4 * n;
#else
          for(k = 0; k < n; k++)
            {
              put_v4(dec, d0, d1, p + i);
              i += 4;
              VEC_ADV();
            }
#endif
          continue;
        }
      if(op < 0xff)
        {
          /* V1 repeat run */
          n = (uint32)(op - 0xc0) + 1;
          if(by >= bh || bx + n > bw || i + 1 > bytes)
            return VXE_VEC_OVERRUN;
          if(st) st->v1rep_runs++;
          if(st) st->v1_blocks += n;
          if(st) st->coded_blocks += n;
#ifdef TARGET_3DO
          vx_v1rep_run_asm((uint32 *)d0, (uint32 *)d1, dec->v1cb, p + i, n);
          /* same O(1) advance as the literal branches: the guard
             (bx + n > bw) proves the run cannot cross a block-row end */
          bx += n; d0 += n * 8; d1 += n * 8;
          if(bx == bw)
            {
              bx = 0; by++;
              if(by <= bh)
                {
                  d0 = dec->backbuf + (2 * by) * w16;
                  d1 = d0 + w16;
                }
            }
          i++;   /* a repeat paints one tile n times */
#else
          for(k = 0; k < n; k++)
            {
              put_v1(dec, d0, d1, p[i]);
              VEC_ADV();
            }
          i++;   /* kept scalar: a repeat paints one tile n times */
#endif
          continue;
        }
      /* op == 0xff: V4 repeat */
      if(i + 5 > bytes)
        return VXE_VEC_OVERRUN;
      n = (uint32)p[i] + 1;
      if(by >= bh || bx + n > bw)
        return VXE_VEC_OVERRUN;
      if(st) st->v4rep_runs++;
      if(st) st->v4_blocks += n;
      if(st) st->coded_blocks += n;
#ifdef TARGET_3DO
          vx_v4rep_run_asm((uint32 *)d0, (uint32 *)d1, dec->v4cb, p + i + 1, n);
          /* same O(1) advance as the literal branches: the guard
             (bx + n > bw) proves the run cannot cross a block-row end */
          bx += n; d0 += n * 8; d1 += n * 8;
          if(bx == bw)
            {
              bx = 0; by++;
              if(by <= bh)
                {
                  d0 = dec->backbuf + (2 * by) * w16;
                  d1 = d0 + w16;
                }
            }
          i += 5;   /* single V4 entry painted n times */
#else
          for(k = 0; k < n; k++)
            {
              put_v4(dec, d0, d1, p + i + 1);
              VEC_ADV();
            }
          i += 5;   /* scalar: single V4 painted n times */
#endif
    }

  if(by != bh || bx != 0)
    return VXE_VEC_OVERRUN;  /* frame must end exactly at grid end */
  (void)bytes;
  return VXE_OK;
#undef VEC_GRID_DONE
#undef VEC_ADV
#undef VEC_SKIPN
}

uint32
vx_dec_frame(VxDec *dec, const uint8 *payload, uint32 payload_bytes,
             VxDecStats *stats)
{
  uint32 pos = 4;  /* skip flags+reserved */
  uint16 flags;
  uint32 saw_vec = 0;
  uint32 err = VXE_OK;
  VxDecStats local;
  VxDecStats *st = stats ? stats : &local;

  if(payload_bytes < 4)
    {
      dec->state_valid = 0;
      st->error = VXE_SUBCHUNK;
      return VXE_SUBCHUNK;
    }

  {
    uint32 *z = (uint32 *)st;
    uint32 i, n = sizeof(*st) / sizeof(uint32);
    for(i = 0; i < n; i++)
      z[i] = 0;
  }

  flags = rd_u16(payload);
  st->keyframe = (flags & 1) ? 1 : 0;

  if(!st->keyframe && !dec->state_valid)
    {
      st->error = VXE_NOT_KEYFRAME;
      return st->error;
    }

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
          err = load_codebook_chunk(dec, payload + pos + 4, cbytes, id, st);
          if(err)
            goto out;
          break;
        case KVEC:
          err = run_vec(dec, payload + pos + 4, cbytes, stats);
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
  st->error = VXE_OK;
  return VXE_OK;

out:
  dec->state_valid = 0;
  st->error = err;
  return err;
}

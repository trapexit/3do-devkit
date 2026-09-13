/*
  3vx_stream.c - see vx_stream.h.
  Chunk framing (big-endian): u32 fourcc, u32 size (incl 16B header),
  u32 time, u32 channel.
*/
#include "vx_stream.h"
#include "debug.h"
#include "mem.h"
#include "msgport.h"
#include "string.h"

#define CHUNK_HDR 16
#define SECTOR    2048

static uint32
rd32(const uint8 *p)
{
  return ((uint32)p[0] << 24) | ((uint32)p[1] << 16)
       | ((uint32)p[2] << 8) | (uint32)p[3];
}

static uint16
rd16(const uint8 *p)
{
  return (uint16)(((uint16)p[0] << 8) | (uint16)p[1]);
}

/* cur/fill are absolute file offsets; physical storage wraps independently. */
static void
window_copy(const VxStream *st, uint32 offset, uint8 *dst, uint32 bytes)
{
  uint32 pos = offset % VX_WIN_BYTES;
  uint32 first = VX_WIN_BYTES - pos;
  if(first > bytes) first = bytes;
  memcpy(dst, st->win + pos, first);
  if(first < bytes) memcpy(dst + first, st->win, bytes - first);
}

/* ---------------- read engine ---------------- */

static void
issue_read(VxStream *st)
{
  uint32 free_tail, want, pos;
  Err err;

  if(st->read_pending || st->eof)
    return;

  free_tail = VX_WIN_BYTES - (st->fill - st->cur);
  pos = st->fill % VX_WIN_BYTES;
  want = VX_READ_BYTES;
  if(want > VX_WIN_BYTES - pos) want = VX_WIN_BYTES - pos;
  if(free_tail < want)
    {
      uint8 hdr[CHUNK_HDR];
      /* Normally wait for a whole read's room. A chunk larger than the
         reserve needs a partial read to finish instead of deadlocking. */
      if(st->fill - st->cur >= CHUNK_HDR)
        {
          window_copy(st, st->cur, hdr, CHUNK_HDR);
          if(rd32(hdr + 4) <= st->fill - st->cur) return;
        }
      want = free_tail & ~(SECTOR - 1u);
    }
  if(want == 0)
    return;
  if(st->next_read_off + want > (uint32)st->file_size)
    {
      uint32 rem = (uint32)st->file_size - st->next_read_off;
      if(rem == 0)
        { st->eof = 1; return; }
      want = (rem + SECTOR - 1) & ~(SECTOR - 1u); /* last block overhang */
    }

  err = AsynchReadBlockFile(&st->bf, st->ioreq,
                            st->win + pos, (int32)want,
                            (int32)st->next_read_off);
  if(err >= 0)
    {
      st->read_pending = 1;
      st->pend_bytes   = want;
    }
  else
    {
      kprintf("read err %ld at %lu\n", (long)err, (unsigned long)st->next_read_off);
      st->wedged = 1;
    }
}

static void
finish_read(VxStream *st)
{
  uint32 got;

  if(!st->read_pending)
    return;
  if(!ReadDoneBlockFile(st->ioreq))
    return;
  if(WaitReadDoneBlockFile(st->ioreq) < 0)
    { st->read_pending = 0; st->wedged = 1; return; }
  st->read_pending = 0;

  /* only bytes inside the file are valid (final block may overhang) */
  got = st->pend_bytes;
  if(st->next_read_off + got > (uint32)st->file_size)
    got = (uint32)st->file_size - st->next_read_off;
  st->fill += got;
  st->next_read_off += got;
  if(st->next_read_off >= (uint32)st->file_size)
    st->eof = 1;
}

/* ---------------- demux ---------------- */

static void
parse_header(VxStream *st, const uint8 *p)
{
  VxStreamInfo *i = &st->info;

#ifdef DEBUG
  {
    int j;
    for(j = 0; j < 48; j += 4)
      kprintf("hdr@%02d = %08lx\n", j, (unsigned long)rd32(p + j));
  }
#endif

  i->width               = rd16(p + 0x14);
  i->height              = rd16(p + 0x16);
  i->fps_num             = rd32(p + 0x18);
  i->fps_den             = rd32(p + 0x1c);
  i->frame_count         = rd32(p + 0x20);
  i->audio_rate          = rd32(p + 0x24);
  i->audio_channels      = rd16(p + 0x28);
  i->audio_format        = rd16(p + 0x2a);
  i->audio_bytes_per_sec = rd32(p + 0x2c);
  i->keyframe_interval   = rd32(p + 0x34);
  /* Bound duration below all 32-bit sample/byte products (over four hours).
     Reject unsupported geometry/rates rather than silently desynchronizing. */
  if(rd16(p + 16) != 1 || rd16(p + 18) != 64
     || i->width != 320 || i->height != 240
     || i->fps_num != 30000 || i->fps_den != 1001
     || i->audio_rate != 22050 || i->audio_channels != 2
     || i->audio_format != 0x5332 || i->audio_bytes_per_sec != 44100
     || i->frame_count == 0 || i->frame_count > 450000u)
    { st->wedged = 1; return; }
  st->header_ok = 1;
}

static void
demux(VxStream *st)
{
  if(st->skip_remaining)
    {
      uint32 n = st->fill - st->cur;
      if(n > st->skip_remaining) n = st->skip_remaining;
      st->cur += n;
      st->skip_remaining -= n;
      if(st->skip_remaining) return;
    }
  while(st->fill - st->cur >= CHUNK_HDR)
    {
      uint8 header[CHUNK_HDR];
      uint32 magic, size, time;
      window_copy(st, st->cur, header, CHUNK_HDR);
      magic = rd32(header);
      size = rd32(header + 4);
      time = rd32(header + 8);

      if(size < CHUNK_HDR || size > VX_WIN_BYTES || (size & 3)
         || size > (uint32)st->file_size - st->cur)
        { st->wedged = 1; return; }
      if(magic != 0x33565848 && magic != 0x5646524d && magic != 0x41554430)
        {
          uint32 n = st->fill - st->cur;
          if(n > size) n = size;
          st->cur += n;
          st->skip_remaining = size - n;
          if(st->skip_remaining) return;
          continue;
        }
      if((magic == 0x33565848 && size != 80)
         || (magic == 0x5646524d && size > VX_FRAME_MAX + CHUNK_HDR)
         || (magic == 0x41554430 && size > VX_AUDRING_BYTES + CHUNK_HDR + 4))
        { st->wedged = 1; return; }
      if(st->fill - st->cur < size)
        return;  /* incomplete: wait for more data */

      switch(magic)
        {
        case 0x33565848: {
          uint8 file_header[80];
          if(st->header_ok || size != 80)
            { st->wedged = 1; return; }
          window_copy(st, st->cur, file_header, 80);
          parse_header(st, file_header);
          break;
        }

        case 0x5646524d: /* VFRM */
          if(!st->header_ok || size < CHUNK_HDR + 4
             || size - CHUNK_HDR > VX_FRAME_MAX
             || time != st->frames_delivered || time >= st->info.frame_count)
            { st->wedged = 1; return; }
          if(st->frame_ready[st->slot_w])
            { st->slotfull++; return; }  /* backpressure: leave chunk staged */
          st->n_vfrm++;
          window_copy(st, st->cur + CHUNK_HDR, st->frames[st->slot_w], size - CHUNK_HDR);
          st->frame_len[st->slot_w]   = size - CHUNK_HDR;
          st->frame_index[st->slot_w] = time;
          st->frame_ready[st->slot_w] = 1;
          st->slot_w++; if(st->slot_w == VX_FRAME_SLOTS) st->slot_w = 0;
          st->frames_delivered++;
          break;

        case 0x41554430: { /* AUD0: payload = u32 count + SDX2 bytes */
          uint32 bytes = size - CHUNK_HDR;
          uint32 avail, w, first, count;
          uint8 count_bytes[4];
          if(bytes < 4) { st->wedged = 1; return; }
          window_copy(st, st->cur + CHUNK_HDR, count_bytes, 4);
          count = rd32(count_bytes);

          if(!st->header_ok || bytes < 4
             || time != st->audio_payload_bytes / 2
             || count > (bytes - 4) / 2
             || count > VX_AUDRING_BYTES / 2)
            { st->wedged = 1; return; }
          bytes = count * 2u;  /* SDX2 stereo bytes */
          if(bytes + 4 > size - CHUNK_HDR)
            { st->wedged = 1; return; }
          avail = VX_AUDRING_BYTES - (st->aud_head - st->aud_tail);
          if(avail < bytes)
            { st->aud_ringfull++; return; } /* backpressure */
          w     = st->aud_head % VX_AUDRING_BYTES;
          first = VX_AUDRING_BYTES - w;
          if(first > bytes)
            first = bytes;
          window_copy(st, st->cur + CHUNK_HDR + 4, st->aud + w, first);
          if(first < bytes)
            window_copy(st, st->cur + CHUNK_HDR + 4 + first, st->aud, bytes - first);
          st->aud_head += bytes;
          st->audio_payload_bytes += bytes;
          st->n_aud0++;
          break;
        }

        default:
          break;  /* forward compat: skip unknown chunks */
        }

      st->cur += size;
      if(st->wedged)
        return;
    }
  if(st->eof && st->cur == st->fill && !st->eof_said)
    {
      st->eof_said = 1;
      kprintf("demux eof: vfs %lu\n", (unsigned long)st->n_vfrm);
      kprintf(" auds %lu\n", (unsigned long)st->n_aud0);
      kprintf(" head %lu\n", (unsigned long)st->aud_head);
      kprintf(" tail %lu\n", (unsigned long)st->aud_tail);
      kprintf(" ringfull_ev %lu\n", (unsigned long)st->aud_ringfull);
      kprintf(" slotfull_ev %lu\n", (unsigned long)st->slotfull);
    }
}

/* ---------------- public ---------------- */

/* Free the open-time buffers, nulling each so a later vx_stream_close
   (guards test pointers, not ownership) never double-frees them. */
static void
free_buffers(VxStream *st)
{
  int s;

  if(st->win) { FreeMem(st->win, VX_WIN_BYTES); st->win = NULL; }
  for(s = 0; s < VX_FRAME_SLOTS; s++)
    if(st->frames[s])
      { FreeMem(st->frames[s], VX_FRAME_MAX); st->frames[s] = NULL; }
  if(st->aud) { FreeMem(st->aud, VX_AUDRING_BYTES); st->aud = NULL; }
}

Err
vx_stream_open(VxStream *st, const char *path)
{
  Err err;
  int s;

  memset(st, 0, sizeof(*st));
  st->port = -1;   /* close guards: DeleteMsgPort/DeleteItem only when >= 0 */
  st->ioreq = -1;

  st->win = (uint8 *)AllocMem(VX_WIN_BYTES,
                              MEMTYPE_DRAM | MEMTYPE_STARTPAGE | MEMTYPE_FILL);
  for(s = 0; s < VX_FRAME_SLOTS; s++)
    st->frames[s] = (uint8 *)AllocMem(VX_FRAME_MAX, MEMTYPE_DRAM);
  st->aud = (uint8 *)AllocMem(VX_AUDRING_BYTES, MEMTYPE_DRAM);
  if(!st->win || !st->aud || !st->frames[0] || !st->frames[1]
     || !st->frames[2] || !st->frames[3])
    {
      free_buffers(st);          /* nulls win/frames/aud */
      return -1;
    }
  err = OpenBlockFile((char *)path, &st->bf);
  if(err < 0)
    {
      free_buffers(st);          /* bf never opened; bf_open stays 0 */
      return err;
    }
  st->bf_open = 1;
  st->file_size = GetBlockFileSize(&st->bf);

  st->port = CreateMsgPort(NULL, 0, 0);
  if(st->port < 0)
    {
      CloseBlockFile(&st->bf);
      st->bf_open = 0;
      free_buffers(st);
      return st->port;
    }
  st->ioreq = CreateBlockFileIOReq(st->bf.fDevice, st->port);
  if((int32)st->ioreq < 0)
    {
      DeleteMsgPort(st->port);
      CloseBlockFile(&st->bf);
      st->bf_open = 0;
      st->port = -1;
      free_buffers(st);
      return (int32)st->ioreq;
    }
  return 0;
}

void
vx_stream_service(VxStream *st)
{
  if(st->wedged)
    return;
  finish_read(st);
  if(st->wedged)
    return;
  demux(st);
  if(st->wedged)
    return;
  issue_read(st);
}

int
vx_stream_next_frame(VxStream *st, const uint8 **payload,
                     uint32 *size, uint32 *frame_index)
{
  if(!st->frame_ready[st->slot_r])
    return 0;
  *payload     = st->frames[st->slot_r];
  *size        = st->frame_len[st->slot_r];
  *frame_index = st->frame_index[st->slot_r];
  return 1;
}

void
vx_stream_frame_consumed(VxStream *st)
{
  st->frame_ready[st->slot_r] = 0;
  st->slot_r++; if(st->slot_r == VX_FRAME_SLOTS) st->slot_r = 0;
}

uint32
vx_stream_audio_pull(VxStream *st, uint8 *dst, uint32 max,
                     uint32 *first_sample)
{
  uint32 avail = st->aud_head - st->aud_tail;
  uint32 n = (avail < max) ? avail : max;
  uint32 r, first;
  uint32 submitted;

  /* The audio folio requires whole words. Keep a stereo sample in the
     ring until the next chunk arrives; pad only the final sample. */
  n &= ~3u;
  submitted = n;
  if(n == 0 && avail == 2 && max >= 4 && st->eof
     && !st->read_pending && st->cur == st->fill)
    {
      n = 2;
      submitted = 4;
      dst[2] = dst[3] = 0;
    }

  if(n == 0)
    return 0;
  if(first_sample)
    *first_sample = st->aud_tail_sample;
  r     = st->aud_tail % VX_AUDRING_BYTES;
  first = VX_AUDRING_BYTES - r;
  if(first > n)
    first = n;
  memcpy(dst, st->aud + r, first);
  if(first < n)
    memcpy(dst + first, st->aud, n - first);
  st->aud_tail += n;
  st->aud_tail_sample += n >> 1;  /* SDX2 stereo: 2 B per per-channel sample */
  return submitted;
}

void
vx_stream_rewind(VxStream *st)
{
  if(st->read_pending)
    {
      if(WaitReadDoneBlockFile(st->ioreq) < 0)
        st->wedged = 1;
      st->read_pending = 0;
    }
  st->cur = 0; st->fill = 0;
  st->win_off = 0; st->next_read_off = 0;
  st->skip_remaining = 0;
  st->slot_w = 0; st->slot_r = 0;
  { int i; for(i = 0; i < VX_FRAME_SLOTS; i++) st->frame_ready[i] = 0; }
  st->aud_head = 0; st->aud_tail = 0; st->aud_tail_sample = 0;
  st->header_ok = 0;
  st->eof = 0;
  st->eof_said = 0;
  st->n_vfrm = 0; st->n_aud0 = 0; st->aud_ringfull = 0; st->slotfull = 0;
  st->audio_payload_bytes = 0;
  st->frames_delivered = 0;
}

void
vx_stream_close(VxStream *st)
{
  int s;

  if(st->bf_open)
    {
      if(st->read_pending)
        WaitReadDoneBlockFile(st->ioreq);
      if(st->ioreq >= 0)
        DeleteItem(st->ioreq);
      if(st->port >= 0)
        DeleteMsgPort(st->port);
      CloseBlockFile(&st->bf);
    }
  for(s = 0; s < VX_FRAME_SLOTS; s++)
    if(st->frames[s])
      FreeMem(st->frames[s], VX_FRAME_MAX);
  if(st->win) FreeMem(st->win, VX_WIN_BYTES);
  if(st->aud) FreeMem(st->aud, VX_AUDRING_BYTES);
}

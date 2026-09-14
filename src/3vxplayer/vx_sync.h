/* Exact 22050 Hz sample -> fps scheduling for 15000/20000/30000-over-1001
   fps, with no u64 math on ARM60.
   For each supported rate, 441441 samples correspond to exactly
   fps_num/50 video frames (300, 400, and 600 respectively), because
   22050*1001/fps_num * fps_num/50 = 22050*1001/50 = 441441. */
#ifndef VX_SYNC_H
#define VX_SYNC_H
#include "types.h"

#define VX_LATE_FRAMES 3u
#define VX_DECODE_BUDGET 2u
#define VX_SCAN_BUDGET 32u
#define VX_SYNC_WAIT 0
#define VX_SYNC_DROP 1
#define VX_SYNC_DECODE 2

static uint32
vx_due_frame(uint32 samples, uint32 fps_num)
{
  const uint32 period = 441441u;        /* samples per fps_num/50 frames */
  const uint32 frames_per_period = fps_num / 50u;
  return (samples / period) * frames_per_period
       + ((samples % period) * frames_per_period) / period;
}

/* A skipped delta invalidates the whole prediction chain. Do not resume
   until an independently decodable keyframe at/after the audio cursor.
   A future keyframe is held until due, never displayed early. */
static int
vx_sync_action(uint32 frame, uint32 due, int keyframe, int *recovering)
{
  if(!*recovering && due > frame && due - frame > VX_LATE_FRAMES)
    *recovering = 1;
  if(*recovering)
    {
      if(!keyframe || frame < due)
        return VX_SYNC_DROP;
      if(frame > due)
        return VX_SYNC_WAIT;
      *recovering = 0;
      return VX_SYNC_DECODE;
    }
  return frame > due ? VX_SYNC_WAIT : VX_SYNC_DECODE;
}
#endif

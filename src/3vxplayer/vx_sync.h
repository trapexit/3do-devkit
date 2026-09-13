/* Exact 22050 Hz sample -> 30000/1001 fps scheduling, with no u64 math.
   147147 samples correspond to exactly 200 video frames. */
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
vx_due_frame(uint32 samples)
{
  return (samples / 147147u) * 200u
       + ((samples % 147147u) * 200u) / 147147u;
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

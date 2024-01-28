#ifndef __ORBIT_H
#define __ORBIT_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: orbit.h,v 1.4 1994/11/11 06:53:58 gyld Exp $
**
******************************************************************************/

#include "debug3do.h"

#include "animutils.h"
#include "audio.h"
#include "celutils.h"
#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "string.h"
#include "umemory.h"

#include "controlpad.h"
#include "effectshandler.h"

#include "orbiterror.h"

#define VERSION "2.0"

#define VELOCITY 0x2000
#define FRAMEINCR 0x10000
#define RADIUS 0x500000 /* this is 80 << 16 bits left to be a frac 16 */
#define ACCELERATOR 0x14000
#define MAXVELOCITY 0x180000
#define BOTTOM 0xc00000 /* this is 64 << 16 */

#define XCenter 0xa00000 /* this is 160 << 16 */
#define YCenter 0x780000 /* this is 120 << 16 */

typedef struct _OrbitObj
{
  CCB *obj_CCB;   /* ccb to hold the cel with which to draw this object */
  ANIM *obj_ANIM; /* animation for this object */
  int32
      obj_Height; /* actual height of artwork, minus any black border pixels */
  frac16 obj_Border; /* size of the black border pixels */
  frac16 obj_yTop; /* used in calculation of new orbit... the highest point in
                      the orbit */
  frac16 obj_radius; /* radius for the current orbit */
  frac16 obj_theta; /* angle describing current position relative to the center
                       of the orbit */
  frac16 obj_velocity;  /* increment to the angle for each update */
  frac16 obj_frameIncr; /* increment to the animation frames */
  frac16 obj_centerX; /* center of the current orbit offset by the width of the
                         cel */
  frac16 obj_centerY; /* center of the current orbit offset by the height of
                         the cel */
  bool obj_Stopping;  /* state for the object:  its in its final orbit, coming
                         to a stop */
  bool obj_Stopped;   /* state for object:  its stopped */
  bool obj_WaitingToStop; /* state for object:  its in an orbit, waiting to get
                             to a position where it will be stopping */
  bool obj_Set; /* state for object:  the new orbit has been calculated */
} OrbitObj, *OrbitObjPtr;

Item gVramIOReq = -1, gVblIOReq = -1;
Item gOrbitErrorItem = -1;
pTMixerInfo gAudioCtx = NULL;
pTSampleInfo gClickSound, gCheerSound;
uint32 gCueSignal;
Item gCue;

static int32 Initialize (ScreenContext *sc, int32 nScreens);
static int32 SetObjCCB (OrbitObj *obj, int32 iObj);
static int32 LoadObj (OrbitObjPtr *pObj);
static void UnloadObj (OrbitObjPtr obj);
static void InitObj (OrbitObjPtr obj);
static bool UpdateObj (OrbitObjPtr obj);
static int32 LoadSounds (void);

#endif /* _ORBIT_H */

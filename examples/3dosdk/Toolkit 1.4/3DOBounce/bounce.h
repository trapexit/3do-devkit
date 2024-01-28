/*
        File:		bounce.h

        Contains:	header info for bounce.c

        Written by:	Greg Gorsiski

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	  4/5/93	JAY		removing /remote/ from
   pathnames (i.e. making path relative) <2>	 3/18/93	JAY
   change lists.h to list.h <1>	 3/18/93	JAY		first checked
   in

        To Do:
*/

#include "types.h"

#include "debug.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"

#ifndef __operamath_h
#include "operamath.h"
#endif

#include "Utils3DO.h"

#define VERSION "1.0"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define SEL_ENABLE_VAVG 1
#define SEL_ENABLE_HAVG 2
#define SEL_ENABLE_BOTH 3
#define SEL_DISABLE_BOTH 4

#define FADE_FRAMECOUNT 48

#define JOYCONTINUOUS                                                         \
  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB)

#define wallOnImg "BounceFolder/art/wallO.img"
#define wallOffImg "BounceFolder/art/wallF.img"
#define globePict "BounceFolder/art/globe.anim"
#define globeSPict "BounceFolder/art/globeS.cel"
#define tvPict "BounceFolder/art/tv.anim"
#define tvSPict "BounceFolder/art/tvS.anim"
#define cubePict "BounceFolder/art/cube.cel"
#define cubeSPict "BounceFolder/art/cubeS.cel"
#define ballPict "BounceFolder/art/ball.cel"
#define ballSPict "BounceFolder/art/ballS.cel"

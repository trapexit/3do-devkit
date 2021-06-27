/*
	File:		bounce.h

	Contains:	xxx put contents here xxx

	Written by:	Greg Gorsiski and Darren Gibbs

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	  4/5/93	JAY		remove /remote/ from filename (i.e. make path relative to
									initial working directory)
		 <1>	  4/3/93	JAY		first checked in

	To Do:
*/

#include "types.h"

//#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "string.h"
#include "stdlib.h"
#include "graphics.h"
#include "hardware.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"

#ifndef __operamath_h
#include "operamath.h"
#endif

#include "Utils3DO.h"

#define VERSION "1.0"

#define DISPLAY_WIDTH	384
#define DISPLAY_HEIGHT	288

/* These wall positions are for calculating audio panning 
   information based on bounce's used screen width...  */
#define LEFT_WALL_POS	5
#define RIGHT_WALL_POS	270
#define WINDOW_WIDTH	(RIGHT_WALL_POS - LEFT_WALL_POS)

#define SEL_ENABLE_VAVG   1
#define SEL_ENABLE_HAVG   2
#define SEL_ENABLE_BOTH   3
#define SEL_DISABLE_BOTH  4

#define FADE_FRAMECOUNT 48

/* Flags for passing info about room collisions */
#define 	BALL 		2 
#define 	TV 			4 
#define 	CUBE	 	8 
#define 	GLOBE	 	16 

#define 	FLOOR	 	32 
#define 	CEILING	 	64 
#define 	LEFT_WALL	128 
#define 	RIGHT_WALL	256 
#define 	FRONT_WALL	512 
#define 	BACK_WALL	1024 

/* Flags for passing info about object collisions */
#define 	BALL_TV_COLL 		2 
#define 	BALL_GLOBE_COLL 	4 
#define 	TV_CUBE_COLL 		8 
#define 	TV_GLOBE_COLL 		16
#define 	CUBE_GLOBE_COLL 	32

#define 	ABOVE 		64
#define 	BELOW	 	128
#define 	LEFT	 	256
#define 	RIGHT 		512


#define JOYCONTINUOUS  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB)


#define wallOnImg	"PALBounceFolder/art/wallO.img"
#define wallOffImg	"PALBounceFolder/art/wallF.img"
#define globePict	"PALBounceFolder/art/globe.anim"
#define globeSPict	"PALBounceFolder/art/globeS.cel"
#define tvPict		"PALBounceFolder/art/tv.anim"
#define tvSPict		"PALBounceFolder/art/tvS.anim"
#define cubePict	"PALBounceFolder/art/cube.cel"
#define cubeSPict	"PALBounceFolder/art/cubeS.cel"
#define ballPict	"PALBounceFolder/art/ball.cel"
#define ballSPict	"PALBounceFolder/art/ballS.cel"


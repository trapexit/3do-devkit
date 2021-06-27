/*
	File:		sprite.h

	Contains:	a header file used by 3DOOrbit

	Written by:	David Maynard

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 3/17/93	JAY		making changes for Cardinal build
		 <1>	 3/17/93	JAY		This is the 2B1 version being checked in for the Cardinal build
		 <0>	11/17/92	dsm		1.0 Release

	To Do:
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#ifndef CardinalChange
#include "list.h"
#else
#include "lists.h"
#endif
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "graphics.h"
#include "hardware.h"
#include "operamath.h"

#include "Form3DO.h"
#include "Parse3DO.h"
#include "Init3DO.h"
#include "Utils3DO.h"
typedef struct Sprite
	{
	struct Sprite *nxtSprite;
	long xcenter;
	long ycenter;
	long width;
	long height;
	CCB *ccbptr;
	frac16	xpos;
	frac16	ypos;
	frac16 xvel;
	frac16 yvel;
	frac16 zpos;
	frac16 zvel;
	frac16 tpos;
	frac16 tvel;
	Point corner[4];
	} Sprite;

#if DEBUG
#define DBUG(x)	{ kprintf x ; }
#else 
#define DBUG(x)	
#endif
extern Sprite * LoadSprite(char *name,  frac16 xvel, frac16 yvel, frac16 zvel, frac16 tvel);
extern int zscaling;
extern int rotating;
extern void DrawSprites(Item bitmapItem );
extern void MoveSprites( void);
extern Sprite *SpriteList;


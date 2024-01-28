#ifndef __ANIMUTILS_H
#define __ANIMUTILS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: animutils.h,v 1.3 1994/10/05 17:26:49 vertex Exp $
**
**  Lib3DO animation utility routines.
**
******************************************************************************/

#include "operamath.h"
#include "form3do.h"

/*----------------------------------------------------------------------------
 * datatypes
 *--------------------------------------------------------------------------*/

typedef struct AnimFrame {
	CCB 		*af_CCB;		/* Pointer to CCB for this frame */
	char		*af_PLUT;		/* Pointer to PLUT for this frame */
	char		*af_pix;		/* Pointer to pixels for this frame */
	int32		reserved;
} AnimFrame;

typedef struct ANIM {
	int32		num_Frames; /* greatest number of PDATs or CCBs in file */
	frac16		cur_Frame;	/* allows fractional values for smooth speed */
	int32		num_Alloced_Frames;
	AnimFrame	*pentries;
	void		*dataBuffer; /* for internal use by LoadAnim/UnloadAnim only! */
} ANIM;

#define CenterHotSpot		1
#define UpperLeftHotSpot	2
#define LowerLeftHotSpot	3
#define UpperRightHotSpot	4
#define LowerRightHotSpot	5

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/


#ifdef __cplusplus
  extern "C" {
#endif

ANIM *	LoadAnim(char *filename, uint32 memTypeBits);
void 	UnloadAnim(ANIM *anim);
ANIM *	ParseAnim(void *inBuf, int32 inBufSize, uint32 memTypeBits);

void	DrawAnimCel(ANIM *pAnim, Item bitmapItem, int32 xPos, int32 yPos, frac16 frameIncrement, int32 hotSpot);
CCB *	GetAnimCel(ANIM *pAnim, frac16 frameIncrement);

#ifdef __cplusplus
  }
#endif

#endif /* __ANIMUTILS_H */

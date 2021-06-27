/*****************************************************************************
 *	File:		AnimUtils.h
 *
 *	Contains:   Animation utility routines. 
 *
 *	Copyright:	(c) 1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History(most recent first):
 *	07/25/94  Ian	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#pragma include_only_once

#ifndef ANIMUTILS_H
#define ANIMUTILS_H

#include "operamath.h"
#include "Form3DO.h"

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

#endif

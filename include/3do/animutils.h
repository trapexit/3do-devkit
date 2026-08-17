#pragma once

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

#include "extern_c.h"

#include "operamath.h"
#include "form3do.h"

/*----------------------------------------------------------------------------
 * datatypes
 *--------------------------------------------------------------------------*/

typedef struct AnimFrame
{
  CCB 	*af_CCB;		/* Pointer to CCB for this frame */
  char	*af_PLUT;		/* Pointer to PLUT for this frame */
  char	*af_pix;		/* Pointer to pixels for this frame */
  s32  reserved;
} AnimFrame;

typedef struct ANIM
{
  s32      num_Frames;        /* greatest number of PDATs or CCBs in file */
  frac16     cur_Frame;         /* allows fractional values for smooth speed */
  s32      num_Alloced_Frames;
  AnimFrame *pentries;
  void      *dataBuffer;        /* for internal use by LoadAnim/UnloadAnim only! */
} ANIM;

#define CenterHotSpot		1
#define UpperLeftHotSpot	2
#define LowerLeftHotSpot	3
#define UpperRightHotSpot	4
#define LowerRightHotSpot	5

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

EXTERN_C_BEGIN

ANIM *LoadAnim(char *filename, u32 memTypeBits);
void  UnloadAnim(ANIM *anim);
ANIM *ParseAnim(void *inBuf, s32 inBufSize, u32 memTypeBits);
void  DrawAnimCel(ANIM *pAnim, Item bitmapItem, s32 xPos, s32 yPos, frac16 frameIncrement, s32 hotSpot);
CCB  *GetAnimCel(ANIM *pAnim, frac16 frameIncrement);

EXTERN_C_END

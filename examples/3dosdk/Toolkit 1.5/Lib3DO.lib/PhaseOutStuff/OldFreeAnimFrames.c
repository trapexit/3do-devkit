/*****************************************************************************
 *	File:			FreeAnimFrames.c
 *
 *	Contains:		Free memory acquired by OldLoadAnim().
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  xxx 	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Parse3DO.h"
#include "mem.h"

void
OldFreeAnimFrames (ANIM *pAnim)
{
  int32 nFrames;
  AnimFrame *frames;

  frames = pAnim->pentries;
  nFrames = pAnim->num_Alloced_Frames;
  FREEMEM (frames, nFrames * sizeof (AnimFrame));
  pAnim->pentries = NULL;
  pAnim->num_Alloced_Frames = 0;
}

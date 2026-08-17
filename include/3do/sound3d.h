#pragma once

/****************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: sound3d.h,v 1.11 1994/09/27 08:56:17 phil Exp $
 **
 **  3D Sound
 **
 **  By: Phil Burk
 **
 ****************************************************************************/

#include "extern_c.h"
#include "types.h"
#include "operamath.h"



#define NOMINAL3DFREQ (0x8000)

typedef struct PolarPosition4D
{
  s32  pp4d_Radius;
  frac16 pp4d_Theta;
  frac16 pp4d_Phi;
  s32  pp4d_Time;
} PolarPosition4D;

typedef struct Sound3D
{
  Item s3d_InsItem;                  /* Sampler3D instrument. */
  /* Knobs used to control filters. */
  Item s3d_LeftAlphaKnob;
  Item s3d_RightAlphaKnob;
  Item s3d_LeftBetaKnob;
  Item s3d_RightBetaKnob;
  Item s3d_LeftFeedKnob;
  Item s3d_RightFeedKnob;
  Item s3d_LeftFreqKnob;
  Item s3d_RightFreqKnob;
  Item s3d_LeftAmplitudeKnob;
  Item s3d_RightAmplitudeKnob;
  Item s3d_LeftCountProbe;            /* PRobe of left sample count. */
  Item s3d_RightCountProbe;           /* Probe of right sample count. */
  u32 s3d_InitialFrame;
  u32 s3d_InitialLeft;
  u32 s3d_InitialRight;
  s32 s3d_RadialVelocity;
  frac16 s3d_ThetaVelocity;
  frac16 s3d_PhiVelocity;
  s32  s3d_MinRadius;             /* at which loudness reaches maximum */
} Sound3D;

typedef struct
{
  s32 erp_Alpha;
  s32 erp_Beta;
  s32 erp_Feed;
  s32 erp_Amplitude;
} EarParams;

typedef struct
{
  EarParams bep_LeftEar;
  EarParams bep_RightEar;
} BothEarParams;

/* This corresponds to E in the equations. */
#define DISTANCE_TO_EAR (20)

EXTERN_C_BEGIN

Sound3D *Open3DSound( void );
/* Allocate in this routine in case size changes. */

s32 Close3DSound( Sound3D *Snd3D );

s32 Load3DSound( Sound3D *Snd3D );
/*	Loads a 3D sound instrument, allocates a control structure,
**	and passes its address back.
*/

s32 Unload3DSound( Sound3D *Snd3D );

s32 Attach3DSoundSample( Sound3D *Snd3D, Item Sample);
/* Attach sample to be read by 3D sampler. For piped through sound,
** this could be a DelayLine.
*/

s32 Start3DSound( Sound3D *Snd3D, PolarPosition4D *Pos4D , TagArg *TagList );
/* Starts a 3D sound playing at the given polar coordinates. */

s32 Move3DSound( Sound3D *Snd3D, PolarPosition4D *Start4D, PolarPosition4D *End4D );
/* Move the sound to the given position to arrive at the given time
** in 3D Ticks.
*/

s32 Set3DSoundRates ( Sound3D *Snd3D, frac16 LeftRate, frac16 RightRate );

s32 Set3DSoundFilters ( Sound3D *Snd3D, BothEarParams *BEP );

u16 Get3DSoundTime( void );

s32 Calc3DSoundRates( PolarPosition4D *Start4D, PolarPosition4D *End4D,
                        frac16 *LeftRate, frac16 *RightRate );

s32 Calc3DSoundEar ( PolarPosition4D *Pos4D, s32 MinRadius, EarParams *ERP );

s32 Calc3DSoundFilters( PolarPosition4D *Pos4D, s32 MinRadius, BothEarParams *BEP);

s32 Get3DSoundPos( Sound3D *Snd3D, PolarPosition4D *Pos4D );
/* Read frame counts and calculate current position and time. */

s32 Get3DSoundPhaseDelay( Sound3D *Snd3D, s32 *PhaseDelay );
s32 Get3DSoundRadius( Sound3D *Snd3D, s32 *RadiusPtr, s32 *PhaseDelay);

s32 Stop3DSound( Sound3D *Snd3D, TagArg *TagList );

EXTERN_C_END

#pragma force_top_level
#pragma include_only_once


/****************************************************************
**
** 3D Sound Include File
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/****************************************************************
** 930125 PLB Changed s3d_MaxRadius to s3d_MinRadius
****************************************************************/
extern int32 IfDebug3D;

#define NOMINAL3DFREQ (0x8000)

typedef struct
{
	int32  pp4d_Radius;
	frac16 pp4d_Theta;
	frac16 pp4d_Phi;
	int32  pp4d_Time;
} PolarPosition4D;

typedef struct
{
	Item s3d_InsItem;
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
	int32 s3d_EO_LeftCount;
	int32 s3d_EO_RightCount;
	uint32 s3d_InitialFrame;
	uint32 s3d_InitialLeft;
	uint32 s3d_InitialRight;
	int32 s3d_RadialVelocity;
	frac16 s3d_ThetaVelocity;
	frac16 s3d_PhiVelocity;
	int32  s3d_MinRadius;    /* at which loudness drops to zero */
} Sound3D;

typedef struct
{
	int32 erp_Alpha;
	int32 erp_Beta;
	int32 erp_Feed;
	int32 erp_Amplitude;
} EarParams;

typedef struct
{
	EarParams bep_LeftEar;
	EarParams bep_RightEar;
} BothEarParams;

/* This corresponds to E in the equations. */
#define DISTANCE_TO_EAR (20)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Sound3D *Open3DSound( void );
/* Allocate in this routine in case size changes. */

int32 Close3DSound( Sound3D *Snd3D );

int32 Load3DSound( Sound3D *Snd3D );
/*	Loads a 3D sound instrument, allocates a control structure,
**	and passes its address back.
*/

int32 Start3DSound( Sound3D *Snd3D, PolarPosition4D *Pos4D , TagArg *TagList );
/* Starts a 3D sound playing at the given polar coordinates. */

int32 Move3DSound( Sound3D *Snd3D, PolarPosition4D *Start4D, PolarPosition4D *End4D );
/* Move the sound to the given position to arrive at the given time
** in 3D Ticks.
*/

int32 Set3DSoundRates ( Sound3D *Snd3D, frac16 LeftRate, frac16 RightRate );

int32 Set3DSoundFilters ( Sound3D *Snd3D, BothEarParams *BEP );

uint16 Get3DSoundTime( void );

int32 Calc3DSoundRates( PolarPosition4D *Start4D, PolarPosition4D *End4D, 
		frac16 *LeftRate, frac16 *RightRate );
		
int32 Calc3DSoundEar ( PolarPosition4D *Pos4D, int32 MinRadius, EarParams *ERP );

int32 Calc3DSoundFilters( PolarPosition4D *Pos4D, int32 MinRadius, BothEarParams *BEP);

int32 Get3DSoundPos( Sound3D *Snd3D, PolarPosition4D *Pos4D );
/* Read frame counts and calculate current position and time. */

int32 Get3DSoundPhaseDelay( Sound3D *Snd3D, int32 *PhaseDelay );
int32 Get3DSoundRadius( Sound3D *Snd3D, int32 *RadiusPtr, int32 *PhaseDelay);

int32 Stop3DSound( Sound3D *Snd3D, TagArg *TagList );

#ifdef __cplusplus
}
#endif /* __cplusplus */

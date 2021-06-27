/*
	File:		bounce_sound.h

	Contains:	3DO bounce demo

	Written by:	Darren Gibbs

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):
		 <3> 	3/10/93		MPH		Changed for PAL version.
		 <2>	  4/5/93	JAY		removing /remote/ from filename (i.e. making pathname relative
									to initial working directory
		 <1>	  4/3/93	JAY		first checked in

	To Do:
*/

#include "filefunctions.h"
#include "graphics.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "Audio.h"


#define 	MAXVOICES 	8
#define 	NUMVOICES 	8
#define 	NUMCHANNELS 8
#define 	NUMSAMPLERS	8
#define 	MAXAMPLITUDE 0x7FFF
#define 	MAX_PITCH 	60
#define 	PITCH_RANGE 40

/* These are used as indices to arrays */
#define 	BALL_TV		0
#define 	BALL_GLOBE	1
#define 	TV_CUBE		2
#define 	CUBE_GLOBE	3
#define 	BALL_FLOOR	4
#define 	TV_FLOOR	5
#define 	CUBE_FLOOR	6
#define 	GLOBE_FLOOR	7

/* Shorter... */
#define 	BALL_TV_SND		"BounceFolder/sound/Bird.aiff"
#define 	BALL_GLOBE_SND	"BounceFolder/sound/3DO.aiff"
#define 	TV_CUBE_SND		"BounceFolder/sound/Interactive.aiff"
#define 	CUBE_GLOBE_SND	"BounceFolder/sound/MultiPlayer.aiff"
#define 	BALL_FLOOR_SND	"BounceFolder/sound/BallBnce.aiff"
#define 	TV_FLOOR_SND	"BounceFolder/sound/TvBnce.aiff"
#define 	CUBE_FLOOR_SND	"BounceFolder/sound/CubeBnce.aiff"
#define 	GLOBE_FLOOR_SND	"BounceFolder/sound/GlobeBnce.aiff"

/* These values seem high based on the value of MAXAMPLITUDE, but
   center panning splits the values between left and right outputs. 
*/
#define		BALL_TV_GAIN 		0x4000
#define		BALL_GLOBE_GAIN		0x4000
#define		TV_CUBE_GAIN		0x4000 
#define		CUBE_GLOBE_GAIN		0x4000 
#define		BALL_FLOOR_GAIN		0x6000 
#define		TV_FLOOR_GAIN		0x2000
#define		CUBE_FLOOR_GAIN		0x2000
#define		GLOBE_FLOOR_GAIN	0x3500

/* Macro to simplify error checking. */
#define CHECKRESULT(val,routine,string) \
	if (val != 0) \
	{ \
		Result = val; \
		kprintf("\n\nFATAL ERROR!!!\n "); \
		kprintf("------> in %s: %s\n",routine,string); \
		goto error; \
	}
	
/* Macro to simplify error checking. */
#define CHECKITEM(val,routine,string) \
	if (val <= 0) \
	{ \
		TempItem = val; \
		kprintf("\n\nFATAL ERROR!!!\n "); \
		kprintf("------> in %s: %s\n",routine,string); \
		goto error; \
	}
	
int32 InitBounceSound(void);
void TermBounceSound(void);
void DoObjectCollisionSound(uint32 IAFlags);
void DoRoomCollisionSound(uint32 IAFlags);
void PanMixerChannel( int32 ChannelNumber, int32 MaxAmp, int32 Pan);
int32 YPositionToPitch(int32 YPosition);



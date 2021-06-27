
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bounce_sound.h,v 1.7 1994/11/23 18:08:45 vertex Exp $
**
******************************************************************************/

#include "types.h"

#define     MAXVOICES   8
#define     NUMVOICES   8
#define     NUMCHANNELS 8
#define     NUMSAMPLERS 8
#define     MAXAMPLITUDE MAXDSPAMPLITUDE
#define     MAX_PITCH   60
#define     PITCH_RANGE 40

/* These are used as indices to arrays */
#define     BALL_TV     0
#define     BALL_GLOBE  1
#define     TV_CUBE     2
#define     CUBE_GLOBE  3
#define     BALL_FLOOR  4
#define     TV_FLOOR    5
#define     CUBE_FLOOR  6
#define     GLOBE_FLOOR 7

#define		SOUND_FOLDER		"sound/"

#define     BALL_TV_SND     "bird.aiff"
#define     BALL_GLOBE_SND  "3do.aiff"
#define     TV_CUBE_SND     "interactive.aiff"
#define     CUBE_GLOBE_SND  "multiplayer.aiff"
#define     BALL_FLOOR_SND  "ballbnce.aiff"
#define     TV_FLOOR_SND    "tvbnce.aiff"
#define     CUBE_FLOOR_SND  "cubebnce.aiff"
#define     GLOBE_FLOOR_SND "globebnce.aiff"

/* These values seem high based on the value of MAXAMPLITUDE, but
   center panning splits the values between left and right outputs.
*/
#define     BALL_TV_GAIN        0x4000
#define     BALL_GLOBE_GAIN     0x4000
#define     TV_CUBE_GAIN        0x4000
#define     CUBE_GLOBE_GAIN     0x4000
#define     BALL_FLOOR_GAIN     0x6000
#define     TV_FLOOR_GAIN       0x2000
#define     CUBE_FLOOR_GAIN     0x2000
#define     GLOBE_FLOOR_GAIN    0x3500

/* These wall positions are for calculating audio panning
   information based on bounce's used screen width...  */
#define LEFT_WALL_POS   5
#define RIGHT_WALL_POS  270
#define WINDOW_WIDTH    (RIGHT_WALL_POS - LEFT_WALL_POS)

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


int32 InitBounceSound(void);
void KillBounceSound(void);
void DoObjectCollisionSound(uint32 IAFlags);
void DoRoomCollisionSound(uint32 IAFlags);
void PanMixerChannel( int32 ChannelNumber, int32 MaxAmp, int32 Pan);
int32 YPositionToPitch(int32 YPosition);




/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bounce_sound.c,v 1.10 1994/12/26 19:31:47 ceckhaus Exp $
**
******************************************************************************/

/*
	bounce_sound - sound utilities for bounce

	Contains explicit references to globals in bounce.c

	The data files are:

	- bouncefolder/sound/bird.aiff          ball-tv collision sound
	- bouncefolder/sound/3do.aiff           ball-globe collision sound
	- bouncefolder/sound/interactive.aiff   tv-cube collision sound
	- bouncefolder/sound/multiplayer.aiff   cube-globe collsion sound
	- bouncefolder/sound/ballbnce.aiff      ball bounce sound
	- bouncefolder/sound/tvbnce.aiff        tv bounce sound
	- bouncefolder/sound/cubebnce.aiff      cube bounce sound
	- bouncefolder/sound/globebnce.aiff     globe bounce sound

	If PAL video is being displayed, the folder palbouncefolder is used
	instead of bouncefolder.
*/


#include "types.h"
#include "audio.h"
#include "graphics.h"
#include "debug3do.h"
#include "effectshandler.h"

#include "bounce.h"
#include "bounce_sound.h"
#include "getvideoinfo.h"

extern int32 gGlobeXPos;
extern int32 gGlobeYPos;
extern int32 gCubeXPos;
extern int32 gCubeYPos;
extern int32 gTvXPos;
extern int32 gTvYPos;
extern int32 gBallXPos;
extern int32 gBallYPos;

extern int32 gDisplayType;

int32 gDisplayHeight;

pTSampleInfo Effect[NUMSAMPLERS];


/* Define Tags for StartInstrument */
TagArg startTags[] =
    {
        { AF_TAG_PITCH, 0},
        { 0, 0 }
    };

pTMixerInfo	gMixerInfo = NULL;

void GetSoundFolder( char *folderPath )
/* Get the path to the sound data folder */
{
	if ( PAL_DISPLAY( gDisplayType ) )
		strcpy( folderPath, PAL_FOLDER );
	else
		strcpy( folderPath, NTSC_FOLDER );
	strcat( folderPath, SOUND_FOLDER );
}

void GetSoundFilename( char *filename, char *fullPathname )
/* Get the full path to a sound file */
{
	GetSoundFolder( fullPathname );
	strcat( fullPathname, filename );
}

int32 InitBounceSound(void)
/* Initialize the mixer and the program's sounds */
{
    int32   retValue = -1;
	char	filename[128];

	/*
		We need the display height in order to map from y-position
		to pitch.
	*/
	gDisplayHeight = GetScreenHeight( gDisplayType );
	if ( gDisplayHeight < 0 )
		goto DONE;

	if ( ehNewMixerInfo(&gMixerInfo, NUMSAMPLERS, "mixer8x2.dsp") < 0 )
		goto DONE;

/* Load up Samples  */
	GetSoundFilename( BALL_TV_SND, filename );
	if ( ehLoadSoundEffect(&Effect[BALL_TV], gMixerInfo, filename, BALL_TV) < 0 )
		goto DONE;

	GetSoundFilename( BALL_GLOBE_SND, filename );
	if ( ehLoadSoundEffect(&Effect[BALL_GLOBE], gMixerInfo, filename, BALL_GLOBE) < 0 )
		goto DONE;

	GetSoundFilename( TV_CUBE_SND, filename );
	if ( ehLoadSoundEffect(&Effect[TV_CUBE], gMixerInfo, filename, TV_CUBE) < 0 )
		goto DONE;

	GetSoundFilename( CUBE_GLOBE_SND, filename );
	if ( ehLoadSoundEffect(&Effect[CUBE_GLOBE], gMixerInfo, filename, CUBE_GLOBE) < 0 )
		goto DONE;

	GetSoundFilename( BALL_FLOOR_SND, filename );
	if ( ehLoadSoundEffect(&Effect[BALL_FLOOR], gMixerInfo, filename, BALL_FLOOR) < 0 )
		goto DONE;

	GetSoundFilename( TV_FLOOR_SND, filename );
	if ( ehLoadSoundEffect(&Effect[TV_FLOOR], gMixerInfo, filename, TV_FLOOR) < 0 )
		goto DONE;

	GetSoundFilename( CUBE_FLOOR_SND, filename );
	if ( ehLoadSoundEffect(&Effect[CUBE_FLOOR], gMixerInfo, filename, CUBE_FLOOR) < 0 )
		goto DONE;

	GetSoundFilename( GLOBE_FLOOR_SND, filename );
	if ( ehLoadSoundEffect(&Effect[GLOBE_FLOOR], gMixerInfo, filename, GLOBE_FLOOR) < 0 )
		goto DONE;

	ehSetChannelLevels(gMixerInfo, Effect[BALL_TV]->si_LeftGainKnob, Effect[BALL_TV]->si_RightGainKnob,
			BALL_TV_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[BALL_GLOBE]->si_LeftGainKnob, Effect[BALL_GLOBE]->si_RightGainKnob,
			BALL_GLOBE_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[TV_CUBE]->si_LeftGainKnob, Effect[TV_CUBE]->si_RightGainKnob,
			TV_CUBE_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[CUBE_GLOBE]->si_LeftGainKnob, Effect[CUBE_GLOBE]->si_RightGainKnob,
			CUBE_GLOBE_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[BALL_FLOOR]->si_LeftGainKnob, Effect[BALL_FLOOR]->si_RightGainKnob,
			BALL_FLOOR_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[TV_FLOOR]->si_LeftGainKnob, Effect[TV_FLOOR]->si_RightGainKnob,
			TV_FLOOR_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[CUBE_FLOOR]->si_LeftGainKnob, Effect[CUBE_FLOOR]->si_RightGainKnob,
			CUBE_FLOOR_GAIN, kEqualBalance);
	ehSetChannelLevels(gMixerInfo, Effect[GLOBE_FLOOR]->si_LeftGainKnob, Effect[GLOBE_FLOOR]->si_RightGainKnob,
			GLOBE_FLOOR_GAIN, kEqualBalance);

	retValue = 0;

DONE:
    return retValue;
}

void DoObjectCollisionSound(uint32 IAFlags)
/* Play the appropriate sound for the collision of two objects. */
{
    if (BALL_TV_COLL & IAFlags)
    {
        PanMixerChannel(BALL_TV, BALL_TV_GAIN, gBallXPos);
        startTags[0].ta_Arg = (int32 *) YPositionToPitch(gBallYPos);
        StartInstrument(Effect[BALL_TV]->si_Player, &startTags[0]);
        ReleaseInstrument(Effect[BALL_TV]->si_Player, NULL);
        goto DONE;
    }


    if (BALL_GLOBE_COLL & IAFlags)
    {
        PanMixerChannel(BALL_GLOBE, BALL_GLOBE_GAIN, gGlobeXPos);
        startTags[0].ta_Arg = (int32 *) YPositionToPitch(gGlobeYPos);
        StartInstrument(Effect[BALL_GLOBE]->si_Player, &startTags[0]);
        ReleaseInstrument(Effect[BALL_GLOBE]->si_Player, NULL);
        goto DONE;
    }


    if (TV_CUBE_COLL & IAFlags)
    {
        PanMixerChannel(TV_CUBE, TV_CUBE_GAIN, gTvXPos);
        startTags[0].ta_Arg = (int32 *) YPositionToPitch(gTvYPos);
        StartInstrument(Effect[TV_CUBE]->si_Player, &startTags[0]);
        ReleaseInstrument(Effect[TV_CUBE]->si_Player, NULL);
        goto DONE;
    }


    if (CUBE_GLOBE_COLL & IAFlags)
    {
        PanMixerChannel(CUBE_GLOBE, CUBE_GLOBE_GAIN, gCubeXPos);
        startTags[0].ta_Arg = (int32 *) YPositionToPitch(gCubeYPos);
        StartInstrument(Effect[CUBE_GLOBE]->si_Player, &startTags[0]);
        ReleaseInstrument(Effect[CUBE_GLOBE]->si_Player, NULL);
        goto DONE;
    }

DONE:
    return;
}

void DoRoomCollisionSound(uint32 IAFlags)
/*
	Play the appropriate sound for the collision of an object
	and the floor.
*/
{
    if ((BALL | FLOOR) == (IAFlags) )
    {
        PanMixerChannel(BALL_FLOOR, BALL_FLOOR_GAIN, gBallXPos);
        StartInstrument(Effect[BALL_FLOOR]->si_Player, NULL);
        ReleaseInstrument(Effect[BALL_FLOOR]->si_Player, NULL);
        goto DONE;
    }
    if ((TV | FLOOR) == (IAFlags) )
    {
        PanMixerChannel(TV_FLOOR, TV_FLOOR_GAIN, gTvXPos);
        StartInstrument(Effect[TV_FLOOR]->si_Player, NULL);
        ReleaseInstrument(Effect[TV_FLOOR]->si_Player, NULL);
        goto DONE;
    }
    if ((CUBE | FLOOR) == (IAFlags) )
    {
        PanMixerChannel(CUBE_FLOOR, CUBE_FLOOR_GAIN, gCubeXPos);
        StartInstrument(Effect[CUBE_FLOOR]->si_Player, NULL);
        ReleaseInstrument(Effect[CUBE_FLOOR]->si_Player, NULL);
        goto DONE;
    }
    if ((GLOBE | FLOOR) == (IAFlags) )
    {
        PanMixerChannel(GLOBE_FLOOR, GLOBE_FLOOR_GAIN, gGlobeXPos);
        StartInstrument(Effect[GLOBE_FLOOR]->si_Player, NULL);
        ReleaseInstrument(Effect[GLOBE_FLOOR]->si_Player, NULL);
        goto DONE;
    }

DONE:
    return;
}

void PanMixerChannel(int32 ChannelNumber, int32 MaxAmp, int32 xPos)
/*
	Set the left and right gains for the sound in the specified channel
	according to the object's x-position.
*/
{
    int32 RightGain;
	int32 LeftGain;

    RightGain = ((MaxAmp * (xPos - LEFT_WALL_POS) ) / WINDOW_WIDTH);
    LeftGain = ((MaxAmp * (WINDOW_WIDTH - (xPos - LEFT_WALL_POS) )) / WINDOW_WIDTH);

    TweakKnob(Effect[ChannelNumber]->si_LeftGainKnob, LeftGain);
    TweakKnob(Effect[ChannelNumber]->si_RightGainKnob, RightGain);

}

int32 YPositionToPitch(int32 yPosition)
/*
	Map the specified y-position to a pitch; the higher the object (i.e., the
	lower the yPosition), the higher the pitch.
*/
{
    return ( (((gDisplayHeight - yPosition) * 10 / 100) + 50) );
}

void KillBounceSound ( void )
/*
	Dispose of the mixer and all of the sounds.
*/
{
    int32   samplerIndex;

    for (samplerIndex = 0; samplerIndex < NUMSAMPLERS; samplerIndex++)
    {
		ehDisposeSampleInfo( Effect[samplerIndex] );
    }

	ehDisposeMixerInfo( gMixerInfo );
}

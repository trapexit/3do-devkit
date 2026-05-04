/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
 *reserved.
 **  This material contains confidential information that is the property of The
 *3DO Company.
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

#define DEBUG 1

#include "audio.h"
#include "debug3do.h"
#include "effectshandler.h"
#include "graphics.h"
#include "operror.h"
#include "types.h"

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
TagArg startTags[] = { { AF_TAG_PITCH, 0 }, { 0, 0 } };

pTMixerInfo gMixerInfo = NULL;

static void
PrintInitBounceSoundError(const char *call, const char *detail, Err err)
{
  char errString[256];

  GetSysErr(errString, sizeof(errString), err);

  if(detail)
    printf("InitBounceSound: %s failed for %s: 0x%lx (%s)\n",
           call,
           detail,
           err,
           errString);
  else
    printf("InitBounceSound: %s failed: 0x%lx (%s)\n", call, err, errString);
}

static
Err
CheckAudioErr(const char *call, const char *detail, Err err)
{
  if(err < 0)
    PrintInitBounceSoundError(call, detail, err);

  return err;
}

void
GetSoundFolder(char *folderPath)
/* Get the path to the sound data folder */
{
  if(PAL_DISPLAY(gDisplayType))
    strcpy(folderPath, PAL_FOLDER);
  else
    strcpy(folderPath, NTSC_FOLDER);
  strcat(folderPath, SOUND_FOLDER);
}

void
GetSoundFilename(char *filename, char *fullPathname)
/* Get the full path to a sound file */
{
  GetSoundFolder(fullPathname);
  strcat(fullPathname, filename);
}

int32
InitBounceSound(void)
/* Initialize the mixer and the program's sounds */
{
  Err err;
  int32 retValue = -1;
  char filename[128];

  /*
    We need the display height in order to map from y-position
    to pitch.
  */
  gDisplayHeight = GetScreenHeight(gDisplayType);
  if(gDisplayHeight < 0)
    {
      PrintInitBounceSoundError("GetScreenHeight", NULL, gDisplayHeight);
      goto DONE;
    }

  err = ehNewMixerInfo(&gMixerInfo, NUMSAMPLERS, "mixer8x2.dsp");
  if(err < 0)
    {
      PrintInitBounceSoundError("ehNewMixerInfo", "mixer8x2.dsp", err);
      goto DONE;
    }

  /* Load up Samples  */
  GetSoundFilename(BALL_TV_SND, filename);
  err = ehLoadSoundEffect(&Effect[BALL_TV], gMixerInfo, filename, BALL_TV);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(BALL_GLOBE_SND, filename);
  err = ehLoadSoundEffect(
                          &Effect[BALL_GLOBE], gMixerInfo, filename, BALL_GLOBE);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(TV_CUBE_SND, filename);
  err = ehLoadSoundEffect(&Effect[TV_CUBE], gMixerInfo, filename, TV_CUBE);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(CUBE_GLOBE_SND, filename);
  err = ehLoadSoundEffect(
                          &Effect[CUBE_GLOBE], gMixerInfo, filename, CUBE_GLOBE);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(BALL_FLOOR_SND, filename);
  err = ehLoadSoundEffect(
                          &Effect[BALL_FLOOR], gMixerInfo, filename, BALL_FLOOR);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(TV_FLOOR_SND, filename);
  err = ehLoadSoundEffect(&Effect[TV_FLOOR], gMixerInfo, filename, TV_FLOOR);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(CUBE_FLOOR_SND, filename);
  err = ehLoadSoundEffect(
                          &Effect[CUBE_FLOOR], gMixerInfo, filename, CUBE_FLOOR);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  GetSoundFilename(GLOBE_FLOOR_SND, filename);
  err = ehLoadSoundEffect(&Effect[GLOBE_FLOOR], gMixerInfo, filename, GLOBE_FLOOR);
  if(err < 0)
    {
      PrintInitBounceSoundError("ehLoadSoundEffect", filename, err);
      goto DONE;
    }

  ehSetChannelLevels(gMixerInfo,
                     Effect[BALL_TV]->si_LeftGainKnob,
                     Effect[BALL_TV]->si_RightGainKnob,
                     BALL_TV_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[BALL_GLOBE]->si_LeftGainKnob,
                     Effect[BALL_GLOBE]->si_RightGainKnob,
                     BALL_GLOBE_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[TV_CUBE]->si_LeftGainKnob,
                     Effect[TV_CUBE]->si_RightGainKnob,
                     TV_CUBE_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[CUBE_GLOBE]->si_LeftGainKnob,
                     Effect[CUBE_GLOBE]->si_RightGainKnob,
                     CUBE_GLOBE_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[BALL_FLOOR]->si_LeftGainKnob,
                     Effect[BALL_FLOOR]->si_RightGainKnob,
                     BALL_FLOOR_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[TV_FLOOR]->si_LeftGainKnob,
                     Effect[TV_FLOOR]->si_RightGainKnob,
                     TV_FLOOR_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[CUBE_FLOOR]->si_LeftGainKnob,
                     Effect[CUBE_FLOOR]->si_RightGainKnob,
                     CUBE_FLOOR_GAIN,
                     kEqualBalance);
  ehSetChannelLevels(gMixerInfo,
                     Effect[GLOBE_FLOOR]->si_LeftGainKnob,
                     Effect[GLOBE_FLOOR]->si_RightGainKnob,
                     GLOBE_FLOOR_GAIN,
                     kEqualBalance);

  retValue = 0;

 DONE:
  return retValue;
}

void
DoObjectCollisionSound(uint32 IAFlags)
/* Play the appropriate sound for the collision of two objects. */
{
  if(BALL_TV_COLL & IAFlags)
    {
      PanMixerChannel(BALL_TV, BALL_TV_GAIN, gBallXPos);
      startTags[0].ta_Arg = (int32 *)YPositionToPitch(gBallYPos);
      CheckAudioErr("StartInstrument",
                    "ball-tv collision",
                    StartInstrument(Effect[BALL_TV]->si_Player, &startTags[0]));
      CheckAudioErr("ReleaseInstrument",
                    "ball-tv collision",
                    ReleaseInstrument(Effect[BALL_TV]->si_Player, NULL));
      goto DONE;
    }

  if(BALL_GLOBE_COLL & IAFlags)
    {
      PanMixerChannel(BALL_GLOBE, BALL_GLOBE_GAIN, gGlobeXPos);
      startTags[0].ta_Arg = (int32 *)YPositionToPitch(gGlobeYPos);
      CheckAudioErr(
                    "StartInstrument",
                    "ball-globe collision",
                    StartInstrument(Effect[BALL_GLOBE]->si_Player, &startTags[0]));
      CheckAudioErr("ReleaseInstrument",
                    "ball-globe collision",
                    ReleaseInstrument(Effect[BALL_GLOBE]->si_Player, NULL));
      goto DONE;
    }

  if(TV_CUBE_COLL & IAFlags)
    {
      PanMixerChannel(TV_CUBE, TV_CUBE_GAIN, gTvXPos);
      startTags[0].ta_Arg = (int32 *)YPositionToPitch(gTvYPos);
      CheckAudioErr("StartInstrument",
                    "tv-cube collision",
                    StartInstrument(Effect[TV_CUBE]->si_Player, &startTags[0]));
      CheckAudioErr("ReleaseInstrument",
                    "tv-cube collision",
                    ReleaseInstrument(Effect[TV_CUBE]->si_Player, NULL));
      goto DONE;
    }

  if(CUBE_GLOBE_COLL & IAFlags)
    {
      PanMixerChannel(CUBE_GLOBE, CUBE_GLOBE_GAIN, gCubeXPos);
      startTags[0].ta_Arg = (int32 *)YPositionToPitch(gCubeYPos);
      CheckAudioErr(
                    "StartInstrument",
                    "cube-globe collision",
                    StartInstrument(Effect[CUBE_GLOBE]->si_Player, &startTags[0]));
      CheckAudioErr("ReleaseInstrument",
                    "cube-globe collision",
                    ReleaseInstrument(Effect[CUBE_GLOBE]->si_Player, NULL));
      goto DONE;
    }

 DONE:
  return;
}

void
DoRoomCollisionSound(uint32 IAFlags)
/*
  Play the appropriate sound for the collision of an object
  and the floor.
*/
{
  if((BALL | FLOOR) == (IAFlags))
    {
      PanMixerChannel(BALL_FLOOR, BALL_FLOOR_GAIN, gBallXPos);
      CheckAudioErr("StartInstrument",
                    "ball-floor collision",
                    StartInstrument(Effect[BALL_FLOOR]->si_Player, NULL));
      CheckAudioErr("ReleaseInstrument",
                    "ball-floor collision",
                    ReleaseInstrument(Effect[BALL_FLOOR]->si_Player, NULL));
      goto DONE;
    }
  if((TV | FLOOR) == (IAFlags))
    {
      PanMixerChannel(TV_FLOOR, TV_FLOOR_GAIN, gTvXPos);
      CheckAudioErr("StartInstrument",
                    "tv-floor collision",
                    StartInstrument(Effect[TV_FLOOR]->si_Player, NULL));
      CheckAudioErr("ReleaseInstrument",
                    "tv-floor collision",
                    ReleaseInstrument(Effect[TV_FLOOR]->si_Player, NULL));
      goto DONE;
    }
  if((CUBE | FLOOR) == (IAFlags))
    {
      PanMixerChannel(CUBE_FLOOR, CUBE_FLOOR_GAIN, gCubeXPos);
      CheckAudioErr("StartInstrument",
                    "cube-floor collision",
                    StartInstrument(Effect[CUBE_FLOOR]->si_Player, NULL));
      CheckAudioErr("ReleaseInstrument",
                    "cube-floor collision",
                    ReleaseInstrument(Effect[CUBE_FLOOR]->si_Player, NULL));
      goto DONE;
    }
  if((GLOBE | FLOOR) == (IAFlags))
    {
      PanMixerChannel(GLOBE_FLOOR, GLOBE_FLOOR_GAIN, gGlobeXPos);
      CheckAudioErr("StartInstrument",
                    "globe-floor collision",
                    StartInstrument(Effect[GLOBE_FLOOR]->si_Player, NULL));
      CheckAudioErr("ReleaseInstrument",
                    "globe-floor collision",
                    ReleaseInstrument(Effect[GLOBE_FLOOR]->si_Player, NULL));
      goto DONE;
    }

 DONE:
  return;
}

void
PanMixerChannel(int32 ChannelNumber, int32 MaxAmp, int32 xPos)
/*
  Set the left and right gains for the sound in the specified channel
  according to the object's x-position.
*/
{
  int32 RightGain;
  int32 LeftGain;

  RightGain = ((MaxAmp * (xPos - LEFT_WALL_POS)) / WINDOW_WIDTH);
  LeftGain
    = ((MaxAmp * (WINDOW_WIDTH - (xPos - LEFT_WALL_POS))) / WINDOW_WIDTH);

  CheckAudioErr("TweakKnob",
                "left pan gain",
                TweakKnob(Effect[ChannelNumber]->si_LeftGainKnob, LeftGain));
  CheckAudioErr("TweakKnob",
                "right pan gain",
                TweakKnob(Effect[ChannelNumber]->si_RightGainKnob, RightGain));
}

int32
YPositionToPitch(int32 yPosition)
/*
  Map the specified y-position to a pitch; the higher the object (i.e.,
  the lower the yPosition), the higher the pitch.
*/
{
  return ((((gDisplayHeight - yPosition) * 10 / 100) + 50));
}

void
KillBounceSound(void)
/*
  Dispose of the mixer and all of the sounds.
*/
{
  int32 samplerIndex;

  for(samplerIndex = 0; samplerIndex < NUMSAMPLERS; samplerIndex++)
    {
      if(Effect[samplerIndex])
        ehDisposeSampleInfo(Effect[samplerIndex]);
    }

  if(gMixerInfo)
    ehDisposeMixerInfo(gMixerInfo);
}

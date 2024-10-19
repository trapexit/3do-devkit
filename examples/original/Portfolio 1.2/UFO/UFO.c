/*
        File:		UFO.c

        Contains:	UFO, the first 3DO game

        Written by:	Steve Beeman

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <8>	 8/15/93	JAY		change LoadAnim
   interface
                 <7>	 7/30/93	JAY		add new parameters to
   WaitVBL() for Dragon Tail
                 <6>	 6/24/93	JAY		change path for
   PlayBgndMusic.task from remote to UFOSound
                 <5>	 6/23/93	JAY		change ccb offset names
   for Dragon (4B1) <4>	  4/5/93	JAY		removing /remote/ from
   filename (i.e. making pathname relative to initial working directory <3>
   3/29/93	JAY		change AF_TAG_LOUDNESS to AF_TAG_AMPLITUDE for
   Cardinal1
                 <2>	 3/18/93	JAY		changed TAG_FREQ to
   TAG_RATE and changed number of arguments in StopInstrument
                 <1>	 3/18/93	JAY		first checked in

        To Do:
*/

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "filefunctions.h"

#define USE_SFX
#define SHOW_ALERTS

#define LASER_COUNT 15

#ifdef SHOW_ALERTS
#define ALERT(s) kprintf ("Alert: %s\n", s)
int nCrashFrameCount = 0;
#else
#define ALERT(s)
#endif

ScreenContext TheScreen;

int nLaserCount = 0;
bool bHoverMode = FALSE;
int nBoomCount = 0;
signed long iDistance = 8000;
signed int iVelocity = 50;

int iScore = 0;
int iMisses = 0;

ubyte *pbBackground = NULL;
CCB *pcUFO = NULL;
CCB *pcCockpit = NULL;
CCB *pcCockpitRed = NULL;
CCB *pcCockpitGreen = NULL;
CCB *pcCockpitFire = NULL;
CCB *pcCrosshairs = NULL;
CCB *pcLaser = NULL;
CCB *pcBlip = NULL;
CCB *pcOptions = NULL;
CCB *pcPleaseWait = NULL;
long *pBogusBuffer = NULL;
ANIM *aExplosion;
CCB *pcExplosion = NULL;

Item iWhirr = -1;
Item iExplosion = -1;
Item iZap = -1;
int32 nWhirrID = -1;
Item iSoundHandler;
Item iSoundHandlerMsgPort;
Item VBLIOReq;

char *szScriptFile = NULL;

unsigned int
randbits (int shift)
{
  shift = 31 - shift;
  return (rand () >> shift);
}

#define FIRST_GREEN 1
#define LAST_GREEN FIRST_GREEN + 14
#define FIRST_RED 16
#define LAST_RED FIRST_RED + 11

bool
UpdateDashboard ()
{
  static int sine[16]
      = { 0,     2753,  5502,  8241,  10965, 13670, 16351, 19003,
          21621, 24201, 26739, 29229, 31668, 34051, 36373, 38632 };
  static int cosine[16]
      = { 65536, 65478, 65304, 65015, 64612, 64094, 63463, 62720,
          61866, 60903, 59832, 58656, 57376, 55995, 54515, 52938 };
  static int cycle = 0;

  int i, dist;
  signed long x, y;
  int16 *p;
  int16 temp;

  p = (int16 *)pcCrosshairs->ccb_PLUTPtr;
  temp = p[FIRST_GREEN];
  for (i = FIRST_GREEN; i < LAST_GREEN; i++)
    p[i] = p[i + 1];
  p[LAST_GREEN] = temp;

  if (!cycle)
    {
      temp = p[FIRST_RED];
      for (i = FIRST_RED; i < LAST_RED; i++)
        p[i] = p[i + 1];
      p[LAST_RED] = temp;
      cycle = 3;
    }
  else
    --cycle;

  i = (pcUFO->ccb_Width << 8) / iDistance;
  i = (i >> 1) + (pcUFO->ccb_XPos >> 16);
  i = ((i - 160) << 1) / 21;

  dist = iDistance / 307;

  if (i < 0)
    {
      x = -1 * dist * sine[i * -1];
      y = dist * cosine[i * -1];
    }
  else
    {
      x = dist * sine[i];
      y = dist * cosine[i];
    }

  pcBlip->ccb_XPos = (246 << 16) + x + pcCockpit->ccb_XPos;
  pcBlip->ccb_YPos = (205 << 16) - y + pcCockpit->ccb_YPos;

  if (nLaserCount == LASER_COUNT)
    DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcLaser);

  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcCrosshairs);
  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcCockpit);
  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcBlip);

  if (nLaserCount)
    {
      --nLaserCount;
      DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen],
                      pcCockpitRed);
    }

  if (bHoverMode)
    DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen],
                    pcCockpitGreen);

  if (!randbits (8))
    DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen],
                    pcCockpitFire);

  return TRUE;
  /* Later, will update score and lives, and will return FALSE when all lives
   * are gone. */
}

bool
Zap (CCB *pcTarget, Coord X, Coord Y)
{
  static TagArg taStartArgs[] = {
#ifndef CardinalChange
    { AF_TAG_RATE, (void *)0x8000 },
#else
    { AF_TAG_FREQ, (void *)0x8000 },
#endif
    { AF_TAG_AMPLITUDE, (void *)0x4000 },
    { 0, 0 }
  };

  static Point Corner[4] = { { 0, 0 }, { 0, 0 }, { 319, 210 }, { 0, 210 } };

  Corner[0].pt_X = Corner[1].pt_X = X;
  Corner[0].pt_Y = Corner[1].pt_Y = Y;

  MapCel (pcLaser, Corner);

#ifdef USE_SFX
  (void)StartInstrument (iZap, taStartArgs);
#endif

  X -= (pcTarget->ccb_XPos >> 16);
  Y -= (pcTarget->ccb_YPos >> 16);
  if (X >= 0 && Y >= 0 && (Y < ((pcTarget->ccb_Height << 8) / iDistance))
      && (X < ((pcTarget->ccb_Width << 8) / iDistance)))
    return TRUE;
  else
    return FALSE;
}

bool
DoOptions (void)
{
  long lEvent;
  int32 xUFO, yUFO, hdxUFO, vdyUFO;
  TagArg taStartArgs[] = {
#ifndef CardinalChange
    { AF_TAG_RATE, (void *)0x8000 },
#else
    { AF_TAG_FREQ, (void *)0x8000 },
#endif
    { AF_TAG_AMPLITUDE, (void *)0x3000 },
    { 0, 0 }
  };

  ALERT ("DoOptions called.");

  xUFO = pcUFO->ccb_XPos;
  yUFO = pcUFO->ccb_YPos;
  hdxUFO = pcUFO->ccb_HDX;
  vdyUFO = pcUFO->ccb_VDY;

  pcUFO->ccb_XPos = (176 << 16);
  pcUFO->ccb_YPos = (67 << 16);
  pcUFO->ccb_HDX = 1 << 19;
  pcUFO->ccb_VDY = 1 << 15;
  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcOptions);
  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcUFO);
  DisplayScreen (TheScreen.sc_Screens[TheScreen.sc_curScreen], 0);

  if (nWhirrID >= 0)
#ifndef CardinalChange
    StopInstrument (iWhirr, NULL);
#else
    StopInstrument (iWhirr, nWhirrID, NULL);
#endif

  WaitVBL (VBLIOReq, 1);
  while (1)
    {
      lEvent = ReadControlPad (0);
      if (lEvent & JOYSTART)
        {
          pcUFO->ccb_XPos = xUFO;
          pcUFO->ccb_YPos = yUFO;
          pcUFO->ccb_HDX = hdxUFO;
          pcUFO->ccb_VDY = vdyUFO;
#ifdef USE_SFX
          nWhirrID = StartInstrument (iWhirr, taStartArgs);
#endif
          return TRUE;
        }
      else
        {
          if (lEvent & JOYFIREA)
            {
            }

          if (lEvent & JOYFIREB)
            {
            }

          if (lEvent & JOYFIREC)
            {
              return FALSE;
            }
        }

      WaitVBL (VBLIOReq, 1);
    }
}

bool
PlayerAction (void)
{
  static TagArg taStartArgs[] = {
#ifndef CardinalChange
    { AF_TAG_RATE, (void *)0x4000 },
#else
    { AF_TAG_FREQ, (void *)0x4000 },
#endif
    { AF_TAG_AMPLITUDE, (void *)0x7FFF },
    { 0, 0 }
  };

  long lEvent;
  long lJoyMove = 3 << 16;

  int32 Balance;
  int32 Volume;

  lEvent = ReadControlPad (JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT);

  if (lEvent & JOYSTART)
    return DoOptions ();
  else
    {
      if (lEvent & JOYFIREA && !nLaserCount)
        {
          nLaserCount = LASER_COUNT;
          if (Zap (pcUFO,
                   (pcCrosshairs->ccb_XPos >> 16)
                       + (pcCrosshairs->ccb_Width >> 1),
                   (pcCrosshairs->ccb_YPos >> 16)
                       + (pcCrosshairs->ccb_Height >> 1)))
            {
              Balance
                  = ((pcCrosshairs->ccb_XPos + (pcCrosshairs->ccb_Width >> 1))
                     >> 2)
                    / 160;
              Volume = ((8000 - iDistance) << 15) / 3000;
              SetMixer (1, Volume, Balance);
#ifdef USE_SFX
              (void)StartInstrument (iExplosion, taStartArgs);
#endif
              nBoomCount = aExplosion->num_Frames;
              pcExplosion->ccb_XPos
                  = pcCrosshairs->ccb_XPos + (pcCrosshairs->ccb_Width << 15)
                    - (pcExplosion->ccb_Width * pcUFO->ccb_VDY >> 1);
              pcExplosion->ccb_YPos
                  = pcCrosshairs->ccb_YPos + (pcCrosshairs->ccb_Height << 15)
                    - (pcExplosion->ccb_Height * pcUFO->ccb_VDY >> 1);
              pcExplosion->ccb_HDX = pcUFO->ccb_HDX;
              pcExplosion->ccb_VDY = pcUFO->ccb_VDY;

              iScore += iDistance;
              iDistance = 8000;
              if (iVelocity > 0)
                iVelocity *= -1;

              pcUFO->ccb_XPos = (20 + randbits (8)) << 16;
              pcUFO->ccb_YPos = (20 + randbits (9)) << 16;
            }
        }

      if (lEvent & JOYFIREB)
        {
          bHoverMode = (bHoverMode == FALSE);
        }

      if (lEvent & JOYFIREC)
        {
        }

      if (lEvent & JOYRIGHT
          && pcCrosshairs->ccb_XPos < ((320 - pcCrosshairs->ccb_Width) << 16))
        pcCrosshairs->ccb_XPos += lJoyMove;

      if (lEvent & JOYLEFT && pcCrosshairs->ccb_XPos > 0)
        pcCrosshairs->ccb_XPos -= lJoyMove;

      if (lEvent & JOYDOWN
          && pcCrosshairs->ccb_YPos < ((240 - pcCrosshairs->ccb_Height) << 16))
        pcCrosshairs->ccb_YPos += lJoyMove;

      if (lEvent & JOYUP && pcCrosshairs->ccb_YPos > 0)
        pcCrosshairs->ccb_YPos -= lJoyMove;
    }

  return TRUE;
}

void
TargetAction (CCB *pcTarget)
{
  static signed long iDeltaX = 1 << 16;
  static signed long iDeltaY = 3 << 15;

  signed long iTest;

  int32 Balance;
  int32 Volume;

  if (iDistance > 8000)
    {
      iDistance = 8000;
      iVelocity *= -1;
      iMisses++;

      pcTarget->ccb_XPos = (20 + randbits (8)) << 16;
      pcTarget->ccb_YPos = (20 + randbits (9)) << 16;
    }
  else if (iDistance < 256)
    {
      iDistance = 256;
      iVelocity *= -1;
    }

  if (iDistance < 500 && bHoverMode)
    {
      iDistance += (iVelocity >> 5);
      pcTarget->ccb_YPos += iDeltaY >> 1;
      pcTarget->ccb_XPos += iDeltaX >> 1;
    }
  else
    {
      iDistance += iVelocity;
      iTest = (256 << 8) / iDistance;
      pcTarget->ccb_YPos += (iDeltaY >> 7) * iTest;
      pcTarget->ccb_XPos += (iDeltaX >> 7) * iTest;

      if (!randbits (8))
        iDeltaX *= -1;

      if (!randbits (8))
        iDeltaY *= -1;
    }

  pcTarget->ccb_HDX = (1 << 28) / iDistance;
  pcTarget->ccb_VDY = (1 << 24) / iDistance;

  iTest
      = (pcTarget->ccb_YPos >> 16) + ((pcTarget->ccb_Height << 8) / iDistance);
  if (pcTarget->ccb_YPos <= (20 << 16))
    {
      pcTarget->ccb_YPos = 20 << 16;
      iDeltaY *= -1;
    }
  else if (iTest >= 149)
    {
      pcTarget->ccb_YPos -= (iTest - 149) << 16;
      iDeltaY *= -1;
    }

  iTest
      = (pcTarget->ccb_XPos >> 16) + ((pcTarget->ccb_Width << 8) / iDistance);
  if (pcTarget->ccb_XPos <= (20 << 16))
    {
      pcTarget->ccb_XPos = 20 << 16;
      iDeltaX *= -1;
    }
  else if (iTest >= 299)
    {
      pcTarget->ccb_XPos -= (iTest - 299) << 16;
      iDeltaX *= -1;
    }

  iTest -= (pcTarget->ccb_XPos >> 16);
  Balance = ((pcTarget->ccb_XPos + (iTest << 15)) >> 2) / 160;
  Volume = ((8000 - iDistance) << 15) / 8000;

  SetMixer (0, Volume, Balance);

  DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen], pcUFO);

  if (nBoomCount)
    {
      nBoomCount--;
      if (nBoomCount)
        {
          (void)GetAnimCel (aExplosion, 1 << 16);
          DrawScreenCels (TheScreen.sc_Screens[TheScreen.sc_curScreen],
                          pcExplosion);
        }
      else
        {
          aExplosion->cur_Frame = 0;
        }
    }

  return;
}

void
UnifyAnimation (ANIM *pAnim)
{
  int i;
  CCB *theCCB;

  theCCB = pAnim->pentries[0].af_CCB;
  for (i = 0; i < pAnim->num_Frames; i++)
    {
      pAnim->pentries[i].af_CCB = theCCB;
    }
}

bool
InitGame (void)
{
  int16 *p;
  int i;

  VBLIOReq = GetVBLIOReq ();

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      ALERT ("Cannot set initial working directory ");
      return FALSE;
    }

  TheScreen.sc_nScreens = 2;

  if (!OpenGraphics (&TheScreen, 2) || !OpenSPORT () || !OpenAudio ())
    {
      ALERT ("Can't open a folio!");
      return FALSE;
    }

  ALERT ("Loading background...");
  if ((pbBackground = LoadImage ("UFOArt/Sky.IMG", NULL, NULL, &TheScreen))
      == NULL)
    {
      ALERT ("Cannot load the background image");
      return FALSE;
    }

  ALERT ("Loading ufo...");
  if ((pcUFO = LoadCel ("UFOArt/UFO.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the UFO cel");
      return FALSE;
    }

  ALERT ("Loading cockpit...");
  if ((pcCockpit = LoadCel ("UFOArt/Cockpit.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the cockpit cel");
      return FALSE;
    }

  ALERT ("Loading options...");
  if ((pcOptions = LoadCel ("UFOArt/Options.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the options cel");
      return FALSE;
    }

  ALERT ("Loading pleasewait...");
  if ((pcPleaseWait = LoadCel ("UFOArt/PleaseWait.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the PleaseWait cel");
      return FALSE;
    }

  ALERT ("Starting music...");
  if ((iSoundHandler = LoadProgram ("UFOSound/PlayBgndMusic.task")) < 0)
    {
      ALERT ("Cannot spawn task for SoundHandler");
      return FALSE;
    }
  (void)SetItemPri (iSoundHandler,
                    KernelBase->kb_CurrentTask->t.n_Priority + 1);
  kprintf ("SoundHandler launched from UFO...\n");

  pcUFO->ccb_XPos = (176 << 16);
  pcUFO->ccb_YPos = (67 << 16);
  pcUFO->ccb_HDX = 1 << 19;
  pcUFO->ccb_VDY = 1 << 15;
  DrawImage (TheScreen.sc_Screens[1], pbBackground, &TheScreen);
  DrawScreenCels (TheScreen.sc_Screens[1], pcCockpit);
  DrawScreenCels (TheScreen.sc_Screens[1], pcOptions);
  DrawScreenCels (TheScreen.sc_Screens[1], pcUFO);
  DrawScreenCels (TheScreen.sc_Screens[1], pcPleaseWait);
  DisplayScreen (TheScreen.sc_Screens[1], 0);

  ALERT ("Loading crosshairs...");
  if ((pcCrosshairs = LoadCel ("UFOArt/Crosshairs.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the crosshairs cel");
      return FALSE;
    }

  ALERT ("Loading laser...");
  if ((pcLaser = LoadCel ("UFOArt/Laser.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the laser cel");
      return FALSE;
    }

  ALERT ("Loading blip...");
  if ((pcBlip = LoadCel ("UFOArt/Blip.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the radar blip cel");
      return FALSE;
    }

  ALERT ("Loading red...");
  if ((pcCockpitRed = LoadCel ("UFOArt/CockpitRed.CEL", MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the red light cel");
      return FALSE;
    }

  ALERT ("Loading green...");
  if ((pcCockpitGreen = LoadCel ("UFOArt/CockpitGreen.CEL", MEMTYPE_CEL))
      == NULL)
    {
      ALERT ("Cannot load the green light cel");
      return FALSE;
    }

  ALERT ("Loading fire...");
  if ((pcCockpitFire = LoadCel ("UFOArt/CockpitFire.CEL", MEMTYPE_CEL))
      == NULL)
    {
      ALERT ("Cannot load the fire light cel");
      return FALSE;
    }

  ALERT ("Loading explosion...");
  if ((aExplosion = LoadAnim ("UFOArt/Boom.ANIM", MEMTYPE_CEL)) == 0)
    {
      ALERT ("Cannot load the animation");
      return FALSE;
    }
  UnifyAnimation (aExplosion);
  pcExplosion = GetAnimCel (aExplosion, 0);

#ifdef USE_SFX
  ALERT ("Loading whirr...");
  if ((iWhirr = LoadSoundEffect ("UFOSound/Whirr.aiff", 1)) < 0)
    {
      ALERT ("Cannot load the sound effect");
      return FALSE;
    }

  ALERT ("Loading boom...");
  if ((iExplosion = LoadSoundEffect ("UFOSound/Explosion.aiff", 1)) < 0)
    {
      ALERT ("Cannot load the explosion sound effect");
      return FALSE;
    }

  ALERT ("Loading zap...");
  if ((iZap = LoadSoundEffect ("UFOSound/Zap.aiff", 1)) < 0)
    {
      ALERT ("Cannot load the zap sound effect");
      return FALSE;
    }

  SetChannel (iWhirr, 0);
  SetChannel (iExplosion, 1);
  SetChannel (iZap, 2);
#endif

  ALERT ("All non-music file access complete...\n");

  pcUFO->ccb_Flags |= CCB_LAST;
  pcExplosion->ccb_Flags |= CCB_LAST;
  pcLaser->ccb_Flags |= CCB_LAST;
  pcCockpitRed->ccb_Flags |= CCB_LAST;
  pcCockpitGreen->ccb_Flags |= CCB_LAST;
  pcCockpitFire->ccb_Flags |= CCB_LAST;
  pcCrosshairs->ccb_Flags |= CCB_LAST;
  pcCockpit->ccb_Flags |= CCB_LAST;
  pcBlip->ccb_Flags |= CCB_LAST;

  pcCrosshairs->ccb_XPos = (long)((160 - pcCrosshairs->ccb_Width) << 16);
  pcCrosshairs->ccb_YPos = (long)((120 - pcCrosshairs->ccb_Height) << 16);
  p = (int16 *)pcCrosshairs->ccb_PLUTPtr;
  for (i = FIRST_RED; i <= LAST_RED; i++)
    p[i] >>= 5;
  // The above is a crude hack to take the red band of the color-cycling
  // palette and turn it green. I'm not proud of it.

  nWhirrID = -1;
  TheScreen.sc_curScreen = 0;
  DrawImage (TheScreen.sc_Screens[0], pbBackground, &TheScreen);
  DrawScreenCels (TheScreen.sc_Screens[0], pcCockpit);

  // Signal(iSoundHandler, 0x10000000);	// Just pick a random signal, the task
  // waits for them all. ALERT("Back from Signal, music can start now.\n");
  // kprintf("Back from Signal, music can start now.\n");

  return DoOptions ();
}

int
main (int argc, char **argv)
{
  if (InitGame ())
    {
      while (PlayerAction ())
        {
          TheScreen.sc_curScreen = 1 - TheScreen.sc_curScreen;
          DrawImage (TheScreen.sc_Screens[TheScreen.sc_curScreen],
                     pbBackground, &TheScreen);
          TargetAction (pcUFO);
          if (!UpdateDashboard ())
            break;
          DisplayScreen (TheScreen.sc_Screens[TheScreen.sc_curScreen], 0);
        }

      kprintf ("You scored %ld points and only let %d get away.\n", iScore,
               iMisses);

      (void)DeleteItem (iSoundHandler);
    }

  FadeToBlack (&TheScreen, 1 * 60);
  kprintf ("Thank you for playing UFO version CES2.0!\n");
  FreeSoundEffects ();
  ShutDown ();
  return (0);
}

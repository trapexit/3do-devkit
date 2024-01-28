/*
        File:		OrbitSound.c

        Contains:	Orbit sound support code

        Written by:	Joe Buczek and Bill Duvall

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	  4/5/93	JAY		remove /remote/ from
   filenames (i.e. make the pathname relative to initial working directory) <2>
   3/23/93	JAY		change AF_TAG_LOUDNESS to AF_TAG_AMPLITUDE <1>
   3/17/93	JAY		making changes for Cardinal 1/03/93	jb
   Use mixer channel #3 for end of round & end of game sounds; This will share
   channel 3 with the "cannot drop piece" sound. 12/30/92	jb
   Add real sound calls. 12/29/92	jb		New today.

        To Do:
*/

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "OrbitSound.h"

Item iStopSound;
Item iAnnounce;
Item iSound2;
Item iSound3;

static TagArg taStartArgs[] = { { AF_TAG_RATE, (void *)0x8000 },
                                { AF_TAG_AMPLITUDE, (void *)0x7FFF },
                                { 0, 0 } };

/*********************************************************************************************
 * This routine is called by the main module at startup time to load/initialize
 *anything that may be needed to play sound effects and background music. It
 *returns TRUE if initialization suceeds, FALSE otherwise.
 *********************************************************************************************/
bool
InitOrbitSound (void)
{

  if ((iStopSound = LoadSoundEffect ("Orbit/sounds/stopsound.aiff", 1)) < 0)
    {
      kprintf ("Cannot load the Stop sound effect");
      return FALSE;
    }

  (void)SetChannel (iStopSound, 0);

#ifdef Use_Announce
  if ((iAnnounce = LoadSoundEffect ("Orbit/sounds/announce.aiff", 1)) < 0)
    {
      kprintf ("Cannot load the Stop sound effect");
      return FALSE;
    }
  (void)SetChannel (iAnnounce, 1);
#endif
  // SetChannel will return FALSE if it fails, but there's no real reason to
  // check for that right now.

  return TRUE;
}

/*********************************************************************************************
 * Routine to play a sound when the object clicks into place.
 * The input argument indicates how many segments are being removed by the
 *playing of the last piece. If this == 0, a simple "click" (or something)
 *should be played. Otherwise, the number could be used to play more complex
 *sounds, for example, the number could be use to play a musical chord
 *containing N notes.
 *********************************************************************************************/
void
PlayStop (long numSegmentsRemoved)
{
  if (numSegmentsRemoved == 0)
    {
      /* Play simple "click"-type sound, no scoring
       * is taking place.
       */
      StartInstrument (iStopSound, taStartArgs);
    }
  else
    {
      /* Play something more complex, like a musical chord
       * that depends on the number of segments being removed.
       */
      StartInstrument (iStopSound, taStartArgs);
      // Someday we'll do something more entertaining than this...
    }
}

void
PlayAnnounce ()
{
  StartInstrument (iAnnounce, taStartArgs);
}

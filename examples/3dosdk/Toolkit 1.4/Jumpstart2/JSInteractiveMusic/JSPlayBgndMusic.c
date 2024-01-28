/*
        File:		JSPlayBgndMusic.c

        Contains:	Background music playing task for JSInteractiveMusic

        Files used:

                $boot/JSData/Sound/BackgroundMusic.aiff		-
   Background music sound file played by this task


        Written by:	Steve Beeman
                                ( Freely adapted for JumpStart by Charlie
   Eckhaus )

        Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 1/5/94		CDE		Derived from
   PlayBgndMusic.c from old Jumpstart's GameUFO example
*/

#include "audio.h"
#include "music.h"

#define NUMBLOCKS (16)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)

int
main (int argc, char *argv[])
{
  SoundFilePlayer *psfpBackgroundMusic;
  int32 cueIn;
  int32 cueNeeded;

  OpenAudioFolio ();

  psfpBackgroundMusic
      = OpenSoundFile ("$boot/JSData/Sound/BackgroundMusic.aiff", 2, BUFSIZE);

  while (1)
    {
      StartSoundFile (psfpBackgroundMusic, 0x1000);

      cueIn = 0;
      cueNeeded = 0;

      do
        {
          if (cueNeeded)
            cueIn = WaitSignal (cueNeeded);

          ServiceSoundFile (psfpBackgroundMusic, cueIn, &cueNeeded);
        }
      while (cueNeeded);

      StopSoundFile (psfpBackgroundMusic);
      RewindSoundFile (psfpBackgroundMusic);
    }
}

/*
        File:		PlayBgndMusic.c

        Contains:	a task that plays music

        Written by:	Steve Beeman

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <4>	 6/24/93	JAY		change number of
   buffers in OpenSoundFile from 1 to 2. Minimum is 2.
                 <3>	 6/23/93	JAY		change calling sequence
   for OpenSoundFile()
                 <2>	  4/5/93	JAY		remove /remote/ from
   filename (i.e. make pathnames relative to initial working directory <1>
   3/18/93	JAY		first checked in

        To Do:
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

  (void)OpenAudioFolio ();

  psfpBackgroundMusic = OpenSoundFile ("BackgroundMusic.aiff", 2, BUFSIZE);

  // kprintf("Now waiting for signal from main application...\n");
  // waiter = Wait(0xFFFFFFFF);
  // kprintf("Signal received, music begins!\n");

  while (1)
    {
      StartSoundFile (psfpBackgroundMusic, 0x1000);

      while (psfpBackgroundMusic->sfp_LastPos
             < psfpBackgroundMusic->sfp_NumToRead)
        {
          BumpSoundFile (psfpBackgroundMusic);
          SleepAudioTicks (1);
        }

      StopSoundFile (psfpBackgroundMusic);
      RewindSoundFile (psfpBackgroundMusic);
    }
}

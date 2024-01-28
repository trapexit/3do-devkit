/*
        File:		SoundTest.cp

        Contains:	Test sound class.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CSound.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "CPlusSwiHack.h"

int
main (void)
{
  CSound snd ("$boot/UFOSound/Explosion.aiff");

  snd.Loop (5);

  snd.SetVolume (0x20000);

  snd.Rewind ();
  snd.Play ();

  ShutDown ();
}
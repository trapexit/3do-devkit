/*
        File:		InstrumentTest.cp

        Contains:	Test instrument class.

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
#include "CInstrument.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "stdio.h"

#include "CPlusSwiHack.h"

Item gVBLIOReq;

int
main (void)
{
  if (!OpenAudio ())
    {
      printf ("OpenAudio failed!\n");
      return 1;
    }

  gVBLIOReq = GetVBLIOReq ();

  CInstrument boom ("$boot/UFOSound/Explosion.aiff");

  boom.SetVolume (0x20000);
  boom.Play ();

  WaitVBL (gVBLIOReq, 100);
  DeleteItem (gVBLIOReq);

  ShutDown ();
}
/*
        File:		JSShowCel.c

        Contains:	JSShowCel

        Files used:

                $boot/JSData/Art/Sky.img	-	the background image
                $boot/JSData/Art/JSUFO.cel	-	the cel to project
   against the background image

        Written by:	Steve Beeman
                                ( Freely adapted for JumpStart by Charlie
   Eckhaus )

        Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 1/3/94		CDE		Derived from old
   Jumpstart's BasicUFO.c; Added Files used header lines.
*/

#include "Portfolio.h"
#include "Utils3DO.h"

ScreenContext gScreenContext;

ubyte *gBackgroundImage = NULL;
CCB *gUFO_CCB = NULL;

void
MakeLastCCB (CCB *aCCB)
/*
        Convenience routine to ensure that the cel engine stops after
        aCCB is processed.

        Setting ccb_NextPtr to NULL is insufficient; the cel
        engine won't stop until it hits a cel explicitly marked
        as last.  Set the final ccb_NextPtr to NULL anyway, to
        facilitate traversal of the singly-linked list in software.
*/
{
  aCCB->ccb_NextPtr = NULL;
  SetFlag (aCCB->ccb_Flags, CCB_LAST);
}

Boolean
Init (void)
{
  gScreenContext.sc_nScreens = 2;

  if (!OpenGraphics (&gScreenContext, 2))
    {
      DIAGNOSTIC ("Can't open the graphics folio!");
      return FALSE;
    }

  if (!OpenSPORT ())
    {
      DIAGNOSTIC ("Can't open SPORT I/O!");
      return FALSE;
    }

  gScreenContext.sc_curScreen = 0;

  if ((gBackgroundImage = (ubyte *)LoadImage ("$boot/JSData/Art/Sky.img", NULL,
                                              NULL, &gScreenContext))
      == NULL)
    {
      DIAGNOSTIC ("Can't load the background image");
      return FALSE;
    }

  if ((gUFO_CCB = LoadCel ("$boot/JSData/Art/JSUFO.cel", MEMTYPE_CEL)) == NULL)
    {
      DIAGNOSTIC ("Can't load the foreground cel");
      return FALSE;
    }

  MakeLastCCB (gUFO_CCB);

  return TRUE;
}

void
Cleanup (void)
/*
        Free global data and system resources.

        UnloadImage and UnloadCel both check for a NULL parameter before
        attempting to execute their main functionality.
*/
{
  UnloadImage (gBackgroundImage);
  UnloadCel (gUFO_CCB);

  ShutDown ();
}

int
main (int argc, char **argv)
{
  Item aVBLIOReq;

  aVBLIOReq = GetVBLIOReq (); // Create the I/O request we'll use for our
                              // VBL timing calls

  if (Init ())
    {
      int nCountdown;

      gScreenContext.sc_curScreen = 0;

      DrawImage (gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
                 gBackgroundImage, &gScreenContext);
      /* Draws the specified image into the current screen. */

      DrawCels (gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
                gUFO_CCB);
      /* Draws one or more cels into the specified bitmap. Since we
      only set up one cel in our cel list, this call will only draw one
      cel. */

      DisplayScreen (gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
                     0);
      /* DisplayScreen automatically waits for the next vertical blank,
      then sets up the hardware to use the specified screen as the
      display buffer. Once set up properly, the hardware will show
      this buffer on the TV automatically every frame. You only
      have to call DisplayScreen when you want to change which block
      of memory the hardware is displaying on the TV. */

      for (nCountdown = (5 * 60); nCountdown; nCountdown--)
        WaitVBL (aVBLIOReq, 1);
    }

  FadeToBlack (&gScreenContext, 60);
  printf ("Wasn't that special?\n");

  DeleteIOReq (aVBLIOReq);
  Cleanup ();
}

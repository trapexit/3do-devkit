
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsshowcel.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsshowcel
|||	jsshowcel - Demonstrates how to display cels (a Jumpstart program).
|||
|||	  Synopsis
|||
|||	    jsshowcel
|||
|||	  Description
|||
|||	    Displays a cel against a background image.
|||
|||	  Associated Files
|||
|||	    jsshowcel.make
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jsshowcel
|||
**/

#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "io.h"
#include "stdio.h"
#include "types.h"

/* We need this to display graphics on the TV */
ScreenContext *gScreenContext;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */

/* The background image against which the cel is displayed */
ubyte *gBackgroundImage = NULL;

/* The cel which is drawn against the background image */
CCB *gUFO_CCB = NULL;

/* The I/O request to use for vertical blank waiting */
Item gVBLIOReq = -1;

int32
Initialize (void)
/*
        Allocate and prepare all of the program's global resources.
        These are:

        - An I/O request for vertical blank waiting.
        - A single screen context for handling 2 screens.
        - A background image.
        - A cel.

        Returns 0 if all operations were performed successfully, otherwise
        a negative value.

        Since the DrawCels call handles chains of linked cels, we explicitly
   specify that our cel is the last in its list.
*/
{
  int32 retValue = -1;

  /*	Create the I/O request we'll use for our timing */
  gVBLIOReq = CreateVBLIOReq ();
  if (gVBLIOReq < 0)
    {
      PRT (("Can't get a VBL I/O Request\n"));
      goto DONE;
    }

  /*	Allocate memory for the screenContext */
  gScreenContext
      = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (gScreenContext == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext\n"));
      goto DONE;
    }

  if ((gDisplayItem = CreateBasicDisplay (gScreenContext, DI_TYPE_DEFAULT, 2))
      < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }

  /* Set the index of the screen we'll be using */
  gScreenContext->sc_curScreen = 0;

  /* Load the background image */
  if ((gBackgroundImage = (ubyte *)LoadImage ("jsdata/art/sky.imag", NULL,
                                              NULL, gScreenContext))
      == NULL)
    {
      PRT (("Can't load the background image\n"));
      goto DONE;
    }

  /* Load the cel */
  if ((gUFO_CCB = LoadCel ("jsdata/art/jsufo.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the foreground cel\n"));
      goto DONE;
    }

  /* Make it the last in the list */
  LAST_CEL (gUFO_CCB);

  retValue = 0;

DONE:
  return retValue;
}

void
Cleanup (void)
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

        - Disposes of the background image.
        - Disposes of the cel.
        - Disposes the screen context.
        - Deletes the I/O request for vertical blank waiting.
*/
{
  UnloadCel (gUFO_CCB);
  UnloadImage (gBackgroundImage);

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  DeleteVBLIOReq (gVBLIOReq);
}

int
main (int argc, char **argv)
{
  printf ("jsshowcel\n");

  /*	Initialize global resources */
  if (Initialize () < 0)
    goto DONE;

  /*	Draw the background image to the current screen. */
  DrawImage (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
             gBackgroundImage, gScreenContext);

  /*
          Draw (project) the cel to the current bitmap.  If this cel
          were linked to other cels, all of them would be drawn.
  */
  DrawCels (gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
            gUFO_CCB);

  /*
          DisplayScreen automatically waits for the next vertical blank,
          then tells the hardware to use the specified screen as the
          display buffer. The hardware will continue to show this buffer
          on the TV each frame, until another call to DisplayScreen, specifying
          a different block of memory, is made.
  */
  DisplayScreen (gScreenContext->sc_Screens[gScreenContext->sc_curScreen], 0);

  /*	Give the user a little time to look at the screen. */
  {
    int nCountdown;
    for (nCountdown = (10 * 30); nCountdown; nCountdown--)
      WaitVBL (gVBLIOReq, 1);
  }

DONE:
  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  /*	Always clean up after ourselves */
  Cleanup ();

  printf ("end of jsshowcel ( wasn't that special? )\n");

  return 0;
}

/*
        File:		ShapeTest.cp

        Contains:	Test graphics shape classes.

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
#include "CShape.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "stdio.h"

#include "CPlusSwiHack.h"

int
main (void)
{
  ScreenContext sc;

  /* Set up double buffered display. */
  if (OpenGraphics (&sc, 1) < 0)
    {
      printf ("OpenGraphics failed\n");
      return 1;
    }

  /* Make the screen we have created visible. */
  sc.sc_curScreen = 1;
  if (DisplayScreen (sc.sc_Screens[0], 0) < 0)
    {
      printf ("DisplayScreen failed\n");
      return 2;
    }

  /* Create a few shapes */
  Rect r1 = { 50, 50, 100, 100 };
  Rect r2 = { 0, 0, 320, 240 };

  CRect s1 (&sc, r2);
  CRect s2 (&sc, r1, 0x2222);
  CRect s3 (&sc, r1, 0x8888);
  CRect s4 (&sc, r1, 0xCCCC);
  CText s5 (&sc, "Press start button to quit!");
  CLine s6 (&sc, 50, 50, 100, 100, 0x5555);

  /* Draw a few shapes */
  s1.Erase (); // Clear the screen

  s2.SetFrameFill (0); // Frame rectangle shape 2
  s2.Draw ();          // Draw rectangle shape 2
  s3.Move (50, 50);    // Move rectangle shape 3
  s3.Draw ();          // Draw rectangle shape 3
  s4.MoveRight (100);  // Move rectangle shape 4
  s4.Draw ();          // Draw rectangle shape 4
  s5.MoveTo (50, 210); // Move text shape 5
  s5.Draw ();          // Draw text shape 5
  s4.MoveDown (50);    // Move line shape 6
  s6.Draw ();          // Draw line shape 6

  while (!(ReadControlPad (JOYSTART) & JOYSTART))
    ;

  s1.Erase (); // Clear the screen
}
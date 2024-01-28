/*
        File:		JSSlideShowVDL.h

        Contains:	Load and display image files which may contain custom
   VDLs (header).

        Files used:

                $boot/JSData/imagelist			-	list of image
   filenames to load and display $boot/JSData/Art/Å.vdl			-
   images listed in the above file

        Written by:	Eric Carlson
                                ( Freely adapted for JumpStart by Charlie
   Eckhaus )

        Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 1/3/94		CDE		Derived from
   SlideShow.h; Added Files used header lines
*/

/*
 * Definitions
 */

#define kScreenCount 2
#define kBufferCount 2

#ifndef noErr
#define noErr 0
#endif
#define TICKCOUNT() GrafBase->gf_VBLNumber

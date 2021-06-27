/*
	File:		JSBasicSlideShow.h

	Contains:	Load and display image files (header).

	Files used:
		
		$boot/JSData/basicImageList		-	list of image filenames to load and display
		$boot/JSData/Art/Å.img			-	images listed in the above file
	
	Written by:	Eric Carlson
				( Freely adapted for JumpStart by Charlie Eckhaus )

	Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 12/21/93	CDE		Derived from SlideShow.h.
*/


/*
 * Definitions
 */

#define kScreenCount	2
#define kBufferCount	2

#ifndef	noErr
#	define noErr		0
#endif
#define TICKCOUNT() GrafBase->gf_VBLNumber	



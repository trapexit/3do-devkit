/*
	File:		ShutDown.c

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2+>	  8/5/93	JAY		change the default directories ("/CD-ROM" and "/remote" to
									$boot.
		 <2>	 7/28/93	JAY		remove ResetSystemGraphics(). It went away in Dragon Tail
									release.

	To Do:
*/

#include "Portfolio.h"
#include "filefunctions.h"
#include "Init3DO.h"

void ShutDown(void)
	{
	CloseAudioFolio();
	}

/* ChangeInitialDirectory adjust your working directory if your current working directory is null or
   if always is true.  Changing your initial directory is useful if your are debugging your application
   using the debug command and you have set the data and source directory from debugger.
*/


Err ChangeInitialDirectory( char *firstChoice, char *secondChoice, bool always )
	{
	Err err;
	char DirectoryName[256];
	Item targetDir;
	
	GetDirectory( DirectoryName, 255 );
	
	if ( ( DirectoryName[0] == 0 ) || always ) {
		if ( ( targetDir = ChangeDirectory( firstChoice ? firstChoice : "$boot" ) ) < 0 )
			if ( ( targetDir = ChangeDirectory( secondChoice ? secondChoice : "$boot" ) ) < 0 )
				return targetDir;
		}
	return 0;
	}

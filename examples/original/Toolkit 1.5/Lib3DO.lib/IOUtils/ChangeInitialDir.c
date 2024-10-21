/*****************************************************************************
 *	File:			ChangeInitialDir.c
 *
 *	Contains:		Routine to change default directory.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	09/04/94  gmd 	added filefunctions.h to the includes.
 *	07/12/94  Ian 	General library cleanup.
 *	08/05/93  JAY	change the default directories ("/CD-ROM" and
 *"/remote") to $boot.
 *
 *	Implementation notes:
 *
 *	Prior to OS v1.0 this routine was required.  Nowadays there probably
 *	isn't much need for it.
 *
 *	ChangeInitialDirectory adjusts your working directory if your current
 *	working directory is null or if 'always' is true.  Changing your
 *initial directory is useful if your are debugging your application using the
 *	debug command and you have set the data and source directory from
 *debugger.
 ****************************************************************************/

#include "Init3DO.h"
#include "filefunctions.h"

Err
ChangeInitialDirectory (char *firstChoice, char *secondChoice, Boolean always)
{
  char DirectoryName[256];
  Item targetDir;

  GetDirectory (DirectoryName, 255);

  if ((DirectoryName[0] == 0) || always)
    {
      if ((targetDir = ChangeDirectory (firstChoice ? firstChoice : "$boot"))
          < 0)
        if ((targetDir
             = ChangeDirectory (secondChoice ? secondChoice : "$boot"))
            < 0)
          return targetDir;
    }
  return 0;
}

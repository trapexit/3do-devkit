/*****************************************************************************
 *	File:			GetFileSize.c 
 *
 *	Contains:		Routine to return the size of a file.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	Rewrite.  Now uses block IO instead of stream IO.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Parse3DO.h"
#include "BlockFile.h"
#include "Debug3DO.h"

int32 GetFileSize(char *fname)
{
	int32		rv;
	BlockFile	thefile;

	thefile.fDevice = 0;
	
	if ((rv = OpenBlockFile(fname, &thefile)) < 0) {
		DIAGNOSE_SYSERR(rv, ("Can't get size of file %s\n", fname));
		goto ERROR_EXIT;
	}

	rv  = thefile.fStatus.fs_ByteCount;

ERROR_EXIT:

	if (thefile.fDevice > 0) {
		CloseBlockFile(&thefile);
	}
	
	return rv;
}

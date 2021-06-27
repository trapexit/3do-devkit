
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: GetFileSize.c,v 1.6 1994/10/05 18:24:33 vertex Exp $
**
**  Lib3DO routine to get the size of a file.
**
******************************************************************************/


#include "parse3do.h"
#include "blockfile.h"
#include "debug3do.h"

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

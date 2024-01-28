/*****************************************************************************
 *	File:			FreeBuffer.c
 *
 *	Contains:		Routine to free a file buffer.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian	General library cleanup.
 *
 *	Implementation notes:
 *
 *	There is probably no need for this function; all non-obsolete functions
 *	which load files have corresponding Unload functions of their own.
 ****************************************************************************/

#include "Utils3DO.h"
#include "mem.h"

void
FreeBuffer (char *filename, int32 *fileBuffer)
{
  int32 buffSize;

  if ((buffSize = GetFileSize (filename)) <= 0)
    return;

  FREEMEM (fileBuffer, buffSize);
}

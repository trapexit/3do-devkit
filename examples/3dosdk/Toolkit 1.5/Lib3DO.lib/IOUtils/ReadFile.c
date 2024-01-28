/*****************************************************************************
 *	File:			ReadFile.c
 *
 *	Contains:		Routine to load a file (or a portion of it) into a
 *buffer.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	If you want to load an entire file at once, LoadFile() is MUCH faster.
 ****************************************************************************/

#include "Parse3DO.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#define FS_CHUNKSIZE (1024 * 1024)

int
ReadFile (char *filename, int32 size, int32 *buffer, int32 offset)
{
  int32 retval;
  Stream *fs;
  char *cbuf;
  int32 ntoread, nleft;
  int retries = 4;

  fs = OpenDiskStream (filename, 0);
  if (fs == NULL)
    return (-1);

  nleft = size;
  cbuf = (char *)buffer;

  if (offset == 0)
    {
      retval = 0;
    }
  else
    {
      retval = SeekDiskStream (fs, offset, SEEK_SET);
    }

  if (retval >= 0)
    while (nleft > 0)
      {
        ntoread = ((FS_CHUNKSIZE >= nleft) ? nleft : FS_CHUNKSIZE);
        retval = ReadDiskStream (fs, cbuf, ntoread);
        if (retval < 0)
          {
            retries--;
            if (retries == 0)
              break;
          }
        else
          {
            nleft -= ntoread;
            cbuf += ntoread;
            retries = 4;
          }
      }
  CloseDiskStream (fs);

  return (int)retval;
}

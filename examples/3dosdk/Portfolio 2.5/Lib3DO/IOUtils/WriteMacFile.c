
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: WriteMacFile.c,v 1.8 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO routine to write a buffer to a file on the Mac filesystem.
**
**  Writes count bytes from the specified buffer into the file filename.
**  Returns the actual length of what was written, which may be larger than
**  the specified count because the count is padded to the next greater
**  word multiple.  (I dunno if this is necessary, but it was in the original
**  code I started with. - Ian)
**
**  How to specify Mac path info?  Randy Carr indicated some trouble with
**  this on the debugger side, need to investigate and document.
**
******************************************************************************/

#include "debug3do.h"
#include "string.h"
#include "utils3do.h"

int32
WriteMacFile (char *filename, void *buf, int32 count)
{
  Err err;
  Item macdev = 0;
  Item ioreq = 0;
  IOReq *ior;
  IOInfo ioinfo;

  if ((macdev = OpenNamedDevice ("mac", 0)) < 0)
    {
      err = macdev;
      DIAGNOSE_SYSERR (err, ("Cannot find/open 'mac' device\n"));
      goto ERROR_EXIT;
    }

  if ((ioreq = CreateIOReq (NULL, 0, macdev, 0)) < 0)
    {
      err = ioreq;
      DIAGNOSE_SYSERR (err, ("Cannot create IOReq for 'mac' device\n"));
      goto ERROR_EXIT;
    }
  ior = (IOReq *)LookupItem (ioreq);

  memset (&ioinfo, 0, sizeof (IOInfo));
  ioinfo.ioi_Command = CMD_WRITE;
  ioinfo.ioi_Unit = 0;
  ioinfo.ioi_Send.iob_Buffer = buf;
  ioinfo.ioi_Send.iob_Len = (count + 3) & ~3;
  ioinfo.ioi_Recv.iob_Buffer = filename;
  ioinfo.ioi_Recv.iob_Len = strlen (filename) + 1L;
  ioinfo.ioi_Offset = 0;

  err = DoIO (ioreq, &ioinfo);

  if (err < 0)
    {
      DIAGNOSE_SYSERR (err,
                       ("Cannot write file %s to 'mac' device", filename));
      goto ERROR_EXIT;
    }

  err = ior->io_Actual;

ERROR_EXIT:

  DeleteIOReq (ioreq);
  CloseNamedDevice (macdev);

  return err;
}

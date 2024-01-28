
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: BlockFile.c,v 1.7 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO utility routines for block-oriented I/O
**
******************************************************************************/

#include "blockfile.h"
#include "debug3do.h"

/*******************************************************************************************
 * Routine to create an I/O request Item using the device Item in the input
 *BlockFile structure.
 *******************************************************************************************/

Item
CreateBlockFileIOReq (Item deviceItem, Item ioDoneReplyPort)
{
  return CreateIOReq (NULL, 0, deviceItem, ioDoneReplyPort);
}

/*******************************************************************************************
 * Routine to open a block file. Opens the disk file, creates and I/O request
 *item, and retrieves initial status for subsequently determining file size.
 *******************************************************************************************/

Err
OpenBlockFile (char *name, BlockFilePtr bf)
{
  Err err;
  Item ioreq;
  IOReq *ior;
  IOInfo Info;

  ioreq = 0;

  if ((bf->fDevice = OpenDiskFile (name)) < 0)
    {
      err = bf->fDevice;
      goto ERROR_EXIT;
    }

  if ((ioreq = CreateBlockFileIOReq (bf->fDevice, 0)) < 0)
    {
      err = ioreq;
      goto ERROR_EXIT;
    }
  ior = (IOReq *)LookupItem (ioreq);

  Info.ioi_Command = CMD_STATUS;
  Info.ioi_Flags = IO_QUICK;
  Info.ioi_Recv.iob_Buffer = &bf->fStatus;
  Info.ioi_Recv.iob_Len = sizeof (bf->fStatus);
  Info.ioi_Unit = 0;
  Info.ioi_Flags2 = 0;
  Info.ioi_CmdOptions = 0;
  Info.ioi_Send.iob_Buffer = 0;
  Info.ioi_Send.iob_Len = 0;
  Info.ioi_Offset = 0;

  err = DoIO (ioreq, &Info);

ERROR_EXIT:

  DeleteIOReq (ioreq);

  if (err < 0)
    {
      CloseBlockFile (bf);
    }

  return err;
}

/*******************************************************************************************
 * Routine to close a block file.
 *******************************************************************************************/

void
CloseBlockFile (BlockFilePtr bf)
{
  if (bf->fDevice > 0)
    {
      CloseDiskFile (bf->fDevice);
      bf->fDevice = 0;
    }
}

/*******************************************************************************************
 * Routine to return the size, in BYTES, of an open block file.
 *******************************************************************************************/

int32
GetBlockFileSize (BlockFilePtr bf)
{
  return (int32)bf->fStatus.fs_ByteCount;
}

/*******************************************************************************************
 * Routine to return the device block size, in BYTES, of an open block file.
 *******************************************************************************************/

int32
GetBlockFileBlockSize (BlockFilePtr bf)
{
  return (int32)bf->fStatus.fs.ds_DeviceBlockSize;
}

/*******************************************************************************************
 * Routine to post an asynchronous read of a block file.
 *	The count and offset values are expressed in bytes, but must be
 *integral multiples of the device's blocksize for everything to work properly.
 *******************************************************************************************/

Err
AsynchReadBlockFile (BlockFilePtr bf, Item ioreqItem, void *buffer,
                     int32 count, int32 offset)
{
  IOInfo Info;

  Info.ioi_Command = CMD_READ;
  Info.ioi_Offset = offset / bf->fStatus.fs.ds_DeviceBlockSize;
  Info.ioi_Recv.iob_Buffer = buffer;
  Info.ioi_Recv.iob_Len = count;
  Info.ioi_Send.iob_Buffer = 0;
  Info.ioi_Send.iob_Len = 0;
  Info.ioi_Flags = 0;
  Info.ioi_Unit = 0;
  Info.ioi_Flags2 = 0;
  Info.ioi_CmdOptions = 0;

  return SendIO (ioreqItem, &Info);
}

/*******************************************************************************************
 * Routine to determine if a posted read request has completed.
 *******************************************************************************************/

Boolean
ReadDoneBlockFile (Item ioreqItem)
{
  return (Boolean)CheckIO (ioreqItem);
}

/*******************************************************************************************
 * Routine to wait for a posted read request to complete.
 *******************************************************************************************/

int32
WaitReadDoneBlockFile (Item ioreqItem)
{
  return WaitIO (ioreqItem);
}

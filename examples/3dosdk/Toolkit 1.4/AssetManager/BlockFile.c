/*******************************************************************************************
 *	File:			BlockFile.c
 *
 *	Contains:		utility routines for block oriented file access
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	8/9/93		jb		Add CHECKRESULT() calls in
 *OpenBlockFile() routine
 *	8/2/93		jb		Change ioi_Pad to ioi_Flags2
 *	4/5/93		jb		Added CreateBlockFileIOReq(), change to
 *not assume only one I/O request per open BlockFile.
 *	3/25/93		jb		New today, lifted from demo code
 *
 *******************************************************************************************/

#include "Portfolio.h"
#include "filefunctions.h"

#include "BlockFile.h"
#include "Debug3DO.h"

/*******************************************************************************************
 * Routine to create an I/O request Item using the device Item in the input
 *BlockFile structure.
 *******************************************************************************************/
Item
CreateBlockFileIOReq (Item deviceItem, Item iodoneReplyPort)
{
  TagArg targs[3];

  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)deviceItem;

  targs[1].ta_Tag = CREATEIOREQ_TAG_REPLYPORT;
  targs[1].ta_Arg = (void *)iodoneReplyPort;

  targs[2].ta_Tag = TAG_END;

  return CreateItem (MKNODEID (KERNELNODE, IOREQNODE), targs);
}

/*******************************************************************************************
 * Routine to open a block file. Opens the disk file, creates and I/O request
 *item, and retrieves initial status for subsequently determining file size.
 *******************************************************************************************/
int32
OpenBlockFile (char *name, BlockFilePtr bf)
{
  IOInfo Info;
  Item ioreqItem;

  bf->fDevice = (Item)OpenDiskFile (name);
  if (bf->fDevice < 0)
    {
      CHECKRESULT ("OpenBlockFile", bf->fDevice);
      return bf->fDevice;
    }

  if ((long)(ioreqItem = CreateBlockFileIOReq (bf->fDevice, (Item)NULL)) < 0)
    {
      CHECKRESULT ("OpenBlockFile() -> CreateBlockFileIOReq()", ioreqItem);
      CloseDiskFile (bf->fDevice);
      return ioreqItem;
    }

  Info.ioi_Command = CMD_STATUS;
  Info.ioi_Flags = 2;
  Info.ioi_Unit = 0;
  Info.ioi_Flags2 = 0;
  Info.ioi_CmdOptions = 0;
  Info.ioi_Recv.iob_Buffer = &(bf->fStatus);
  Info.ioi_Recv.iob_Len = sizeof (bf->fStatus);
  Info.ioi_Send.iob_Buffer = 0;
  Info.ioi_Send.iob_Len = 0;
  Info.ioi_Offset = 0;

  DoIO (ioreqItem, &Info);

  DeleteItem (ioreqItem);

  return 0;
}

/*******************************************************************************************
 * Routine to close a block file.
 *******************************************************************************************/
void
CloseBlockFile (BlockFilePtr bf)
{
  CloseDiskFile (bf->fDevice);
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
GetBlockFIleBlockSize (BlockFilePtr bf)
{
  return bf->fStatus.fs.ds_DeviceBlockSize;
}

/*******************************************************************************************
 * Routine to post an asynchronous read of a block file.
 *******************************************************************************************/
int32
AsynchReadBlockFile (BlockFilePtr bf, Item ioreqItem, void *buffer, long count,
                     long offset)
{
  IOInfo Info;

  Info.ioi_Command = CMD_READ;
  Info.ioi_Flags = 0;
  Info.ioi_Unit = 0;
  Info.ioi_Flags2 = 0;
  Info.ioi_CmdOptions = 0;
  Info.ioi_Recv.iob_Buffer = buffer;
  Info.ioi_Recv.iob_Len
      = count; /* ееее is this a block count or BYTE count ? */
  Info.ioi_Send.iob_Buffer = 0;
  Info.ioi_Send.iob_Len = 0;
  Info.ioi_Offset = offset / bf->fStatus.fs.ds_DeviceBlockSize;

  return SendIO (ioreqItem, &Info);
}

/*******************************************************************************************
 * Routine to determine if a posted read request has completed.
 *******************************************************************************************/
bool
ReadDoneBlockFile (Item ioreqItem)
{
  return (bool)CheckIO (ioreqItem);
}

/*******************************************************************************************
 * Routine to wait for a posted read request to complete.
 *******************************************************************************************/
int32
WaitReadDoneBlockFile (Item ioreqItem)
{
  return WaitIO (ioreqItem);
}

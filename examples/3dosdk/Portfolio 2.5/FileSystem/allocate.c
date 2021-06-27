
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: allocate.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/allocate
|||	allocate - Allocates blocks for a file.
|||
|||	  Synopsis
|||
|||	    allocate \<file> \<bytecount>
|||
|||	  Description
|||
|||	    Demonstrates how to allocate blocks of storage for a file.
|||
|||	  Arguments
|||
|||	    file                         Name of the file for which to allocate
|||	                                 blocks.
|||
|||	    byte count                   Number of bytes to allocate.
|||
|||	  Associated Files
|||
|||	    allocate.c
|||
|||	  Location
|||
|||	    $c/allocate
|||
|||	    examples/FileSystem
|||
**/

#include "types.h"
#include "driver.h"
#include "kernel.h"
#include "io.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "operror.h"
#include "string.h"
#include "stdio.h"


/*****************************************************************************/


int main(int32 argc, char **argv)
{
Item       fileItem;
Item       ioReqItem;
int32      bytes;
int32      blocks;
IOInfo     ioInfo;
FileStatus fileStatus;
Err        err;

    /* we need two arguments */
    if (argc != 3)
    {
        printf("Usage: allocate <file> <byte count>\n");
        return -1;
    }

    /* number of bytes needed */
    bytes = strtol(argv[2], NULL, 0);

    /* open the file for access */
    fileItem = OpenDiskFile(argv[1]);
    if (fileItem >= 0)
    {
        /* create an IOReq to talk to the file */
        ioReqItem = CreateIOReq(NULL, 0, fileItem, 0);
        if (ioReqItem >= 0)
        {
            /* get the file's current size */
            memset(&ioInfo, 0, sizeof(ioInfo));
            ioInfo.ioi_Command         = CMD_STATUS;
            ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
            ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);
            err = DoIO(ioReqItem, &ioInfo);
            if (err >= 0)
            {
                printf("File currently has %d blocks allocated\n", fileStatus.fs.ds_DeviceBlockCount);
                printf("Block size is %d, ", fileStatus.fs.ds_DeviceBlockSize);
                blocks = (bytes + fileStatus.fs.ds_DeviceBlockSize - 1) /
                         fileStatus.fs.ds_DeviceBlockSize;
                printf("need %d blocks\n", blocks);

                /* try to allocate the blocks we need */
                ioInfo.ioi_Command         = FILECMD_ALLOCBLOCKS;
                ioInfo.ioi_Recv.iob_Buffer = NULL;
                ioInfo.ioi_Recv.iob_Len    = 0;
                ioInfo.ioi_Offset          = blocks;
                err = DoIO(ioReqItem, &ioInfo);
                if (err >= 0)
                {
                    printf("Allocation has succeeded\n");

                    /* check the new size */
                    ioInfo.ioi_Command         = CMD_STATUS;
                    ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
                    ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);
                    ioInfo.ioi_Offset          = 0;
                    err = DoIO(ioReqItem, &ioInfo);
                    if (err >= 0)
                    {
                        printf("File now has %d blocks allocated\n", fileStatus.fs.ds_DeviceBlockCount);
                    }
                    else
                    {
                        printf("DoIO(CMD_STATUS) failed: ");
                        PrintfSysErr(err);
                    }
                }
                else
                {
                    printf("DoIO(FILECMD_ALLOCBLOCKS) failed: ");
                    PrintfSysErr(err);
                }
            }
            else
            {
                printf("DoIO(CMD_STATUS) failed: ");
                PrintfSysErr(err);
            }
            DeleteIOReq(ioReqItem);
        }
        else
        {
            printf("CreateIOReq() failed: ");
            PrintfSysErr(ioReqItem);
        }
        CloseDiskFile(fileItem);
    }
    else
    {
        printf("OpenDiskFile(\"%s\") failed: ",argv[1]);
        PrintfSysErr(fileItem);
    }

    return 0;
}

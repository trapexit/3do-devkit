
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: nvram.c,v 1.3 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/nvram
|||	nvram - Demonstrate how to save files to nvram and other writable devices.
|||
|||	  Synopsis
|||
|||	    nvram
|||
|||	  Description
|||
|||	    Demonstrates how to save data to a file in NVRAM or to any other writable
|||	    device.
|||
|||	    The SaveDataFile() function takes a name, a data pointer, and a size
|||	    indicator, and creates a file of that name and size, containing the
|||	    supplied data.
|||
|||	    Writing data to a file requires a bit of finesse. Data can only be written
|||	    in blocks, not bytes. If the data being written is not a multiple of the
|||	    target device's blocksize, the last chunk of data must be copied to a
|||	    buffer which is the size of a block, and the block can then be written.
|||
|||	  Associated Files
|||
|||	    nvram.c
|||
|||	  Location
|||
|||	    examples/Nvram
|||
**/

#include "types.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "io.h"
#include "string.h"
#include "mem.h"
#include "stdio.h"
#include "operror.h"


/*****************************************************************************/


Err SaveDataFile(const char *name, void *data, uint32 dataSize)
{
Item       fileItem;
Item       ioReqItem;
IOInfo     ioInfo;
void      *temp;
Err        result;
uint32     numBlocks;
uint32     blockSize;
uint32     roundedSize;
FileStatus status;

    /* get rid of the file if it was already there */
    DeleteFile((char *)name);

    /* create the file again... */
    result = CreateFile((char *)name);
    if (result >= 0)
    {
        /* open the file for access */
        fileItem = OpenDiskFile((char *)name);
        if (fileItem >= 0)
        {
            /* create an IOReq to communicate with the file */
            ioReqItem = CreateIOReq(NULL, 0, fileItem, 0);
            if (ioReqItem >= 0)
            {
                /* get the block size of the file */
                memset(&ioInfo, 0, sizeof(IOInfo));
                ioInfo.ioi_Command         = CMD_STATUS;
                ioInfo.ioi_Recv.iob_Buffer = &status;
                ioInfo.ioi_Recv.iob_Len    = sizeof(FileStatus);
                result = DoIO(ioReqItem, &ioInfo);
                if (result >= 0)
                {
                    blockSize = status.fs.ds_DeviceBlockSize;
                    numBlocks = (dataSize + blockSize - 1) / blockSize;

                    /* allocate the blocks we need for this file */
                    ioInfo.ioi_Command         = FILECMD_ALLOCBLOCKS;
                    ioInfo.ioi_Recv.iob_Buffer = NULL;
                    ioInfo.ioi_Recv.iob_Len    = 0;
                    ioInfo.ioi_Offset          = numBlocks;
                    result = DoIO(ioReqItem, &ioInfo);
                    if (result >= 0)
                    {
                        /* tell the system how many bytes for this file */
                        memset(&ioInfo,0,sizeof(IOInfo));
                        ioInfo.ioi_Command         = FILECMD_SETEOF;
                        ioInfo.ioi_Offset          = dataSize;
                        result = DoIO(ioReqItem, &ioInfo);
                        if (result >= 0)
                        {
                            roundedSize = 0;
                            if (dataSize >= blockSize)
                            {
                                /* If we have more than one block's worth of
                                 * data, write as much of it as possible.
                                 */

                                roundedSize = (dataSize / blockSize) * blockSize;
                                ioInfo.ioi_Command         = CMD_WRITE;
                                ioInfo.ioi_Send.iob_Buffer = (void *)data;
                                ioInfo.ioi_Send.iob_Len    = roundedSize;
                                ioInfo.ioi_Offset          = 0;
                                result = DoIO(ioReqItem, &ioInfo);

                                data      = (void *)((uint32)data + roundedSize);
                                dataSize -= roundedSize;
                            }

                            if ((result >= 0) && dataSize)
                            {
                                /* If the amount of data left isn't as large
                                 * as a whole block, we must allocate a memory
                                 * buffer of the size of the block, copy the
                                 * rest of the data into it, and write the
                                 * buffer to disk.
                                 */

                                temp = AllocMem(blockSize,MEMTYPE_DMA | MEMTYPE_FILL);
                                if (temp)
                                {
                                    memcpy(temp,data,dataSize);
                                    ioInfo.ioi_Command         = CMD_WRITE;
                                    ioInfo.ioi_Send.iob_Buffer = temp;
                                    ioInfo.ioi_Send.iob_Len    = blockSize;
                                    ioInfo.ioi_Offset          = roundedSize;
                                    result = DoIO(ioReqItem, &ioInfo);

                                    FreeMem(temp,blockSize);
                                }
                                else
                                {
                                    result = NOMEM;
                                }
                            }
                        }
                    }
                }
                DeleteIOReq(ioReqItem);
            }
            else
            {
                result = ioReqItem;
            }
            CloseDiskFile(fileItem);
        }
        else
        {
            result = fileItem;
        }

        /* don't leave a potentially corrupt file around... */
        if (result < 0)
            DeleteFile((char *)name);
    }

    return (result);
}


/*****************************************************************************/


static char testData[] = "This is some test data to save out";

int main(int32 argc, char **argv)
{
Err err;

    err = SaveDataFile("/NVRAM/test",testData,sizeof(testData));
    if (err < 0)
    {
        printf("Could not save data: ");
        PrintfSysErr(err);
    }
}

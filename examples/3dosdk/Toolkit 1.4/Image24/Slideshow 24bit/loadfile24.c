/***************************************************************\
*								*
* General routine to load a file        			*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  27-Jul-93					*
*								*
* Copyright (c) 1993, The 3DO Company, Inc.                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

#define DBUG(x)                                                               \
  {                                                                           \
    printf x;                                                                 \
  }
#define FULLDBUG(x) /* { printf x ; } */
#define FSDBUG(x)   /* printf x */

#include "BlockFile.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "Debug3DO.h"
#include "loadfile24.h"

static IOInfo ioInfo;
static Item IOReqItem;
static IOReq *ior;

int32
openmacdevice (void)
{
  return 0;
}

int32
getfilesize (char *name)
{
  Item fitem;
  OpenFile *fptr;
  int32 filesize;
  TagArg targs[3];
  int32 j;
  FileStatus statbuffer;

  fitem = OpenDiskFile (name);
  fptr = (OpenFile *)LookupItem (fitem);
  if (fptr)
    {
      targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
      targs[0].ta_Arg = (void *)(fptr->ofi.dev.n_Item);
      targs[1].ta_Tag = TAG_END;
      if ((IOReqItem = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), targs))
          < 0)
        {
          DBUG (("GETFILESIZE:  Unable to create IOReq node (%ld)\n",
                 IOReqItem));
          return -1;
        }

      ior = (IOReq *)LookupItem (IOReqItem);

      memset (&ioInfo, 0, sizeof (ioInfo));

      ioInfo.ioi_Command = CMD_STATUS;
      ioInfo.ioi_Recv.iob_Buffer = &statbuffer;
      ioInfo.ioi_Recv.iob_Len = sizeof (statbuffer);
      ioInfo.ioi_Send.iob_Buffer = NULL;
      ioInfo.ioi_Send.iob_Len = 0;
      ioInfo.ioi_Offset = 0;

      if (((j = DoIO (IOReqItem, &ioInfo)) < 0) || (ior->io_Error != 0))
        {
          DeleteItem (IOReqItem);
          DBUG (("GETFILESIZE:  Error in attempt to get file status\n"));
          return -1;
        }

      filesize = statbuffer.fs_ByteCount;
    }
  else
    {
      filesize = -1;
    }
  DeleteItem (IOReqItem);
  FSDBUG (("fitem = %lx, fptr->ofi.dev.n_Item = %lx\n", fitem,
           fptr->ofi.dev.n_Item));
  CloseDiskFile (fitem);

  return filesize;
}

int32
parsefile (Stream *fs, char *name, void *buffer, uint32 buffersize,
           uint32 memtype, VdlChunk **rawVDLPtr, ImageCC *image)
{
  int32 chunkID;
  int32 chunkSize, datasize;
  VdlChunk *vdlbuf = NULL;
  int32 gotPDAT = 0;
  int32 gotVDL = 0;
  int bpp = 0;
  if (ReadDiskStream (fs, (char *)image, sizeof (ImageCC)) != sizeof (ImageCC))
    {
      DBUG (("Error reading file %s\n", name));
      return 0;
    }
  if (image->chunk_ID != CHUNK_IMAG)
    {
      DBUG (("Error: Expected IMAG CHUNK %s\n", name));
      return 0;
    }
  bpp = image->bitsperpixel;
  while ((ReadDiskStream (fs, (char *)&chunkID, 4) == 4))
    {
      if (ReadDiskStream (fs, (char *)&chunkSize, 4) != 4)
        {
          DBUG (("Error reading file %s\n", name));
          return 0;
        }
      datasize = chunkSize - (int32)8;
      switch (chunkID)
        {
        case (int32)CHUNK_PDAT:
          if (buffer && datasize > buffersize)
            {
              DBUG (("File size exceeds buffer size (%ld > %ld): %s\n",
                     datasize, buffersize, name));
              return 0;
            }
          if (!buffer)
            {
              buffer = ALLOCMEM ((int)(datasize), memtype);
            }
          if (!buffer)
            {
              DBUG (("Unable to allocate memory for file (%ld): %s\n",
                     datasize, name));
              return 0;
            }
          if (ReadDiskStream (fs, (char *)buffer, datasize) != datasize)
            {
              DBUG (("Error reading file %s\n", name));
              return 0;
            }
          gotPDAT = 1;
          break;

        case (int32)CHUNK_VDL:
          vdlbuf = (VdlChunk *)ALLOCMEM ((int)(chunkSize), MEMTYPE_ANY);
          if (!vdlbuf)
            {
              DBUG (("Unable to allocate memory for vdl (%ld): %s\n", datasize,
                     name));
              return 0;
            }
          vdlbuf->chunk_ID = CHUNK_VDL;
          vdlbuf->chunk_size = chunkSize;
          if (ReadDiskStream (fs, (char *)(&vdlbuf->vdlcount), datasize)
              != datasize)
            {
              DBUG (("Error reading file %s\n", name));
              return 0;
            }
          *rawVDLPtr = vdlbuf;
          gotVDL = 1;
          break;

        default:
          if (SeekDiskStream (fs, datasize, SEEK_CUR) == -1)
            {
              DBUG (("Error seeking file %s\n", name));
              return 0;
            }
          break;
        } /* end of switch */

    } /* end of whiule loop */
  return (bpp);
}

int32
loadfile24 (char *name, void *buffer, uint32 buffersize, uint32 memtype,
            VdlChunk **rawVDLPtr, ImageCC *image, int32 width, int32 height)
{
  int32 filesize;
  Stream *fstream;
  int32 bpp;

  filesize = getfilesize (name);

  if (filesize < 0)
    {
      return 0;
    }

  if (filesize == 0)
    {
      DBUG (("Empty file: %s\n", name));
      return 0;
    }
  fstream = OpenDiskStream (name, 0);

  bpp = parsefile (fstream, name, buffer, buffersize, memtype, rawVDLPtr,
                   image);

  if (bpp == 0)
    {
      DBUG (("File not a valid image format : %s\n", name));
      return -1;
    }

  if (image->w != width)
    {
      printf (" Expected width of %d, got width of %d skipping image %s\n",
              width, image->w, name);
      bpp = -1;
    }
  if (image->h != height)
    {
      printf (" Expected height of %d, got height of %d skipping image %s\n",
              height, image->h, name);
      bpp = -1;
    }

  CloseDiskStream (fstream);
  return bpp;
}

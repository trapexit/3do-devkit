
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: loadfile24.c,v 1.6 1994/11/22 22:42:06 vertex Exp $
**
**  Load an image file and return its image control chunk.
**
******************************************************************************/

#include "loadfile24.h"
#include "debug3do.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "parse3do.h"
#include "types.h"

int32
parsefile (Stream *fs, char *name, void *buffer, uint32 buffersize,
           uint32 memtype, VdlChunk **rawVDLPtr, ImageCC *image)
{
  int32 chunkID;
  int32 chunkSize, datasize;
  VdlChunk *vdlbuf = NULL;
  int32 gotPDAT = 0;
  int32 gotVDL = 0;
  int32 bpp = 0;
  int32 retValue = -1;

  if (ReadDiskStream (fs, (char *)image, sizeof (ImageCC)) != sizeof (ImageCC))
    {
      PRT (("Error reading file %s\n", name));
      goto DONE;
    }

  if (image->chunk_ID != CHUNK_IMAG)
    {
      PRT (("Expected IMAG chunk at beginning of file %s\n", name));
      goto DONE;
    }

  bpp = image->bitsperpixel;
  while (ReadDiskStream (fs, (char *)&chunkID, 4) == 4)
    {
      if (ReadDiskStream (fs, (char *)&chunkSize, 4) != 4)
        {
          PRT (("Can't read file %s\n", name));
          goto DONE;
        }

      datasize = chunkSize - (int32)8;
      switch (chunkID)
        {
        case (int32)CHUNK_PDAT:
          if (buffer && (datasize > buffersize))
            {
              PRT (("Filesize exceeds buffersize (%ld > %ld): %s\n", datasize,
                    buffersize, name));
              goto DONE;
            }

          if (buffer == NULL)
            buffer = AllocMem ((int)(datasize), memtype);

          if (buffer == NULL)
            {
              PRT (("Can't allocate memory (%ld bytes) for file: %s\n",
                    datasize, name));
              goto DONE;
            }

          if (ReadDiskStream (fs, (char *)buffer, datasize) != datasize)
            {
              PRT (("Can't read file %s\n", name));
              goto DONE;
            }

          gotPDAT = 1;
          break;

        case (int32)CHUNK_VDL:
          vdlbuf = (VdlChunk *)AllocMem ((int)(chunkSize), MEMTYPE_ANY);
          if (!vdlbuf)
            {
              PRT (("Can't allocate memory (%ld bytes) for vdl\n", datasize));
              goto DONE;
            }

          vdlbuf->chunk_ID = CHUNK_VDL;
          vdlbuf->chunk_size = chunkSize;
          if (ReadDiskStream (fs, (char *)&vdlbuf->vdlcount, datasize)
              != datasize)
            {
              PRT (("Error reading file %s\n", name));
              goto DONE;
            }

          *rawVDLPtr = vdlbuf;
          gotVDL = 1;
          break;

        default:
          if (SeekDiskStream (fs, datasize, SEEK_CUR) == -1)
            {
              PRT (("Can't seek past chunk in %s\n", name));
              goto DONE;
            }
          break;
        } /* end switch */

    } /* end while */

  retValue = bpp;

DONE:
  return retValue;
}

int32
loadfile24 (char *name, void *buffer, uint32 buffersize, uint32 memtype,
            VdlChunk **rawVDLPtr, ImageCC *image, int32 width, int32 height)
/*
        Load an image from a file which may contain a custom VDL.

        name		the filename
        buffer		pointer to a block of memory to hold the image
        buffersize	the length, in bytes, of the image buffer
        memtype		memory type flags for the image buffer, if it must be
   allocated rawVDLPtr	pointer to the rawVDLPtr to be filled in if the image
   has a custom VDL
        image		pointer to the image control chunk loaded from the file
        width		expected width of the image; it's an error if this and
   the actual width don't match height		expected height of the image;
   it's an error if this and the actual height don't match

        Returns 0 if the image is successfully loaded, otherwise -1.
*/
{
  int32 filesize;
  Stream *fstream = NULL;
  int32 bpp;
  int32 retValue = -1;

  filesize = GetFileSize (name);

  if (filesize < 0)
    {
      goto DONE;
    }

  if (filesize == 0)
    {
      PRT (("%s is empty\n", name));
      goto DONE;
    }

  fstream = OpenDiskStream (name, 0);
  if (fstream == NULL)
    {
      PRT (("Can't open %s\n", name));
      goto DONE;
    }

  bpp = parsefile (fstream, name, buffer, buffersize, memtype, rawVDLPtr,
                   image);
  if (bpp <= 0)
    {
      PRT (("%s isn't a valid image format\n", name));
      goto DONE;
    }

  if (image->w != width)
    {
      PRT (("Expected width of %d, got width of %d; Skipping file %s\n", width,
            image->w, name));
      goto DONE;
    }
  if (image->h != height)
    {
      PRT (("Expected height of %d, got height of %d; Skipping image %s\n",
            height, image->h, name));
      goto DONE;
    }

  retValue = 0;

DONE:
  if (fstream != NULL)
    CloseDiskStream (fstream);

  return retValue;
}

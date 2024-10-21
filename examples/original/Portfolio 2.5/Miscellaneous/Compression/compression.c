
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: compression.c,v 1.7 1995/02/14 01:00:29 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/compression
|||	compression - Demonstrates use of the compression folio.
|||
|||	  Synopsis
|||
|||	    compression
|||
|||	  Description
|||
|||	    Simple program demonstrating how to use the compression routines
supplied
|||	    by compression folio. The program loads itself into a memory
buffer,
|||	    compresses the data, decompresses it, and compares the original
data with
|||	    the decompressed data to make sure the compression and
decompression
|||	    processes worked successfully.
|||
|||	  Associated Files
|||
|||	    compression.c
|||
|||	  Location
|||
|||	    examples/Miscellaneous/Compression
|||
**/

#include "compression.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "mem.h"
#include "stdio.h"
#include "types.h"

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  Stream *stream;
  Err err;
  bool same;
  uint32 i;
  int32 fileSize;
  uint32 *originalData;
  uint32 *compressedData;
  uint32 *finalData;
  int32 numFinalWords;
  int32 numCompWords;

  err = OpenCompressionFolio ();
  if (err >= 0)
    {
      stream = OpenDiskStream (argv[0], 0);
      if (stream)
        {
          fileSize = stream->st_FileLength & 0xfffffffc;
          originalData = (uint32 *)malloc (fileSize);
          compressedData = (uint32 *)malloc (fileSize);
          finalData = (uint32 *)malloc (fileSize);

          if (originalData && compressedData && finalData)
            {
              if (ReadDiskStream (stream, (char *)originalData, fileSize)
                  == fileSize)
                {
                  err = SimpleCompress (
                      originalData, fileSize / sizeof (uint32), compressedData,
                      fileSize / sizeof (uint32));
                  if (err >= 0)
                    {
                      numCompWords = err;
                      err = SimpleDecompress (compressedData, numCompWords,
                                              finalData,
                                              fileSize / sizeof (uint32));
                      if (err >= 0)
                        {
                          numFinalWords = err;
                          printf ("Original data size    : %d\n",
                                  fileSize / sizeof (uint32));
                          printf ("Compressed data size  : %d\n",
                                  numCompWords);
                          printf ("Uncompressed data size: %d\n",
                                  numFinalWords);

                          same = TRUE;
                          for (i = 0; i < fileSize / sizeof (uint32); i++)
                            {
                              if (originalData[i] != finalData[i])
                                {
                                  same = FALSE;
                                  break;
                                }
                            }

                          if (same)
                            {
                              printf ("Uncompressed data matched original\n");
                            }
                          else
                            {
                              printf ("Uncompressed data differed with "
                                      "original!\n");
                              for (i = 0; i < 10; i++)
                                {
                                  printf (
                                      "orig $%08x, final $%08x, comp $%08x\n",
                                      originalData[i], finalData[i],
                                      compressedData[i]);
                                }
                            }
                        }
                      else
                        {
                          printf ("SimpleDecompress() failed: ");
                          PrintfSysErr (err);
                        }
                    }
                  else
                    {
                      printf ("SimpleCompress() failed: ");
                      PrintfSysErr (err);
                    }
                }
              else
                {
                  printf ("Could not read whole file\n");
                }
            }
          else
            {
              printf ("Could not allocate memory buffers\n");
            }

          free (originalData);
          free (compressedData);
          free (finalData);

          CloseDiskStream (stream);
        }
      else
        {
          printf ("Could not open '%s' as an input file\n", argv[0]);
        }
      CloseCompressionFolio ();
    }
  else
    {
      printf ("OpenCompressionFolio() failed: ");
      PrintfSysErr (err);
    }

  return (0);
}

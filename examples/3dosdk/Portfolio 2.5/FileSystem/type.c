
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: type.c,v 1.10 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/type
|||	type - Type a file's content to the output terminal.
|||
|||	  Synopsis
|||
|||	     type \<file1> [file2] [file3]...
|||
|||	  Description
|||
|||	    Demonstrates how to read a file and send its contents to the
Debugger
|||	    Terminal window using the byte stream routines.
|||
|||	  Arguments
|||
|||	    file1                        Name of the file to type. You can
specify an
|||	                                 arbitrary number of file names, and
they
|||	                                 will all get typed out.
|||
|||	  Associated Files
|||
|||	    type.c
|||
|||	  Location
|||
|||	    $c/type examples/FileSystem
|||
**/

#include "filestream.h"
#include "filestreamfunctions.h"
#include "stdio.h"
#include "types.h"

/*****************************************************************************/

#define MAXLINE 1024

static void
Type (const char *path)
{
  Stream *stream;
  char c;
  char line[MAXLINE];
  int32 linelen;
  int32 lines;
  int32 endOfLine, endOfFile;

  /* Open the file as a byte-stream.  A buffer size of zero is
   * specified, which permits the file folio to choose an appropriate
   * amount of buffer space based on the file's actual block size.
   */

  stream = OpenDiskStream ((char *)path, 0);
  if (!stream)
    {
      printf ("File '%s' could not be opened\n", path);
      return;
    }

  lines = 0;
  linelen = 0;
  endOfLine = FALSE;
  endOfFile = FALSE;

  /* Spin through a loop, grabbing one byte each time.
   *
   * A more efficient implementation would grab bytes
   * in larger batches (e.g. 128 bytes at a time) and parse them in a
   * more reasonable fashion.
   */

  while (!endOfFile)
    {
      if (ReadDiskStream (stream, &c, 1) < 1)
        {
          endOfFile = TRUE;
          endOfLine = TRUE;
          line[linelen] = '\0';
        }
      else if (c == '\r' || c == '\n')
        {
          endOfLine = TRUE;
          line[linelen] = '\0';
        }
      else if (linelen < MAXLINE - 1)
        {
          line[linelen++] = c;
        }

      if (endOfLine)
        {
          printf ("%s\n", line);
          linelen = 0;
          lines++;
          endOfLine = FALSE;
        }
    }

  printf ("\n%d lines processed\n\n", lines);

  /* close the file */
  CloseDiskStream (stream);
}

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  int32 i;

  if (argc < 2)
    {
      printf ("Usage: type file1 [file2] [file3] [...]\n");
    }
  else
    {
      /* go through all the arguments */
      for (i = 1; i < argc; i++)
        Type (argv[i]);
    }

  return 0;
}

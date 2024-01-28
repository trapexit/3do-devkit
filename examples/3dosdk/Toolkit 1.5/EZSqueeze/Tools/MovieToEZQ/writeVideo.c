#include <Memory.h>
#include <QDOffscreen.h>
#include <Quickdraw.h>
#include <Types.h>
#include <Windows.h>
#include <osutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "myTypes.h"
//#include "colorspace.h"
//#include "globals.h"
//#include "myGworld.h"
#include "EZQ_Defs.h"

/* External Definitions */

extern Boolean DCPMInitted;

extern int Min (int a, int b);
extern int Max (int a, int b);
extern Ptr myMalloc (int size);
extern Ptr *myFree (Ptr address);

extern int putDPCM (char *buffer, int appendFlag, PPMHeader *image,
                    FILE *logFile, PPMHeader *origImage);

#pragma segment Main

int
writeFastDPCM (FILE *file, int appendFlag, PPMHeader *image, FILE *logFile,
               PPMHeader *origImage, char *buffer)
{
  Boolean myBufferFlag;

  int bytesWritten;
  /* Write out coded image to file and return number of bytes written */

  if (file < 0)
    {
      fprintf (stdout, "\nNo output file passed to writeDPCM");
      fflush (stdout);
      return (0);
    }
  if (buffer == nil)
    {
      if ((buffer = (unsigned char *)myMalloc (
               bytesWritten = sizeof (*buffer) * image->width * image->height))
          == nil)
        {
          fprintf (stdout, "\nCould not allocate buffer in writeDPCM");
          fflush (stdout);
          return (0);
        }
      myBufferFlag = true;
    }
  else
    myBufferFlag = false;

  // fprintf(stdout, "\nMax Bytes to be written = %d", bytesWritten);
  // fflush(stdout);

  if ((bytesWritten = putDPCM (buffer, appendFlag, image, logFile, origImage))
      > 0)
    fwrite (buffer, bytesWritten, sizeof (*buffer), file);

  if (myBufferFlag)
    myFree (buffer);
  return bytesWritten;
} /* end writeDPCM */

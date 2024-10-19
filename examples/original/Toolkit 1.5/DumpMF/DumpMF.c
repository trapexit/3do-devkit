/*******************************************************************************************
 *	File:			DumpMF.c
 *
 *	Contains:		Main module of MPW tool to parse a Standard MIDI
 *File into text descriptions of every event.
 *
 *	Usage:			MFParser -i <filename>
 *
 *
 *	Written by:		Software Attic
 *					 with a few clever ideas courtesy of Phil
 *Burk
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	06/02/94	rdg		Port back to MPW after debugging in
 *ThinkC 05/10/94	rdg		New today.
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.0b2"

/* When turned on, compiles in a little code to get the command line string
 * from the ANSI terminal window.
 */
#define THINK_DEBUGGING 0

#include <Memory.h>
#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <Types.h>

#include "MFToText.h"

/****************************/
/* Local routine prototypes */
/****************************/
static void usage (void);
static char *GetIOBuffer (long bufferSize);
static Boolean ParseCommandLine (int argc, char **argv);

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
char *gProgramNameStringPtr = NULL; /* program name string ptr for usage() */

char *gInputFileName = NULL; /* no default input file name */
long gIOBufferSize
    = (32 * 1024L); /* bigger I/O buffers for MPW.	Speeds up processing */

/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
void
usage (void)
{
  printf ("Summary: Reads Std MIDI files and dumps text to StdOut.\n");
  printf ("Usage: %s -i <filename>\n", gProgramNameStringPtr);
  printf ("Version %s\n", PROGRAM_VERSION_STRING);
  printf ("\t\t-i\t\t\tinput file name\n");
}

/******************************************************************
 * Open and parse the MIDIFile, dump text to stdout.
 ******************************************************************/
int
main (int argc, char *argv[])
{
  long result;
  FILE *inputFile = NULL;

#if THINK_DEBUGGING

  char name[32];

  /* Read in the file name */
  printf ("\n Enter an Input Filename: ");
  gets (name);

  gInputFileName = name;

#else

  /* Copy program name string pointer for the usage() routine
   */
  gProgramNameStringPtr = argv[0];

  if (argc < 2)
    {
      usage ();
      goto ERROR_EXIT;
    }

  /* Parse command line and check results */

  if (!ParseCommandLine (argc, argv))
    {
      usage ();
      goto ERROR_EXIT;
    }

#endif

  if (gInputFileName == NULL)
    {
      usage ();
      printf ("error: input file name not specified\n");
      goto ERROR_EXIT;
    }

  /* Open the input and output files */

  inputFile = fopen (gInputFileName, "rb");
  if (!inputFile)
    {
      printf ("error: could not open the input file = %s\n", gInputFileName);
      goto ERROR_EXIT;
    }

  if (setvbuf (inputFile, GetIOBuffer (gIOBufferSize), _IOFBF, gIOBufferSize))
    {
      printf ("setvbuf() failed for file: %s\n", gInputFileName);
      goto ERROR_EXIT;
    }

  /* Convert the MIDIFile to text */
  result = MIDIFileToText (inputFile);

  /* Let them know we're done */
  if (result == 0)
    printf ("\n MIDIFile Parsing Successful!\n");
  else
    printf ("\n MIDIFile Parsing Failed!\n");

  /* Close the input file */
  fclose (inputFile);

  return 0; // successfull completion

ERROR_EXIT:

  if (inputFile)
    fclose (inputFile);

  return 1; // all was not well

} /* main */

/*******************************************************************************************
 * Routine to allocate alternate I/O buffers for the weaving process. Bigger is
 *better!
 *******************************************************************************************/
static char *
GetIOBuffer (long bufferSize)
{
  char *ioBuf;

  ioBuf = (char *)NewPtr (bufferSize);
  if (ioBuf == NULL)
    {
      fprintf (stderr,
               "GetIOBuffer() - failed. Use larger MPW memory partition!\n");
      fprintf (stderr,
               "                Use Get Info... from the Finder to set it.\n");
    }

  return ioBuf;
}

/*********************************************************************************
 * Routine to parse command line arguments
 *********************************************************************************/
#define PARSE_FLAG_ARG(argString, argFormat, argVariable)                     \
  if (strcmp (*argv, argString) == 0)                                         \
    {                                                                         \
      argv++;                                                                 \
      if ((argc -= 2) < 0)                                                    \
        return false;                                                         \
      sscanf (*argv++, argFormat, &argVariable);                              \
      continue;                                                               \
    }

static Boolean
ParseCommandLine (int argc, char **argv)
{
  /* Skip past the program name */
  argv++;
  argc--;

  while (argc > 0)
    {
      PARSE_FLAG_ARG ("-iobs", "%li", gIOBufferSize);

      if (strcmp (*argv, "-i") == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gInputFileName = *argv++;
          continue;
        }

      /* Unknown flag encountered
       */
      else
        return false;
    }

  return true;
}

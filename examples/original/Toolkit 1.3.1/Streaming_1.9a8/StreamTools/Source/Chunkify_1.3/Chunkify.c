/*******************************************************************************************
 *	File:			Chunkify.c
 *
 *	Contains:		MPW tool to take a binary file and turn it into
 *chunks that can be used with the Weaver tool.
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/19/93	jb		Version 1.3
 *						Change command line numeric parsing to
 *accept either decimal or hexadecimal inputs. 9/12/93		rdg
 *Version 1.2 Spin cursor. Use bigger I/O buffers for speed. Fix bug in initial
 *chunk size computation.  It was not quadbyte aligning the proper variable.
 *						Fix another bug in chunk size
 *computation.  Since the first chunk written uses a special larger header than
 *						the subsequent data chunks, the data size
 *needs to be increased for all non-first-chunks to make up for the difference
 *in size between the headers.  YUK! Initalize variables in main(). Return 1 on
 *error instead of 0 and delete partial output. 6/16/93		jb
 *Update to use Neil's join subscriber definitions. 6/16/93		jb
 *New today from WriteTestStream source
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.3"

#include <Memory.h>
#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <Types.h>
#include <cursorctl.h>

#define CHAR4LITERAL(a, b, c, d)                                              \
  ((unsigned long)(a << 24) | (b << 16) | (c << 8) | d)

#define JOIN_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('J', 'O', 'I', 'N') /* chunk data type for this subscriber */
#define JOIN_HEADER_SUBTYPE                                                   \
  CHAR4LITERAL ('J', 'H', 'D', 'R') /* subtype for header blocks */
#define JOIN_DATA_SUBTYPE                                                     \
  CHAR4LITERAL ('J', 'D', 'A', 'T') /* subtype for continuation blocks */

/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

#define int32 long
#define SUBS_CHUNK_COMMON                                                     \
  int32 chunkType;    /* chunk type */                                        \
  int32 chunkSize;    /* chunk size including header */                       \
  int32 time;         /* position in stream time */                           \
  int32 channel;      /* logical channel number */                            \
  int32 subChunkType; /* data sub-type */

typedef struct JoinChunkFirst
{
  SUBS_CHUNK_COMMON
  int32 joinChunkType; /* 'JHDR' for JoinChunkFirst  or 'JDAT' for
                          JoinChunkData */
  int32 totalDataSize; /* the total size of the data in all chunks */
  int32 ramType;       /* AllocMem flags for this type of data */
  int32 compType;      /* type of compression used on this data */
  int32 dataSize;      /* the size of the data in this chunk */
                       /* char		data[4];				   the data goes here...
                        */
} JoinChunkFirst, *JoinChunkFirstPtr;

typedef struct JoinChunkData
{
  SUBS_CHUNK_COMMON
  int32 joinChunkType; /* 'JHDR' for JoinChunkFirst  or 'JDAT' for
                          JoinChunkData */
  int32 dataSize;      /* the size of the data in this chunk */
                       /* char		data[4];				   the data goes here...
                        */
} JoinChunkData, *JoinChunkDataPtr;

/*******************************/
/* Command line switch strings */
/*******************************/
#define CHUNK_SIZE_STRING "-cs"    /* chunk size */
#define TIME_INTERVAL_STRING "-ti" /* time interval between chunks */
#define OUTPUT_FILE_STRING "-o"    /* output file name */
#define INPUT_FILE_STRING "-i"     /* input file name */
#define CHUNK_SUBTYPE_STRING                                                  \
  "-type" /* switch for specifying data type we're joining */
#define CHANNEL_NUM_STRING "-chan" /* switch for specifying channel number */
#define CHUNK_MEM_TYPE "-memtype"  /* switch for specifying memory type */
#define COMPRESSION_TYPE_STRING                                               \
  "-comp" /* switch for specifying compression type */

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
long gChunkSize = (16 * 1024);    /* default chunk size */
long gTimestampInterval = (1);    /* default timestamp per chunk interval */
char *gOutputFileName = NULL;     /* no default output file name */
char *gInputFileName = NULL;      /* no default input file name */
char *gSubchunkTypeString = NULL; /* data chunk sub-type string */
long gChannelNumber = (0);        /* default channel number */
long gMemType = (0);              /* default memory type */
long gCompressionType = (0); /* default compression type, not currently used */
long gIOBufferSize
    = (32 * 1024L); /* bigger I/O buffers for MPW.	Speeds up processing */

char *gProgramNameStringPtr; /* program name string ptr for usage() */

/****************************/
/* Local routine prototypes */
/****************************/
static void usage (void);
static Boolean ParseCommandLine (int argc, char **argv);
static char *GetIOBuffer (long bufferSize);

/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
void
usage (void)
{
  printf ("%s version %s\n", gProgramNameStringPtr, PROGRAM_VERSION_STRING);
  printf ("Usage: %s <flags>\n", gProgramNameStringPtr);
  printf ("	%s <file>		input file name\n", INPUT_FILE_STRING);
  printf ("	%s <file>		output file name\n",
          OUTPUT_FILE_STRING);
  printf ("	%s <type>	chunk sub-type\n", CHUNK_SUBTYPE_STRING);
  printf ("	%s <num>	channel number (0)\n", CHUNK_MEM_TYPE);
  printf ("	%s <num>		compression type (0)\n",
          COMPRESSION_TYPE_STRING);
  printf ("	%s <type>	memory type (0)\n", CHANNEL_NUM_STRING);
  printf (
      "	%s <size>		size of a data chunk in BYTES (16384)\n",
      CHUNK_SIZE_STRING);
  printf ("	%s <time>		timestamp interval per chunk (1)\n",
          TIME_INTERVAL_STRING);
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
      PARSE_FLAG_ARG (CHUNK_SIZE_STRING, "%li", gChunkSize);

      PARSE_FLAG_ARG (TIME_INTERVAL_STRING, "%li", gTimestampInterval);

      PARSE_FLAG_ARG (CHANNEL_NUM_STRING, "%li", gChannelNumber);

      PARSE_FLAG_ARG (CHUNK_MEM_TYPE, "%li", gMemType);

      PARSE_FLAG_ARG (COMPRESSION_TYPE_STRING, "%li", gCompressionType);

      if (strcmp (*argv, OUTPUT_FILE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gOutputFileName = *argv++;
          continue;
        }

      if (strcmp (*argv, INPUT_FILE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gInputFileName = *argv++;
          continue;
        }

      if (strcmp (*argv, CHUNK_SUBTYPE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gSubchunkTypeString = *argv++;
          continue;
        }

      /* Unknown flag encountered
       */
      return false;
    }

  return true;
}

/*********************************************************************************
 * Main program
 *********************************************************************************/
int
main (int argc, char *argv[])
{
  FILE *inputFile = NULL;
  FILE *outputFile = NULL;
  char *buffer = 0;
  long chunkDataSize = 0;
  long bytesRead = 0;
  long alignedBytesRead = 0;
  JoinChunkFirstPtr jhp = NULL;
  JoinChunkDataPtr jdp = NULL;
  Boolean fWritingFirstChunk = true;
  long subChunkType = 0;
  long inputFileSize = 0;
  long chunkHeaderSize = 0;
  long fourZeroes = 0;

  /* Copy program name string pointer for the usage() routine
   */
  gProgramNameStringPtr = argv[0];

  if (argc < 3)
    {
      usage ();
      goto error;
    }

  /* Parse command line and check results */

  if (!ParseCommandLine (argc, argv))
    {
      usage ();
      goto error;
    }

  if (gInputFileName == NULL)
    {
      usage ();
      printf ("error: input file name not specified\n");
      goto error;
    }

  if (gOutputFileName == NULL)
    {
      usage ();
      printf ("error: output file name not specified\n");
      goto error;
    }

  /* Open the input file */

  inputFile = fopen (gInputFileName, "rb");
  if (!inputFile)
    {
      printf ("error: could not open the input file = %s\n", gInputFileName);
      goto error;
    }

  if (setvbuf (inputFile, GetIOBuffer (gIOBufferSize), _IOFBF, gIOBufferSize))
    {
      printf ("setvbuf() failed for file: %s\n", gInputFileName);
      goto error;
    }

  if (0 != fseek (inputFile, 0, SEEK_END))
    {
      printf ("error: could not get file size for file = %s\n",
              gInputFileName);
      goto error;
    }

  inputFileSize = ftell (inputFile);
  fseek (inputFile, 0, SEEK_SET);

  /* Get rid of any files of the same name */
  remove (gOutputFileName);

  /* Open the output file */
  outputFile = fopen (gOutputFileName, "wb");
  if (!outputFile)
    {
      printf ("error: could not open the output file = %s\n", gOutputFileName);
      goto error;
    }

  if (setvbuf (outputFile, GetIOBuffer (gIOBufferSize), _IOFBF, gIOBufferSize))
    {
      printf ("setvbuf() failed for file: %s\n", gOutputFileName);
      goto error;
    }

  /* We want to spin cursor */
  InitCursorCtl (NULL);

  /* Assemble chunk sub-type command line switch value into
   * a real chunk sub-type variable.
   */
  if (gSubchunkTypeString == NULL)
    {
      usage ();
      printf ("error: chunk sub-type must be specified using %s flag\n",
              CHUNK_SUBTYPE_STRING);
      goto error;
    }

  if (strlen (gSubchunkTypeString) != 4)
    {
      usage ();
      printf ("error: chunk sub-type must be 4 characters in length\n");
      goto error;
    }

  /* Assemble a 4 character sub-chunk type
   */
  subChunkType = (gSubchunkTypeString[0] << 24)
                 | (gSubchunkTypeString[1] << 16)
                 | (gSubchunkTypeString[2] << 8) | gSubchunkTypeString[3];

  /* Force chunk data size to a QUADBYTE multiple, then deduct the size
   * of the header we must write for each chunk in the output file.
   */
  gChunkSize = (gChunkSize + 3) & ~3;
  chunkDataSize = gChunkSize - sizeof (JoinChunkFirst);
  if (chunkDataSize < 4)
    {
      usage ();
      printf ("error: chunk size must be >= %ld characters in length\n",
              sizeof (JoinChunkFirst) + 4);
      goto error;
    }

  /* Allocate the I/O buffer */

  buffer = malloc (chunkDataSize + sizeof (JoinChunkFirst));
  if (buffer == 0)
    {
      printf ("error: could not allocate input buffer size = %ld\n",
              chunkDataSize + sizeof (JoinChunkFirst));
      goto error;
    }

  /* Let folks know we are working on it */
  SpinCursor (32);

  /*************************************************************
   * Main loop to format and write out a chunk stream file.
   *************************************************************/

  jhp = (JoinChunkFirstPtr)buffer;
  jdp = (JoinChunkDataPtr)buffer;

  jhp->chunkType = JOIN_CHUNK_TYPE;
  jhp->subChunkType = subChunkType;
  jhp->time = 0;
  jhp->channel = gChannelNumber;
  jhp->joinChunkType = JOIN_HEADER_SUBTYPE;
  jhp->totalDataSize = inputFileSize;
  jhp->ramType = gMemType;
  jhp->compType = gCompressionType;

  chunkHeaderSize = sizeof (JoinChunkFirst);

  /* Initialize a buffer of zero bytes we can use to force
   * quadbyte alignment if the input data file is not a quadbyte
   * multiple in size.
   */
  fourZeroes = 0;

  fWritingFirstChunk = true;
  while (!feof (inputFile))
    {
      /* Let folks know we are working on it */
      SpinCursor (32);

      bytesRead
          = fread (buffer + chunkHeaderSize, 1, chunkDataSize, inputFile);
      if (bytesRead < 0)
        {
          printf ("error: fread() failed!\n");
          goto error;
        }

      /* If we are at EOF, get out */
      if (bytesRead == 0)
        break;

      alignedBytesRead = (bytesRead + 3) & ~3;

      /* Set the stream chunk size */
      jdp->chunkSize = alignedBytesRead + chunkHeaderSize;

      /* Set the data size of this block */
      if (fWritingFirstChunk)
        jhp->dataSize = bytesRead;
      else
        jdp->dataSize = bytesRead;

      /* Write filled buffer to the output file */
      if (1
          != fwrite (buffer, alignedBytesRead + chunkHeaderSize, 1,
                     outputFile))
        {
          printf ("error: fwrite() failed!\n");
          goto error;
        }

      if (alignedBytesRead != bytesRead)
        {
          if (1
              != fwrite (&fourZeroes, alignedBytesRead - bytesRead, 1,
                         outputFile))
            {
              printf ("error: fwrite() failed!\n");
              goto error;
            }
        }

      /* Update the time stamp
       */
      jdp->time += gTimestampInterval;

      if (fWritingFirstChunk)
        {
          /* Switch to the normal data header size from the original
           * larger size. Once we've written the header out, always write
           * subsequent chunks with the smaller headers and an appropriate
           * sub-type.
           */
          chunkHeaderSize = sizeof (JoinChunkData);

          /* Because we are now using a smaller header the data size must
           * be enlarged by the difference between the two headers or else
           * the chunk size written out will be smaller!!!
           */
          chunkDataSize += (sizeof (JoinChunkFirst) - sizeof (JoinChunkData));

          jdp->joinChunkType = JOIN_DATA_SUBTYPE;

          /* Switch to writing small headers */
          fWritingFirstChunk = false;
        }
    }

  /* Close both files */
  fclose (outputFile);
  fclose (inputFile);

  return 0; /* Success */

error:
  if (inputFile)
    fclose (inputFile);

  if (outputFile)
    fclose (outputFile);

  remove (gOutputFileName);

  return 1; /* Failure */
}

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

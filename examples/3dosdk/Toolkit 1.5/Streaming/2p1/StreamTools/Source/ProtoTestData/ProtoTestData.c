/*******************************************************************************************
 *	File:			ProtoTestData.c
 *
 *	Contains:		MPW tool to write simple test chunk files
 *					for ProtoSubscriber testing
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/7/94		rdg		v1.1 modified to support creation of
 *HALT chunks 4/4/94		rdg		adapted from original test data
 *tool
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1"

#include "HaltChunk.h"
#include "SubsChunkCommon.h"
#include <CursorCtl.h>
#include <Memory.h>
#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <Types.h>

/* Use bigger I/O buffers for MPW.	Speeds up processing */
#define IO_BUFFER_SIZE (32 * 1024L)

/* Handy for making four byte TYPE idents */
#define CHAR4LITERAL(a, b, c, d)                                              \
  ((unsigned long)(a << 24) | (b << 16) | (c << 8) | d)

/**********************************************
 * Format of a data chunk for this subscriber.
 * This comes from the the subscriber's header
 * file and should be kept consistant.
 **********************************************/

/* Stream version supported by this subscriber.  Used for sanity
 * checking when a header chunk is received.
 */
#define PROTO_STREAM_VERSION 0

/* Subscriber data type.  This ID is used is used when registering a subscriber
 * with the streamer so that the streamer will know what sort of data gets
 * delivered to our thread.
 */
#define PRTO_CHUNK_TYPE CHAR4LITERAL ('P', 'R', 'T', 'O')

/* Header subchunk type.  Header chunks contain one time initalization and
 * configuration stuff for a given channel.
 */
#define PHDR_CHUNK_TYPE CHAR4LITERAL ('P', 'H', 'D', 'R')

/* Data subchunk type. The actual data that will be consumed. */
#define PDAT_CHUNK_TYPE CHAR4LITERAL ('P', 'D', 'A', 'T')

/* HALT subchunk type.  Used to halt data flow in the streamer until we
 * reply.  Useful for time consuming subscriber level initalization tasks. */
#define PHLT_CHUNK_TYPE CHAR4LITERAL ('P', 'H', 'L', 'T')

/************************************************/
/* Subscriber Header and Data chunk definitions */
/************************************************/

typedef struct ProtoHeaderChunk
{
  SUBS_CHUNK_COMMON;
  long version; /*	version control for proto stream data */
} ProtoHeaderChunk, *ProtoHeaderChunkPtr;

typedef struct ProtoDataChunk
{
  SUBS_CHUNK_COMMON;
  long actualDataSize; /* Number of actual data bytes in chunk,
                                                  i.e. not including the chunk
                          header */
  char data[4];        /* start of test data */
} ProtoDataChunk, *ProtoDataChunkPtr;

typedef struct ProtoHaltChunk
{
  HaltChunk haltHeader;
  long haltDuration;
} ProtoHaltChunk, *ProtoHaltChunkPtr;

/*******************************/
/* Command line switch strings */
/*******************************/
#define NUM_CHUNKS_STRING "-nc"        /* number of chunks to write */
#define CHUNK_SIZE_STRING "-cs"        /* chunk size */
#define TIME_INTERVAL_STRING "-ts"     /* time interval between chunks */
#define SUBSCHUNK_TYPE_STRING "-sct"   /* subscriber chunk type to write */
#define HEADERCHUNK_TYPE_STRING "-hct" /* header chunk type to write */
#define DATACHUNK_TYPE_STRING "-dct"   /* data chunk type to write */
#define CHUNK_DATA_STRING "-cd"        /* chunk data to write */
#define CHUNK_CHANNEL_STRING "-lc"     /* logical channel for chunks */
#define HALT_CHUNK_STRING "-hc"        /* halt chunk option */
#define OUTPUT_FILE_STRING "-o"        /* output file name */

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
long gNumChunks = 1;           /* default # chunks to write */
long gChunkSize = (16 * 1024); /* default chunk size */
long gTimestampInterval = 1;   /* default timestamp per chunk interval */
unsigned long gSubsChunkType
    = PRTO_CHUNK_TYPE; /* default subscriber chunk type */
unsigned long gHeaderChunkType
    = PHDR_CHUNK_TYPE; /* default header chunk type  */
unsigned long gDataChunkType = PDAT_CHUNK_TYPE; /* default data chunk type */
long gChunkData = 0;             /* default data to fill chunks with */
long gChunkChannel = 0;          /* default logical channel for the chunks  */
Boolean gWriteHaltChunk = false; /* Weather or not to write a halt chunk */
long gHaltChunkTime = 0;         /* When to insert a HALT chunk */
long gHaltDuration = 0;          /* Length in audio ticks to wait upon HALT */
char *gOutputFileName = NULL;    /* no default output file name */

char *gProgramNameStringPtr = NULL; /* program name string ptr for usage() */

/****************************/
/* Local routine prototypes */
/****************************/
static void usage (void);
static Boolean ParseCommandLine (int argc, char **argv);
static char *GetIOBuffer (long bufferSize);

/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
static void
usage (void)
{
  printf ("%s version %s\n", gProgramNameStringPtr, PROGRAM_VERSION_STRING);
  printf ("Usage: %s <flags>\n", gProgramNameStringPtr);
  printf (
      "	%s\t<num>\t\t1\t\t\tnumber of chunks to write to the output file\n",
      NUM_CHUNKS_STRING);
  printf ("	%s\t<size>\t16K\t\tsize of a data chunk in BYTES\n",
          CHUNK_SIZE_STRING);
  printf ("	%s\t<time>\t1\t\t\ttimestamp interval per chunk\n",
          TIME_INTERVAL_STRING);
  printf ("	%s\t<type>\tPRTO\t\tsubscriber chunk type\n",
          SUBSCHUNK_TYPE_STRING);
  printf ("	%s\t<type>\tPHDR\t\theader chunk type\n",
          HEADERCHUNK_TYPE_STRING);
  printf ("	%s\t<type>\tPDAT\t\tdata chunk type\n", DATACHUNK_TYPE_STRING);
  printf ("	%s\t<valu>\t0\t\t\tchunk data to write\n", CHUNK_DATA_STRING);
  printf ("	%s\t<chan>\t0\t\t\tlogical channel for chunks\n",
          CHUNK_CHANNEL_STRING);
  printf ("	%s\t<time><dur>\t0\t\tHALT chunk parameters\n",
          HALT_CHUNK_STRING);
  printf ("	%s\t\t<file>\tNULL\t\toutput file name\n", OUTPUT_FILE_STRING);
}

/*********************************************************************************
 * Write a test chunk file for the ProtoSubscriber that can be woven into a
 * stream.
 *********************************************************************************/
int
main (int argc, char *argv[])
{
  FILE *outputFile;
  char *chunkPtr;
  char *chunkEndPtr;

  ProtoHeaderChunkPtr hcp;
  ProtoDataChunkPtr dcp;
  long chunkNum;

  ProtoHaltChunk aHaltChunk;

  /* Set up to do spinning cursor so people will know we are alive
   * if it's necessary to write LOTS of data.
   */
  InitCursorCtl (NULL);

  /* Copy program name string pointer for the usage() routine */
  gProgramNameStringPtr = argv[0];

  /* Punt if not enough command line args */
  if (argc < 3)
    {
      usage ();
      return 0;
    }

  /* Parse command line and check results */
  if (!ParseCommandLine (argc, argv))
    {
      usage ();
      return 0;
    }

  /* Punt if we don't have an output file name */
  if (gOutputFileName == NULL)
    {
      usage ();
      printf ("error: output file name not specified\n");
      return 0;
    }

  /* Open the output file */
  outputFile = fopen (gOutputFileName, "wb");
  if (!outputFile)
    {
      printf ("error: could not open the output file = %s\n", gOutputFileName);
      return 0;
    }

  /* Allocate bigger I/O buffers for the output file to speed processing */
  if (setvbuf (outputFile, GetIOBuffer (IO_BUFFER_SIZE), _IOFBF,
               IO_BUFFER_SIZE))
    {
      printf ("setvbuf() failed for file: %s\n", gOutputFileName);
      fclose (outputFile);
      return 0;
    }

  /* Allocate the I/O buffer */
  chunkPtr = malloc (gChunkSize);
  if (chunkPtr == 0)
    {
      printf ("error: could not allocate I/O block memory size = %ld\n",
              gChunkSize);
      return 0;
    }

  /* If we're going to write a HALT chunk, range check parameter values and
   * format the chunk now so we can drop it into the chunk file with
   * the proper time arrives.
   */
  if ((gWriteHaltChunk) && (gHaltDuration <= 0))
    {
      printf ("error: HALT chunk duration must be greater than zero!\n");
      fclose (outputFile);
      return 0;
    }

  if (gWriteHaltChunk)
    {
      /* This chunk header is for the Streamer... it then delivers the
       * encapsulated chunk described below to the appropriate subscriber.
       */
      aHaltChunk.haltHeader.streamerPart.chunkType = STREAM_CHUNK_TYPE;
      aHaltChunk.haltHeader.streamerPart.chunkSize = sizeof (ProtoHaltChunk);
      aHaltChunk.haltHeader.streamerPart.time = gHaltChunkTime;
      aHaltChunk.haltHeader.streamerPart.channel = 0;
      aHaltChunk.haltHeader.streamerPart.subChunkType = HALT_CHUNK_TYPE;

      /* This is the chunk which is delivered to the ProtoSubscriber */
      aHaltChunk.haltHeader.subscriberPart.chunkType = PRTO_CHUNK_TYPE;
      aHaltChunk.haltHeader.subscriberPart.chunkSize
          = (sizeof (ProtoHaltChunk) - sizeof (SimpleChunk));
      aHaltChunk.haltHeader.subscriberPart.time = gHaltChunkTime;
      aHaltChunk.haltHeader.subscriberPart.channel = 0;
      aHaltChunk.haltHeader.subscriberPart.subChunkType = PHLT_CHUNK_TYPE;

      /* This is the data that the subscriber receives */
      aHaltChunk.haltDuration = gHaltDuration;
    }

  /* Fill the chunk initially with ZERO for easy dump analysis */
  memset (chunkPtr, 0, gChunkSize);

  /*************************************************************
   * Main loop to format and write out a chunk file.
   *************************************************************/

  /* Alywas make the first chunk a header chunk */
  hcp = (ProtoHeaderChunkPtr)chunkPtr;

  hcp->chunkType = gSubsChunkType;
  hcp->chunkSize = sizeof (ProtoHeaderChunk);
  hcp->time = 0;
  hcp->channel = gChunkChannel;
  hcp->subChunkType = gHeaderChunkType;
  hcp->version = PROTO_STREAM_VERSION;

  /* Write filled buffer to the output file */
  if (1 != fwrite (chunkPtr, hcp->chunkSize, 1, outputFile))
    {
      printf ("error: write failed!\n");
      return 0;
    }

  /* Get ready to write data chunks */
  dcp = (ProtoDataChunkPtr)chunkPtr;
  chunkEndPtr = chunkPtr + gChunkSize;

  /* Only the time stamp will change from chunk to chunk.
   * Just write these once and loop...
   */
  dcp->chunkType = gSubsChunkType;
  dcp->chunkSize = gChunkSize;
  dcp->time = 1;
  dcp->channel = gChunkChannel;
  dcp->subChunkType = gDataChunkType;
  dcp->actualDataSize = chunkEndPtr - &dcp->data;

  /* Write the data in the data portion of the chunk */
  memset (&dcp->data, gChunkData, dcp->actualDataSize);

  /* Crank out the chunks and update time */
  for (chunkNum = 1; chunkNum < gNumChunks; chunkNum++)
    {
      SpinCursor (32);

      /* If it's time, write the HALT chunk out */
      if ((gWriteHaltChunk) && (dcp->time >= gHaltChunkTime))
        {
          /* Write the chunk to the output file */
          if (1
              != fwrite (&aHaltChunk,
                         aHaltChunk.haltHeader.streamerPart.chunkSize, 1,
                         outputFile))
            {
              printf ("error: write failed!\n");
              return 0;
            }

          gWriteHaltChunk = false;
        }

      /* Write the chunk to the output file */
      if (1 != fwrite (chunkPtr, gChunkSize, 1, outputFile))
        {
          printf ("error: write failed!\n");
          return 0;
        }

      /* Update the time stamp */
      dcp->time += gTimestampInterval;
    }

  /* Close the output file */
  fclose (outputFile);

  return 0;
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
  /* Temporary string ptr for use when extracting TYPE
   * idents from the command line.
   */
  char *ts;

  /* Skip past the program name */
  argv++;
  argc--;

  while (argc > 0)
    {
      PARSE_FLAG_ARG (NUM_CHUNKS_STRING, "%ld", gNumChunks);

      PARSE_FLAG_ARG (CHUNK_SIZE_STRING, "%ld", gChunkSize);

      PARSE_FLAG_ARG (TIME_INTERVAL_STRING, "%ld", gTimestampInterval);

      PARSE_FLAG_ARG (CHUNK_DATA_STRING, "%ld", gChunkData);

      PARSE_FLAG_ARG (CHUNK_CHANNEL_STRING, "%ld", gChunkChannel);

      if (strcmp (*argv, HALT_CHUNK_STRING) == 0)
        {
          argv++;
          if ((argc -= 3) < 0)
            return false;

          gWriteHaltChunk = true;

          sscanf (*argv++, "%ld", &gHaltChunkTime);
          sscanf (*argv++, "%ld", &gHaltDuration);

          continue;
        }

      if (strcmp (*argv, SUBSCHUNK_TYPE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;

          ts = *argv++;

          /* Extract the chunk type from the command line */
          gSubsChunkType = CHAR4LITERAL (ts[0], ts[1], ts[2], ts[3]);

          continue;
        }

      if (strcmp (*argv, HEADERCHUNK_TYPE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;

          ts = *argv++;

          /* Extract the chunk type from the command line */
          gHeaderChunkType = CHAR4LITERAL (ts[0], ts[1], ts[2], ts[3]);

          continue;
        }

      if (strcmp (*argv, DATACHUNK_TYPE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;

          ts = *argv++;

          /* Extract the chunk type from the command line */
          gDataChunkType = CHAR4LITERAL (ts[0], ts[1], ts[2], ts[3]);

          continue;
        }

      if (strcmp (*argv, OUTPUT_FILE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gOutputFileName = *argv++;

          continue;
        }

      /* Unknown flag encountered
       */
      return false;
    }

  return true;
}

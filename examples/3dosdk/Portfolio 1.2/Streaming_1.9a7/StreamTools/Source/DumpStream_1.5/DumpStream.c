/*******************************************************************************************
 *	File:			DumpStream.c
 *
 *	Contains:		tool to dump a stream
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/12/93		rdg		Version 1.5
 *						Spin cursor.
 *	7/29/93		jb		Version 1.4
 *						Add output of channel number and subchunk
 *type 7/14/93		jb		Version 1.3 Add BANDWIDTH_FLAG_STRING
 *option to allow data bandwidth problem detection (not foolproof, but better
 *than nothing). Add FILLBLOCK_FLAG_STRING option to allow output of filler
 *						block size to stdout (permit analysis of
 *stream with other tools, such as spreadsheets). 6/25/93		jb
 *Version 1.2 Make sure we don't try to dump data when the chunk size is less
 *than a reasonable minimum (should only happen with filler chunks) 6/3/93
 *jb		Version 1.1 Add statistics calcs and output.
 *	5/20/93		jb		Version 1.0
 *	5/20/93		jb		Add check for non-quadbyte aligned
 *chunk sizes. Add program version string output in Usage() routine. Add
 *general purpose warnings switch to enable future data checking. 5/18/93
 *jb		Output blank lines between stream blocks (demarked by filler
 *chunk) 5/17/93		jb		New today.
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.5"

#include <StdLib.h>
#include <String.h>
#include <Types.h>
#include <cursorctl.h>

#include "WeaveStream.h"

#define AUDIO_TICKS_PER_SECOND 240
#define CDROM_BYTES_PER_AUDIO_TICK (300000L / 240L)

#define MAX_OK_CHUNK_SIZE (156L * 1024L)

#define MEDIA_BLOCK_SIZE (2048L)
#define MAX_BUFFER_SIZE (128 * 1024L)
#define HEX_DUMP_SIZE 16 /* hex bytes to dump on verbose output */

#define VERBOSE_FLAG_STRING "-v"
#define VERBOSEHEX_FLAG_STRING "-vh"
#define STARTTIME_FLAG_STRING "-s"
#define INPUTFILE_FLAG_STRING "-i"
#define HEX_FPOS_FLAG_STRING "-xf"
#define WARNING_FLAG_STRING "-w"
#define STATS_FLAG_STRING "-stats"
#define FILLBLOCK_FLAG_STRING "-fbs"
#define BLOCK_SIZE_FLAG "-bs"
#define BANDWIDTH_FLAG_STRING "-bw"

#define QUOTE ((char)0x22) /* ASCII double quote character */

/*************************/
/* Command line switches */
/*************************/
long gDumpStartTime = 0;
char *gInputStreamName = NULL;
Boolean gVerboseFlag = false;
Boolean gVerboseHexFlag = false;
Boolean gHexFposOutputFlag = false;
Boolean gWarningsFlag = true;
Boolean gStatsFlag = false;
Boolean gFillerBlockSizesFlag = false;
Boolean gBandwidthCheck = false;
long gBandwidthMax = 0;
long gBlockSize = (32 * 1024L); /* default block size, zero = unwoven stream */

/********************************************************/
/* Macro for testing a value to be non-QUADBYTE aligned */
/********************************************************/
#define NOT_QUADBYTE_ALIGNED(x) (((unsigned long)x) & 0x3)

/**************************/
/* Stream block structure */
/**************************/
typedef struct StreamBlockXtra
{
  long channel;      /* logical channel number */
  long subChunkType; /* data sub-type */

} StreamBlockXtra, *StreamBlockXtraPtr;

/**************************/
/* Local utility routines */
/**************************/
static void DumpStreamFile (char *fileName, FILE *fd, long startTime,
                            Boolean verbose, Boolean verboseHex);
static Boolean ParseCommandLine (int argc, char **argv);
static void Usage (char *commandNamePtr);

/*******************************************************************************************
 * Command line parser. Accepts the usual -x unix style switches. Puts parsing
 * results into application global variables which can be pre-initialized to
 * default values. Returns TRUE if command line parsed OK, FALSE if any error.
 *******************************************************************************************/
static Boolean
ParseCommandLine (int argc, char **argv)
{
  argv++;
  argc--;

  while (argc > 0)
    {
      /* Check for the flag that specifies the stream data
       * block size. Default for this is 32k bytes.
       */
      if (strcmp (*argv, BLOCK_SIZE_FLAG) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          sscanf (*argv++, "%li", &gBlockSize);
        }

      /* Check for the flag that specifies the stream data
       * start time. Default for this is time == 0.
       */
      if (strcmp (*argv, STARTTIME_FLAG_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          sscanf (*argv++, "%li", &gDumpStartTime);
        }

      /* Check for the flag that specifies the output data file name
       */
      else if (strcmp (*argv, INPUTFILE_FLAG_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;

          gInputStreamName = *argv++; /* save pointer to file name */
        }

      /* Check for the flag that specifies hex output of file positions
       */
      else if (strcmp (*argv, HEX_FPOS_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gHexFposOutputFlag = ~gHexFposOutputFlag;
        }

      /* Check for the flag that specifies verbose output
       */
      else if (strcmp (*argv, VERBOSE_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gVerboseFlag = ~gVerboseFlag;
        }

      /* Check for the flag that specifies verbose hex output
       */
      else if (strcmp (*argv, VERBOSEHEX_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gVerboseFlag = ~gVerboseHexFlag;
        }

      /* Check for the flag to suppress warning messages
       */
      else if (strcmp (*argv, WARNING_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gWarningsFlag = ~gWarningsFlag;
        }

      /* Check for the flag to output statistics
       */
      else if (strcmp (*argv, STATS_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gStatsFlag = ~gStatsFlag;
        }

      /* Check for the flag to output filler block sizes
       */
      else if (strcmp (*argv, FILLBLOCK_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gFillerBlockSizesFlag = ~gFillerBlockSizesFlag;
        }

      /* Check for the flag to check stream data bandwidth
       */
      else if (strcmp (*argv, BANDWIDTH_FLAG_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          sscanf (*argv++, "%li", &gBandwidthMax);
          gBandwidthCheck = true;
        }

      /* Unknown flag encountered
       */
      else
        return false;
    }

  return true;
}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *commandNamePtr)
{
  fprintf (stderr, "%s version %s\n", commandNamePtr, PROGRAM_VERSION_STRING);
  fprintf (stderr, "usage: %s flags\n", commandNamePtr);
  fprintf (stderr, "\t%s <file>\tinput stream file name\n",
           INPUTFILE_FLAG_STRING);
  fprintf (stderr, "\t%s <time>\tstart dump at time\n", STARTTIME_FLAG_STRING);
  fprintf (stderr, "\t%s <bps>\tmax data bandwidth\n", BANDWIDTH_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\t\tverbose output\n", VERBOSE_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\thex file position output\n",
           HEX_FPOS_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\t\tsuppress warnings\n", WARNING_FLAG_STRING);
  fprintf (stderr, "\t%s\t\tstatistics output only\n", STATS_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\tfiller block sizes output only\n",
           FILLBLOCK_FLAG_STRING);
}

/*******************************************************************************************
 * Routine to dump a stream file to standard output
 *******************************************************************************************/
static void
DumpStreamFile (char *fileName, FILE *fd, long startTime, Boolean verbose,
                Boolean verboseHex)
{
  char *buffer;
  WeaveChunkPtr wcp;
  char *type;
  char *data;
  long bytesToDump;
  long filePosition = 0;
  Boolean fDumping = false;
  Boolean fFillerChunk;
  long totalFillerBytes = 0;
  Boolean fFoundEndOfFirstChunk = false;
  long streamBlockSize = 0;
  long percentFiller = 0;
  long totalFillerChunks = 0;
  long nextBlockStart;
  long lastStreamTime;

  StreamBlockXtraPtr sbx;

  buffer = (char *)malloc (MAX_BUFFER_SIZE);
  wcp = (WeaveChunkPtr)buffer;
  type = (char *)&wcp->weaveChunkType;
  if (buffer == NULL)
    {
      fprintf (stderr, "DumpStreamFile() - unable to allocate input buffer\n");
      return;
    }

  if (!gStatsFlag)
    {
      printf ("\n\n  Dump of stream file: %s\n\n", fileName);

      if (verbose)
        {
          printf (
              "  File Pos   Type        Size       Time    Chan    SubType\n");
          printf (
              "  --------  ------       ----       ----    ----    -------\n");
        }
      else if (verboseHex)
        {
          printf ("  File Pos   Type        Size       Time\n");
          printf ("  --------  ------       ----       ----\n");
        }
    }

  /* Set the "last" time to trigger comparison of
   * time/data bandwidth if gBandwidthCheck is set.
   */
  lastStreamTime = -1;

  while (1 == fread (buffer, sizeof (WeaveChunk), 1, fd))
    {
      SpinCursor (32);

      if (gBlockSize != 0)
        {
          /* Calculate the next block starting position. If there isn't enough
           * room in the current "block" to hold another complete chunk header,
           * then skip ahead to the next whole stream block like the parser
           * would. NOTE: this is only useful for woven streams whose data have
           * been aligned on fixed blocks.
           */
          nextBlockStart
              = ((filePosition / gBlockSize) * gBlockSize) + gBlockSize;
          if ((filePosition + sizeof (WeaveChunk)) >= nextBlockStart)
            {
              filePosition = nextBlockStart;
              fseek (fd, filePosition, SEEK_SET);
              continue;
            }
        }

        /* Skip chunks until the specified start time
         * arrives in the stream. Set a one-shot flag to enable
         * dumping. We do this because chunk times may be funky
         * for some chunks.
         */

#define ALT_FILLER_DATA_TYPE (('F' << 24) | ('I' << 16) | ('L' << 8) | 'L')

      if ((wcp->weaveChunkType == FILLER_DATA_TYPE)
          || (wcp->weaveChunkType == ALT_FILLER_DATA_TYPE))
        fFillerChunk = true;
      else
        fFillerChunk = false;

      /* Try to detect data bandwidth problems in the following way:
       * Watch for timestamp > previous_timestamp. Check for bandwidth
       * problems when they change because there may be several chunks
       * with the same timestamp and we want the very last one.
       */
      if ((!fFillerChunk) && gBandwidthCheck)
        {
          if (wcp->weaveChunkTime != lastStreamTime)
            {
              /* Reset the "last" time so we detect the next transition */
              lastStreamTime = wcp->weaveChunkTime;

              /* Test for data bandwidth exceeded and output a warning if so */
              if ((wcp->weaveChunkTime
                   * (gBandwidthMax / AUDIO_TICKS_PER_SECOND))
                  < filePosition)
                {
                  printf ("Warning: bandwidth exceeded at time = %ld, filepos "
                          "= %ld\n",
                          wcp->weaveChunkTime, filePosition);
                }
            }
        }

      /* Begin dumping when we've reached a non-filler chunk whose
       * timestamp meets or exceeds the starting time specified
       * on the command line.
       */
      if ((!fDumping) && (!fFillerChunk) && (wcp->weaveChunkTime >= startTime))
        fDumping = true;

      if (fDumping)
        {
          /* Check to see if chunk size is a quadbyte multiple. A well formed
           * stream must have quadbyte multiple sized data for the streamer
           * to work properly.
           */
          if (gWarningsFlag)
            {
              if (NOT_QUADBYTE_ALIGNED (wcp->weaveChunkSize))
                printf ("Warning: following chunk size not a quadbyte "
                        "multiple:\n");
            }

          if (!gStatsFlag)
            {
              /* Output hex formatted file position if requested
               */
              if (gHexFposOutputFlag)
                printf ("%10lX: ", filePosition);
              else
                printf ("%10ld: ", filePosition);

              printf ("'%c%c%c%c' %10ld %10ld", type[0], type[1], type[2],
                      type[3], wcp->weaveChunkSize, wcp->weaveChunkTime);

              if (verbose || verboseHex)
                {
                  data = (char *)&wcp->weaveChunkTime
                         + sizeof (wcp->weaveChunkTime);

                  /* Pin hex dump size to the max constant
                   */
                  bytesToDump = wcp->weaveChunkSize - sizeof (WeaveChunk);
                  if (bytesToDump > HEX_DUMP_SIZE)
                    bytesToDump = HEX_DUMP_SIZE;

                  if ((bytesToDump <= 0) && (!fFillerChunk))
                    printf ("*** data hunk size less than normal minimum ***");

                  else if (verboseHex)
                    {
                      if (bytesToDump > 0)
                        {
                          if (1 == fread (data, bytesToDump, 1, fd))
                            {
                              printf (" --");

                              while (bytesToDump-- > 0)
                                printf (" %02X", (0xFF & *data++));
                            }
                        }
                    }
                  else if (verbose)
                    {
                      sbx = (StreamBlockXtraPtr)data;
                      if (1 == fread (data, sizeof (StreamBlockXtra), 1, fd))
                        {
                          printf (" --");
                          printf ("%5ld%10.4s", sbx->channel,
                                  &sbx->subChunkType);
                        }
                    }
                }

              printf ("\n");

              /* Output a blank line to separate blocks in the stream.
               * This assumes that a block is terminated with a filler chunk.
               */
              if (fFillerChunk)
                printf ("\n");
            }
          else
            /* Accumulating statistics, not doing a normal data dump
             * of the stream. We keep a total count of the filler bytes
             * in the file, and try to deduce the stream block size by
             * adding the fileposition and size of the first filler
             * chunk encountered.
             */
            {
              if (fFillerChunk)
                {
                  /* Output the filler block size to stdout if the
                   * option specifying this is selected.
                   */
                  if (gFillerBlockSizesFlag)
                    printf ("%ld\n", wcp->weaveChunkSize);

                  /* Tally all filler bytes in the file */
                  totalFillerBytes += wcp->weaveChunkSize;
                  totalFillerChunks++;

                  /* Use the sum of the file position of the first filler
                   * chunk and its length as the stream block size.
                   */
                  if (!fFoundEndOfFirstChunk)
                    {
                      streamBlockSize = filePosition + wcp->weaveChunkSize;
                      fFoundEndOfFirstChunk = true;
                    }
                }
            }
        }

      /* Position the file to the next chunk in the file
       */
      if (wcp->weaveChunkSize > MAX_OK_CHUNK_SIZE)
        {
          printf ("invalid chunk size!\n");
          return;
        }
      filePosition += wcp->weaveChunkSize;
      fseek (fd, filePosition, SEEK_SET);
    }

  /* If we're doing a statistics only dump, then now is our chance to display
   * what we've collected about the stream. For now, we give the percentage of
   * data to filler (to help determine wasted space and stream data bandwidth),
   * and a suggested optimum blocksize, which is the average block size less
   * the filler blocks, rounded to the nearest MEDIA_BLOCK_SIZE.
   */
  if (gStatsFlag)
    {
      /* Check filePosition to prevent divide-by-zero
       */
      if ((filePosition > 0) && (!gFillerBlockSizesFlag))
        {
          percentFiller = (totalFillerBytes * 100) / filePosition;

          if (totalFillerChunks > 0)
            {
              printf ("%c%s%c contains %ld%% filler\n", QUOTE, fileName, QUOTE,
                      percentFiller);

              printf ("\t\tblock size = %ld bytes\n", streamBlockSize);

              printf ("\t\taverage filler block size = %ld bytes\n\n",
                      totalFillerBytes / totalFillerChunks);
            }
          else
            {
              printf ("%c%s%c contained no filler blocks\n", QUOTE, fileName,
                      QUOTE);
              printf ("stream block size cannot be determined\n");
            }
        }
      else
        printf ("File %s is empty!\n", fileName);
    }
}

/*******************************************************************************************
 * Tool application main entrypoint.
 *******************************************************************************************/
void
main (int argc, char *argv[])
{
  FILE *fd;

  if (!ParseCommandLine (argc, argv))
    {
      Usage (argv[0]);
      return;
    }

  /* Make sure an input stream name was specified */
  if (gInputStreamName == NULL)
    {
      Usage (argv[0]);
      return;
    }

  /* Open the input data stream file */
  fd = fopen (gInputStreamName, "rb");
  if (fd == NULL)
    {
      fprintf (stderr, "error opening input stream %s\n", gInputStreamName);
      return;
    }

  /* We want to spin cursor later */
  InitCursorCtl (NULL);

  DumpStreamFile (gInputStreamName, fd, gDumpStartTime, gVerboseFlag,
                  gVerboseHexFlag);

  exit (0);
}

/*******************************************************************************************
 *	File:			CtlMaker.c
 *
 *	Contains:		MPW tool to make Control subscriber data chunks
 *for weaving into streams. Can be used to make used with the Weaver tool.
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	5/12/94		dtc		Version 1.1
 *						Changed the definitions of DS_MSG_HEADER
 *and SUBS_CHUNK_COMMON to require semicolon when used in context.  (For
 *readibility and compilation using ObjectMaster). 7/10/93		jb
 *New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1"

#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <Types.h>

#define CTRL_CHUNK_TYPE ('CTRL') /* chunk type for this subscriber */

#define GOTO_CHUNK_SUBTYPE ('GOTO') /* GOTO chunk sub-type */
#define ALRM_CHUNK_SUBTYPE ('ALRM') /* ALRM chunk sub-type */
#define PAUS_CHUNK_SUBTYPE ('PAUS') /* PAUS chunk sub-type */
#define STOP_CHUNK_SUBTYPE ('STOP') /* STOP chunk sub-type */

/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

#define int32 long
#define uint32 unsigned long
#define SUBS_CHUNK_COMMON                                                     \
  int32 chunkType;   /* chunk type */                                         \
  int32 chunkSize;   /* chunk size including header */                        \
  int32 time;        /* position in stream time */                            \
  int32 channel;     /* logical channel number */                             \
  int32 subChunkType /* data sub-type */

typedef struct ControlChunk
{
  SUBS_CHUNK_COMMON; /* from SubscriberUtils.h */
  union
  {
    struct
    { /* sub-type 'GOTO' */
      uint32 value;
    } marker;

    struct
    { /* sub-type 'SYNC' */
      uint32 value;
    } sync;

    struct
    { /* sub-type 'ALRM' */
      uint32 options;
      uint32 newTime;
    } alarm;

    struct
    { /* sub-type 'PAUS' */
      uint32 options;
      uint32 amount;
    } pause;

    struct
    { /* sub-type 'STOP' */
      uint32 options;
    } stop;
  } u;

} ControlChunk, *ControlChunkPtr;

/*******************************/
/* Command line switch strings */
/*******************************/
#define OUTPUT_FILE_STRING "-o" /* output file name */
#define TIME_STRING "-time"     /* stream time for the chunk */

#define GOTO_STRING "-goto"   /* make a GOTO chunk */
#define PAUSE_STRING "-pause" /* make a PAUS chunk */
#define STOP_STRING "-stop"   /* make a STOP chunk */
#define ALARM_STRING "-alarm" /* make an ALRM chunk */

#define OPTIONS_STRING                                                        \
  "-options" /* switch for specifying 'options' field value */

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
char *gOutputFileName = NULL; /* no default output file name */
long gSubchunkType = (0);     /* data chunk sub-type */
long gTimestamp = (0);        /* timestamp for output chunk */
long gGotoDestination = (0);  /* GOTO destination */
long gPauseAmount = (0);      /* PAUS amount */
long gAlarmNewTime = (0);     /* ALRM newTime value */
long gChunkOptions = (0); /* 'option' value for all chunk types that use it */

char *gProgramNameStringPtr; /* program name string ptr for usage() */

/**************************************************/
/* Not currently used, but might be in the future */
/**************************************************/
long gChannelNumber = (0);     /* default channel number */
long gTimestampInterval = (1); /* default timestamp per chunk interval */

/****************************/
/* Local routine prototypes */
/****************************/
static void usage (void);
static Boolean ParseCommandLine (int argc, char **argv);

/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
void
usage (void)
{
  printf ("%s version %s\n", gProgramNameStringPtr, PROGRAM_VERSION_STRING);
  printf ("Usage: %s <flags>\n", gProgramNameStringPtr);
  printf ("   %s <file>                output file name\n",
          OUTPUT_FILE_STRING);
  printf ("   %s <stream time>      timestamp for output chunk\n",
          TIME_STRING);
  printf ("   %s <marker value>     place to branch in the stream\n",
          GOTO_STRING);
  printf ("   %s <pause duration>  length of pause in stream time\n",
          PAUSE_STRING);
  printf ("   %s                    specify creation of a STOP chunk\n",
          STOP_STRING);
  printf ("   %s <new Time>        new stream clock value\n", ALARM_STRING);
  printf ("   %s <chunk options> options for chunk, if applicable\n",
          OPTIONS_STRING);
#if 0
	printf( "   %s <num>              channel number (0)\n", CHUNK_MEM_TYPE );
	printf( "   %s <time>             timestamp interval per chunk (1)\n", TIME_INTERVAL_STRING );
#endif
}

/*********************************************************************************
 * Routine to parse command line arguments
 *********************************************************************************/
#define PARSE_FLAG_ARG(argString, argFormat, argVariable, statement)          \
  if (strcmp (*argv, argString) == 0)                                         \
    {                                                                         \
      argv++;                                                                 \
      if ((argc -= 2) < 0)                                                    \
        return false;                                                         \
      sscanf (*argv++, argFormat, &argVariable);                              \
      statement;                                                              \
      continue;                                                               \
    }

static Boolean
ParseCommandLine (int argc, char **argv)
{
  long dummy;

  /* Skip past the program name */
  argv++;
  argc--;

  while (argc > 0)
    {
      PARSE_FLAG_ARG (TIME_STRING, "%li", gTimestamp, (dummy = 0));
      PARSE_FLAG_ARG (GOTO_STRING, "%li", gGotoDestination,
                      (gSubchunkType = GOTO_CHUNK_SUBTYPE));
      PARSE_FLAG_ARG (PAUSE_STRING, "%li", gPauseAmount,
                      (gSubchunkType = PAUS_CHUNK_SUBTYPE));
      PARSE_FLAG_ARG (ALARM_STRING, "%li", gAlarmNewTime,
                      (gSubchunkType = ALRM_CHUNK_SUBTYPE));
      PARSE_FLAG_ARG (OPTIONS_STRING, "%li", gChunkOptions, (dummy = 0));
#if 0
		PARSE_FLAG_ARG( TIME_INTERVAL_STRING, "%li", gTimestampInterval );
		PARSE_FLAG_ARG( CHANNEL_NUM_STRING, "%ld", gChannelNumber );
#endif

      if (strcmp (*argv, OUTPUT_FILE_STRING) == 0)
        {
          argv++;
          if ((argc -= 2) < 0)
            return false;
          gOutputFileName = *argv++;
          continue;
        }

      if (strcmp (*argv, STOP_STRING) == 0)
        {
          argv++;
          argc--;
          gSubchunkType = STOP_CHUNK_SUBTYPE;
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
  FILE *outputFile;
  ControlChunk chunk;

  /* Copy program name string pointer for the usage() routine
   */
  gProgramNameStringPtr = argv[0];

  if (argc < 4)
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

  /* Fill in the common chunk fields before filling in any of
   * the specific fields required by the type we are doing.
   */
  memset (&chunk, 0, sizeof (chunk));
  chunk.chunkType = CTRL_CHUNK_TYPE;
  chunk.chunkSize = sizeof (chunk);
  chunk.time = gTimestamp;
  chunk.channel = gChannelNumber;
  chunk.subChunkType = gSubchunkType;

  /* Fill in special fields depending on chunk subtype we are building
   */
  switch (gSubchunkType)
    {
    case GOTO_CHUNK_SUBTYPE:
      chunk.u.marker.value = gGotoDestination;
      break;

    case ALRM_CHUNK_SUBTYPE:
      chunk.u.alarm.options = gChunkOptions;
      chunk.u.alarm.newTime = gAlarmNewTime;
      break;

    case PAUS_CHUNK_SUBTYPE:
      chunk.u.pause.options = gChunkOptions;
      chunk.u.pause.amount = gPauseAmount;
      break;

    case STOP_CHUNK_SUBTYPE:
      chunk.u.stop.options = gChunkOptions;
      break;

    default:
      printf ("Chunk subtype was not specified. You must choose one of the "
              "following:\n");
      printf ("   %s, %s, %s, or %s\n", GOTO_STRING, PAUSE_STRING, STOP_STRING,
              ALARM_STRING);
      return 0;
    }

  /* Now write out the chunk we just filled in */
  if (1 != fwrite (&chunk, sizeof (chunk), 1, outputFile))
    {
      printf ("error: fwrite() failed!\n");
      return 0;
    }

  fclose (outputFile);

  return 0;
}

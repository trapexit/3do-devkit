/*******************************************************************************************
 *	File:			DumpStreamCCBs.c
 *
 *	Contains:		tool to dump CCB's found in an SANM stream
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	 8/30/94	dld		Changed the line that dumps sccb_Y to
 *say "sccb_Y" instead of "sccb_X". 7/24/94		dtc
 *Version 1.2 Changed version number to 1.2
 *	7/17/94		dld		Changed dump of x,y positions to
 *floating point.  These values are 16.16, and so I am dumping them as decimal
 *floating point values converted on-the-fly. 5/12/94		dtc
 *Version 1.1d2 Changed the definitions of DS_MSG_HEADER and SUBS_CHUNK_COMMON
 *						to require semicolon when used in
 *context.  (For readibility and compilation using ObjectMaster). 9/28/93
 *jb		Version 1.1d1 Add dumping for APOS and AMAP chunks; add command
 *line switches to control dumping of all chunks. 9/16/93		jb
 *Version 1.0d2 Add capability to dump old fashioned cel files (non-stream).
 *						Decode the AV Value in the PIXC
 *word. 9/14/93		jb		Version 1.0d1 Started with source code
 *of DumpStream.c tool. Does extensive decoding of the CCB flags, PIXC, and
 *Preamble control words.
 *	=======================================================================================
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

#define PROGRAM_VERSION_STRING "1.2"

#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <Types.h>
#include <cursorctl.h>

#include "PortableSANMDefs.h"
#include "WeaveStream.h"

#define MAX_FLAG_BITS_PER_LINE 4

#define MAX_OK_CHUNK_SIZE (156L * 1024L)
#define MAX_BUFFER_SIZE (128 * 1024L)
#define CCB_CHUNK_TYPE 'CCB '

#define STARTTIME_FLAG_STRING "-s"
#define INPUTFILE_FLAG_STRING "-i"
#define HEX_FPOS_FLAG_STRING "-xf"
#define FILLBLOCK_FLAG_STRING "-fbs"
#define BLOCK_SIZE_FLAG "-bs"
#define CEL_FILE_FLAG "-cf"
#define DUMP_APOS_FLAG_STRING "-APOS"
#define DUMP_ACCB_FLAG_STRING "-ACCB"
#define DUMP_AMAP_FLAG_STRING "-AMAP"

/*************************/
/* Command line switches */
/*************************/
long gDumpStartTime = 0;
char *gInputStreamName = NULL;
Boolean gHexFposOutputFlag = false;
Boolean gFillerBlockSizesFlag = false;
Boolean gCelFileFlag = false;
Boolean gDumpAPOSFlag = true;
Boolean gDumpACCBFlag = true;
Boolean gDumpAMAPFlag = false;
long gBlockSize = (32 * 1024L); /* default block size, zero = unwoven stream */

/********************************************************/
/* Macro for testing a value to be non-QUADBYTE aligned */
/********************************************************/
#define NOT_QUADBYTE_ALIGNED(x) (((unsigned long)x) & 0x3)

/***************************************************/
/* The following are new animation player subtypes */
/***************************************************/
#define APOS_CHUNK_TYPE ('APOS')
/* The above chunk subtype is used to change the X-Y position of
 * an animation cel without transmitting the entire CCB again.
 */
typedef struct StreamAnimPos
{
  SUBS_CHUNK_COMMON; /* subType = 'APOS' */
  long duration;     /* in audio ticks */
  long newXValue;    /* new cel X position */
  long newYValue;    /* new cel Y position */
} StreamAnimPos, *StreamAnimPosPtr;

#define AMAP_CHUNK_TYPE ('AMAP')
/* The above chunk subtype is used to change the mapping values
 * of an animation cel. The params are passed to MapCel() and
 * applied to the CCB.
 */
typedef struct StreamAnimMap
{
  SUBS_CHUNK_COMMON; /* subType = 'AMAP' */
  long duration;     /* in audio ticks */
  struct LongPoint
  {
    long x;
    long y;
  } quad[4]; /* 4 corner points for MapCel() call */

} StreamAnimMap, *StreamAnimMapPtr;

/**************************/
/* Local utility routines */
/**************************/
static void DumpStreamFile (char *fileName, FILE *fd, long startTime);
static Boolean ParseCommandLine (int argc, char **argv);
static void DumpSACCB (long fileOffset, StreamAnimCCBPtr saccb);
static void DumpSAPOS (long fileOffset, StreamAnimPosPtr sapos);
static void DumpSAMAP (long filePosition, StreamAnimMapPtr samap);
static void Usage (char *commandNamePtr);
static char *ConvertF16_String (char *stringBuffer, long value);

/**************/
/* Local data */
/**************/
typedef struct NamedBit
{
  char *bitName;          /* ptr to name string for bit */
  unsigned long bitValue; /* mask to AND with to test for bit */
} NamedBit, *NamedBitPtr;

NamedBit gCCBFlagBits[] = {
  "CCB_SKIP",   0x80000000,       "CCB_LAST",  0x40000000,   "CCB_NPABS",
  0x20000000,   "CCB_SPABS",      0x10000000,  "CCB_PPABS",  0x08000000,
  "CCB_LDSIZE", 0x04000000,       "CCB_LDPRS", 0x02000000,   "CCB_LDPIXC",
  0x01000000,   "CCB_LDPLUT",     0x00800000,  "CCB_CCBPRE", 0x00400000,
  "CCB_YOXY",   0x00200000,       "CCB_ACSC",  0x00100000,   "CCB_ALSC",
  0x00080000,   "CCB_ACW",        0x00040000,  "CCB_ACCW",   0x00020000,
  "CCB_TWD",    0x00010000,       "CCB_LCE",   0x00008000,   "CCB_ACE",
  0x00004000,   "CCB_reserved13", 0x00002000,  "CCB_MARIA",  0x00001000,
  "CCB_PXOR",   0x00000800,       "CCB_USEAV", 0x00000400,   "CCB_PACKED",
  0x00000200,   "CCB_PLUTPOS",    0x00000040,  "CCB_BGND",   0x00000020,
  "CCB_NOBLK",  0x00000010
};
#define NUM_CCB_FLAGS (sizeof (gCCBFlagBits) / sizeof (NamedBit))

NamedBit gPRE0Bits[] = { "PACKED",  0x80000000, "BGND", 0x40000000,
                         "UNCODED", 0x00000010, "REP8", 0x00000008 };
#define NUM_PRE0_FLAGS (sizeof (gPRE0Bits) / sizeof (NamedBit))

NamedBit gPRE1Bits[] = { "NOSWAP", 0x00004000, "LRFORM", 0x00000800 };
#define NUM_PRE1_FLAGS (sizeof (gPRE1Bits) / sizeof (NamedBit))

NamedBit gAVFlagBits[] = { "Disable output wrap prevention", 0x0004,
                           "Enable 2S sign extension",       0x0002,
                           "Subtract 2S in final stage",     0x0001 };
#define NUM_AV_FLAGS (sizeof (gAVFlagBits) / sizeof (NamedBit))

/***************************************************/
/* Strings for formatting a "quad" point structure */
/***************************************************/
static char *gQuadCornerName[]
    = { "topLeft", "topRight", "bottomRight", "bottomLeft" };

/************************************************/
/* Used for indenting the details of a CCB dump */
/************************************************/
char *gCCBDetailsIndent = "\t";

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
          gHexFposOutputFlag = !gHexFposOutputFlag;
        }

      /* Check for the flag to output filler block sizes
       */
      else if (strcmp (*argv, FILLBLOCK_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gFillerBlockSizesFlag = !gFillerBlockSizesFlag;
        }

      /* Check for the flag to interpret non-stream (old) cel files
       */
      else if (strcmp (*argv, CEL_FILE_FLAG) == 0)
        {
          argv++;
          argc--;
          gCelFileFlag = !gCelFileFlag;
        }

      /* Check for the flag to interpret positioning chunks
       */
      else if (strcmp (*argv, DUMP_APOS_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gDumpAPOSFlag = !gDumpAPOSFlag;
        }

      /* Check for the flag to interpret CCB's
       */
      else if (strcmp (*argv, DUMP_ACCB_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gDumpACCBFlag = !gDumpACCBFlag;
        }

      /* Check for the flag to interpret map chunks
       */
      else if (strcmp (*argv, DUMP_AMAP_FLAG_STRING) == 0)
        {
          argv++;
          argc--;
          gDumpAMAPFlag = !gDumpAMAPFlag;
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
  fprintf (stderr, "\t%s\t\t\thex file position output\n",
           HEX_FPOS_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\tfiller block sizes output only\n",
           FILLBLOCK_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\tdump non-stream cel file\n", CEL_FILE_FLAG);
  fprintf (stderr, "\t%s\t\t\tdump APOS chunks (default)\n",
           DUMP_APOS_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\tdump ACCB chunks (default)\n",
           DUMP_ACCB_FLAG_STRING);
  fprintf (stderr, "\t%s\t\t\tdump AMAP chunks\n", DUMP_AMAP_FLAG_STRING);
}

/*******************************************************************************************
 * Routine to convert a frac16 into a string, for printing.
 *******************************************************************************************/

static char *
ConvertF16_String (char *stringBuffer, long value)
{
  long i, frac;
  long digit;
  char buf[16];
  Boolean printedDot = false;

  frac = value & 0xFFFF;
  for (i = 0; i < 8; i++)
    {
      if (frac == 0)
        break;
      frac = frac * 10;
      digit = frac >> 16;
      frac = frac & 0xFFFF;
      buf[i] = digit + '0';
    }
  buf[i] = 0;
  if (i == 0)
    sprintf (stringBuffer, "%d", value >> 16);
  else
    sprintf (stringBuffer, "%d.%s", value >> 16, buf);

  return stringBuffer;
}

/*******************************************************************************************
 * Routine to dump a PIXC control word
 *******************************************************************************************/
static void
DumpPIXC (char *label, unsigned long flags, unsigned long ccbFlags)
{
  NamedBitPtr pnb;
  short outCount;
  short flagNumber;

  /* Primary source */
  printf ("%s\t%s Primary source is ", gCCBDetailsIndent, label);
  if (flags & 0x8000)
    printf ("frame buffer\n");
  else
    printf ("pixel decoder (default)\n");

  /* Source of PMV */
  printf ("%s\t%s MS = ", gCCBDetailsIndent, label);
  switch ((flags >> 13) & 3)
    {
    case 0:
      printf ("00 - PMV from CCB value\n");
      break;
    case 1:
      printf ("01 - PMV is AMV from decoder\n");
      break;
    case 2:
      printf ("10 - PMV and PDV from color value out of decoder\n");
      break;
    case 3:
      printf ("11 - PMV alone from color value out of decoder\n");
      break;
    }

  /* The MF field is only meaningful if the MS field is == 0 */
  if (((flags >> 13) & 3) == 0)
    printf ("%s\t%s MF = %d (multiplier)\n", gCCBDetailsIndent, label,
            1 + ((flags >> 10) & 7));

  /* The DF field is only meaningful if the MS field is == 2 */
  if (((flags >> 13) & 3) != 2)
    {
      printf ("%s\t%s DF = ", gCCBDetailsIndent, label);
      switch ((flags >> 8) & 3)
        {
        case 0:
          printf ("16");
          break;
        case 1:
          printf ("2");
          break;
        case 2:
          printf ("4");
          break;
        case 3:
          printf ("8");
          break;
        }
      printf (" (divider)\n");
    }

  /* Secondary source */
  printf ("%s\t%s Secondary source is ", gCCBDetailsIndent, label);
  switch ((flags >> 6) & 3)
    {
    case 0:
      printf ("00 - none\n");
      break;
    case 1:
      printf ("AV = %d\n", ((flags >> 1) & 0x1f));
      break;
    case 2:
      printf ("10 - current frame buffer pixel\n");
      break;
    case 3:
      printf ("11 - pixel coming from the decoder\n");
      break;
    }

#define CCB_USEAV 0x00000400
  /* AV Value */
  if (ccbFlags & CCB_USEAV)
    {
      unsigned long flagsCopy = (flags >> 1) & 0x1f;

      /* Secondary divider */
      printf ("%s\t%s SDV = ", gCCBDetailsIndent, label);
      switch ((flagsCopy >> 3) & 3)
        {
        case 0:
          printf ("1\n");
          break;
        case 1:
          printf ("2\n");
          break;
        case 2:
          printf ("4\n");
          break;
        case 3:
          printf ("2 LSB's of color from decoder\n");
          break;
        }

      /* Decode AV bits if any are set */
      if (flagsCopy & 7)
        {
          printf ("%s\t%s AV function is  ", gCCBDetailsIndent, label);
          pnb = gAVFlagBits;
          outCount = 0;
          for (flagNumber = 0; flagNumber < NUM_AV_FLAGS; pnb++, flagNumber++)
            {
              if (flagsCopy & pnb->bitValue)
                {
                  if (outCount > MAX_FLAG_BITS_PER_LINE)
                    {
                      outCount = 0;
                      printf ("\n%s\t", gCCBDetailsIndent);
                    }

                  if (outCount != 0)
                    printf (", ");

                  printf ("%s", pnb->bitName);
                  ++outCount;
                }
            }
          printf ("\n");
        }
    }
  else
    /* Secondary divider */
    printf ("%s\t%s Secondary divider = %d\n", gCCBDetailsIndent, label,
            1 + (flags & 1));
}

/*******************************************************************************************
 * Routine to dump a CCB from a streamed ANIM to standard output
 *******************************************************************************************/
static void
DumpSACCB (long filePosition, StreamAnimCCBPtr saccb)
{
  NamedBitPtr pnb = gCCBFlagBits;
  register unsigned long flags;
  short outCount;
  short flagNumber;
  char buf[64];

  printf ("-----------------------------------------------\n");
  printf ("File offset: ");
  if (gHexFposOutputFlag)
    printf ("0x%-10lX\n", filePosition);
  else
    printf ("%-10ld\n", filePosition);

  printf ("Stream time: %-10ld\n", saccb->time);
  printf ("Channel num: %-10ld\n", saccb->channel);
  printf ("CCB Details:\n");

  /* Loop through table of flag bits and output names of all flags
   * that are set in the flags word.
   */
  printf ("%sccb_Flags:\t", gCCBDetailsIndent);
  outCount = 0;
  flags = saccb->ccb_Flags;
  for (flagNumber = 0; flagNumber < NUM_CCB_FLAGS; pnb++, flagNumber++)
    {
      if (flags & pnb->bitValue)
        {
          if (outCount > MAX_FLAG_BITS_PER_LINE)
            {
              outCount = 0;
              printf ("\n%s\t", gCCBDetailsIndent);
            }

          if (outCount != 0)
            printf (", ");

          printf ("%s", pnb->bitName);
          ++outCount;
        }
    }
  printf ("\n");

#define CCB_POVER_MASK 0x00000180
#define CCB_POVER_SHIFT 7
  printf ("%s\tP-Mode = ", gCCBDetailsIndent);
  switch ((flags & CCB_POVER_MASK) >> CCB_POVER_SHIFT)
    {
    case 0:
      printf ("in pixel data\n");
      break;
    case 1:
      printf ("**invalid**\n");
      break;
    case 2:
      printf ("P-Mode 0\n");
      break;
    case 3:
      printf ("P-Mode 1\n");
      break;
    }

  printf ("%sccb_X:\t\t%s\n", gCCBDetailsIndent,
          ConvertF16_String (buf, saccb->ccb_X));
  printf ("%sccb_Y:\t\t%s\n", gCCBDetailsIndent,
          ConvertF16_String (buf, saccb->ccb_Y));

  printf ("%sccb_hdx:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_hdx);
  printf ("%sccb_hdy:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_hdy);
  printf ("%sccb_vdx:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_vdx);
  printf ("%sccb_vdy:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_vdy);
  printf ("%sccb_ddx:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_ddx);
  printf ("%sccb_ddy:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_ddy);
  printf ("%sccb_PIXC:\t0x%-10lX\n", gCCBDetailsIndent, saccb->ccb_PPMPC);

  /* If the LDPIX bit is set in the flags, then interpret the bits
   * in the PPMPC field.
   */
#define CCB_LDPPMP 0x01000000
  if (saccb->ccb_Flags & CCB_LDPPMP)
    {
      DumpPIXC ("P-mode 0: ", saccb->ccb_PPMPC & 0xffff, saccb->ccb_Flags);
      printf ("\n");

      DumpPIXC ("P-mode 1: ", (saccb->ccb_PPMPC >> 16) & 0xffff,
                saccb->ccb_Flags);
      printf ("\n");
    }

  /*********************/
  /* Decode PREAMBLE 0 */
  /*********************/
  printf ("%sccb_PRE0:\t0x%-10lX", gCCBDetailsIndent, saccb->ccb_PRE0);
  pnb = gPRE0Bits;
  flags = saccb->ccb_PRE0;
  outCount = 9999;
  for (flagNumber = 0; flagNumber < NUM_PRE0_FLAGS; pnb++, flagNumber++)
    {
      if (flags & pnb->bitValue)
        {
          if (outCount > MAX_FLAG_BITS_PER_LINE)
            {
              outCount = 0;
              printf ("\n%s\t\t\t", gCCBDetailsIndent);
            }

          if (outCount != 0)
            printf (", ");

          printf ("%s", pnb->bitName);
          ++outCount;
        }
    }
  printf ("\n");

  printf ("%s\tBits per pixel = ", gCCBDetailsIndent);
  switch (flags & 7)
    {
    case 0:
      printf ("**reserved**\n");
      break;
    case 1:
      printf ("1\n");
      break;
    case 2:
      printf ("2\n");
      break;
    case 3:
      printf ("4\n");
      break;
    case 4:
      printf ("6\n");
      break;
    case 5:
      printf ("8\n");
      break;
    case 6:
      printf ("16\n");
      break;
    case 7:
      printf ("**reserved**\n");
      break;
    }
  printf ("\n");

  printf ("%sccb_PRE1:\t0x%-10lX", gCCBDetailsIndent, saccb->ccb_PRE1);
  pnb = gPRE1Bits;
  flags = saccb->ccb_PRE1;
  outCount = 9999;
  for (flagNumber = 0; flagNumber < NUM_PRE1_FLAGS; pnb++, flagNumber++)
    {
      if (flags & pnb->bitValue)
        {
          if (outCount > MAX_FLAG_BITS_PER_LINE)
            {
              outCount = 0;
              printf ("\n%s\t\t\t", gCCBDetailsIndent);
            }

          if (outCount != 0)
            printf (", ");

          printf ("%s", pnb->bitName);
          ++outCount;
        }
    }
  printf ("\n");

  printf ("%sccb_Width:\t%-10ld\n", gCCBDetailsIndent, saccb->ccb_Width);
  printf ("%sccb_Height:\t%-10ld\n", gCCBDetailsIndent, saccb->ccb_Height);

#undef MAX_FLAG_BITS_PER_LINE
}

/*******************************************************************************************
 * Routine to dump an AMAP chunk from a streamed ANIM to standard output
 *******************************************************************************************/
static void
DumpSAMAP (long filePosition, StreamAnimMapPtr samap)
{
  long cornerNumber;
  char buf[64];

  printf ("-----------------------------------------------\n");
  printf ("File offset: ");
  if (gHexFposOutputFlag)
    printf ("0x%-10lX\n", filePosition);
  else
    printf ("%-10ld\n", filePosition);

  printf ("Stream time: %-10ld\n", samap->time);
  printf ("Channel num: %-10ld\n", samap->channel);

  printf ("%sduration:\t%ld", gCCBDetailsIndent, samap->duration);

  for (cornerNumber = 0; cornerNumber < 4; cornerNumber++)
    {
      printf ("%s%s:\t%s, %s\n", gCCBDetailsIndent,
              gQuadCornerName[cornerNumber],
              ConvertF16_String (buf, samap->quad[cornerNumber].x),
              ConvertF16_String (buf, samap->quad[cornerNumber].y));
    }
}

/*******************************************************************************************
 * Routine to dump an APOS chunk from a streamed ANIM to standard output
 *******************************************************************************************/
static void
DumpSAPOS (long filePosition, StreamAnimPosPtr sapos)
{
  char buf[64];

  printf ("-----------------------------------------------\n");
  printf ("File offset: ");
  if (gHexFposOutputFlag)
    printf ("0x%-10lX\n", filePosition);
  else
    printf ("%-10ld\n", filePosition);

  printf ("Stream time: %-10ld\n", sapos->time);
  printf ("Channel num: %-10ld\n", sapos->channel);
  printf ("APOS Details:\n%s", gCCBDetailsIndent);

  printf ("duration:\t%10ld\t", sapos->duration);
  printf ("newXValue:\t%s\t", ConvertF16_String (buf, sapos->newXValue));
  printf ("newYValue:\t%s", ConvertF16_String (buf, sapos->newYValue));
}

/*******************************************************************************************
 * Routine to dump a stream file to standard output
 *******************************************************************************************/
static void
DumpStreamFile (char *fileName, FILE *fd, long startTime)
{
  char *buffer;
  long filePosition = 0;
  Boolean fDumping = false;
  Boolean fFillerChunk;
  long nextBlockStart;

  StreamAnimCCBPtr saccb;
  SubsChunkDataPtr chunkPtr;

  buffer = (char *)malloc (MAX_BUFFER_SIZE);
  if (buffer == NULL)
    {
      fprintf (stderr, "DumpStreamFile() - unable to allocate input buffer\n");
      return;
    }
  chunkPtr = (SubsChunkDataPtr)buffer;
  saccb = (StreamAnimCCBPtr)buffer;

  printf ("\n\n  Dump of stream file: %s\n\n", fileName);

  while (1 == fread (buffer, sizeof (SubsChunkData), 1, fd))
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
          if ((filePosition + sizeof (SubsChunkData)) >= nextBlockStart)
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
      if (chunkPtr->chunkType == FILLER_DATA_TYPE)
        fFillerChunk = true;
      else
        fFillerChunk = false;

      /* Begin dumping when we've reached a non-filler chunk whose
       * timestamp meets or exceeds the starting time specified
       * on the command line.
       */
      if ((!fDumping) && (!fFillerChunk) && (chunkPtr->time >= startTime))
        fDumping = true;

      if (fDumping || gCelFileFlag)
        {
          /* Check to see if chunk size is a quadbyte multiple. A well formed
           * stream must have quadbyte multiple sized data for the streamer
           * to work properly.
           */
          if (NOT_QUADBYTE_ALIGNED (chunkPtr->chunkSize))
            printf (
                "Warning: following chunk size not a quadbyte multiple:\n");

          /* See if the chunk's major type matches what we are looking for
           */
          if (chunkPtr->chunkType == SANM_CHUNK_TYPE)
            {
              /* Right major type. Read some more data and see if the subtype
               * is the kind of data we want to dump.
               */
              if (gDumpACCBFlag && (saccb->subChunkType == ACCB_CHUNK_TYPE))
                {
                  if (1
                      != fread (buffer + sizeof (SubsChunkData),
                                sizeof (StreamAnimCCB)
                                    - sizeof (SubsChunkData),
                                1, fd))
                    return;

                  /* Dump the details of the CCB */
                  DumpSACCB (filePosition, (StreamAnimCCBPtr)buffer);
                  printf ("\n");
                }

              else if (gDumpAPOSFlag
                       && (saccb->subChunkType == APOS_CHUNK_TYPE))
                {
                  if (1
                      != fread (buffer + sizeof (SubsChunkData),
                                sizeof (StreamAnimPos)
                                    - sizeof (SubsChunkData),
                                1, fd))
                    return;

                  /* Dump the details of the position chunk */
                  DumpSAPOS (filePosition, (StreamAnimPosPtr)buffer);
                  printf ("\n");
                }

              else if (gDumpAMAPFlag
                       && (saccb->subChunkType == AMAP_CHUNK_TYPE))
                {
                  if (1
                      != fread (buffer + sizeof (SubsChunkData),
                                sizeof (StreamAnimMap)
                                    - sizeof (SubsChunkData),
                                1, fd))
                    return;

                  /* Dump the details of the map chunk */
                  DumpSAMAP (filePosition, (StreamAnimMapPtr)buffer);
                  printf ("\n");
                }
            }

          /* Allow the dumping of non-stream cel files. Here we have to play
           * some games to get the data right so that the rest of the dumping
           * code will work correctly because we're set up to deal with
           * streams, not old fashioned data files.
           */
          else if (gCelFileFlag && (chunkPtr->chunkType == CCB_CHUNK_TYPE))
            {
              /* Back up because we've read past where the CCB would start
               * in a non-stream file. The CCB data will be 8 bytes past the
               * chunk type and chunk size fields, each of which are 4 byte
               * fields.
               */
              fseek (fd, filePosition + 4 + 4, SEEK_SET);
              if (1
                  == fread (&saccb->ccb_Flags,
                            sizeof (StreamAnimCCB) - sizeof (SubsChunkData), 1,
                            fd))
                {
                  /* Zero out fields that would never exist for
                   * the old cel file format so the dump output is clean.
                   */
                  saccb->time = 0;
                  saccb->channel = 0;

                  /* Dump the details of the CCB */
                  DumpSACCB (filePosition, saccb);
                  printf ("\n");
                }
              else
                return;
            }
        }

      /* Position the file to the next chunk in the file
       */
      if (chunkPtr->chunkSize > MAX_OK_CHUNK_SIZE)
        {
          printf ("highly questionable chunk size = %ld\n",
                  chunkPtr->chunkSize);
          return;
        }
      filePosition += chunkPtr->chunkSize;
      fseek (fd, filePosition, SEEK_SET);
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

  DumpStreamFile (gInputStreamName, fd, gDumpStartTime);

  exit (0);
}

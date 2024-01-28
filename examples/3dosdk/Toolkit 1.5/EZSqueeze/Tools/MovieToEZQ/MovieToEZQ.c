/* MovieToFilm.c

        File:			MovieToFilm.c

        Contains:		Main control for compressing "raw " format
   quicktime movies into EZ Squeeze.

        Adapted by:		Bill Duvall

        Copyright:	й 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History:

                8/11/94		Clean up and comments			WSD
                5/1/94 		Adapted from MovieToStream		WSD

        ----------------------------------------------------------------------------------------

        This file is the main control for compressing "raw " quicktime movies
   into EZ Squeeze format. It is an MPW tool.

        To see options, execute "MovieToFilm" with no arguments.

        */

#include "Films.h"
#include "StdLib.h"
#include <CursorCtl.h>
#include <ErrMgr.h>
#include <Errors.h>
#include <FixMath.h>
#include <Memory.h>
#include <Movies.h>
#include <Packages.h>
#include <QuickDraw.h>
#include <Strings.h>
#include <TextUtils.h>
#include <Types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sane.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
/* #include	"VQDecoder.h" */
#include "EZQ_Defs.h"
#include "myGWorld.h"

/* Added by WSD 5/xx/94 */

// Compilation Control

//#define IgnoreSound

#define UseDPCM
#define DebugPrint --enable to print out what is going into frame header, etc.

// Utility Defines

#define Long2Fix(v) (((long)(v)) << 16)
#define Fix2Long(v) (((long)((v) + 0x8000)) >> 16)
#define UFix2Long(v) (((unsigned long)((v) + 0x8000)) >> 16)
#define UFix2X(v)                                                             \
  (((extended)((unsigned long)(v) >> 16))                                     \
   + (((extended)((v)&0xFFFF)) / 32768.0))

#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))

/*  New Timing code by WSD 5/11/94 */

typedef extended ClockType;

#define AudioRate ((ClockType)44100.0)
#define _3DO_AudioTicks ((ClockType)184.0)
#define ClockPrecision                                                        \
  ((ClockType)10E8) /* Allows error of .0.01035391 frames per hour */
#define ClockUnitsPerSecond (AudioRate * ClockPrecision)
#define _3DO_TimeScale (AudioRate / _3DO_AudioTicks)
#define _3DO_TimeBase                                                         \
  ((((ClockType)1.0) / _3DO_TimeScale) * ClockUnitsPerSecond)

#define MakeTimeBase(rate) (ClockUnitsPerSecond / (ClockType)(rate))

typedef struct TrackInfo
{
  OSType mediaType;    // QT media type
  ClockType startTime; // Starting time relative to the movie starting time
  ClockType timeBase;  // Channel Tick value in normalized Clock Ticks
  ClockType duration;  // How long is it??
  long length;         // number of samples or length in track time units
  Media mediaID;
  short channel;
  unsigned char userCode;  // Requested by Lynn Ackler
  unsigned char userFlags; // Requested by Lynn Ackler
} TrackInfo;

#define ChannelParm(trackNo)                                                  \
  ((trackInfo[MovieTrack].channel & 0xFFFF)                                   \
   + ((trackInfo[MovieTrack].userFlags & 0xFF) << 24)                         \
   + ((trackInfo[MovieTrack].userCode & 0xFF) << 16))

// static and global variables

static int frameCounter;
static int blockNumber; /* Used for keeping track of output blocks */

static WindowPtr myCWindow = nil;

#ifdef UseDPCM
#define PROGRAM_VERSION_STRING "1.1-EZQ"

static Boolean dilateTimeFlag = true; // reset with -a (actual time) option
Boolean encodeBlackFlag = true;
static Boolean useMonitorWindow = false; // set with -w option

static int frameLimit = 0;  // set with -x option
static int grandTotalBytes; // to print out overall data rate

static ClockType grandTotalTime; // for data rate printout

short runLiteralThreshold = RunLitThreshold; // Threshold for including literal
                                             // pixels in runs (squeeze.c)

ClockType timeOffset;

int pixelThreshold
    = 0;              // minimum difference between pixels for calculating runs
int pixelBitRate = 4; // Controls what type of compression to use

extended redFactor = 1.0;   // Multiply incoming red value by this number
extended greenFactor = 1.0; // Multiply incoming green value by this number
extended blueFactor = 1.0;  // Multiply incoming blue value by this number

extern void printRuns (FILE *file);

extern Boolean tablesEveryFrame
    = true; // if true, then send a table with each frame -- otherwise, first
            // frame only.
extern Boolean scanErrorTables (char *fileName);

extern long pass1_Ticks; // tickcounts for compression timing
extern long pass2_Ticks; // tickcounts for compression timing
extern long pass3_Ticks; // tickcounts for compression timing
extern long putTime;     // tickcounts for compression timing

extern int maxRedError;     // defined in callSqueeze.c
extern int maxGreenError;   // defined in callSqueeze.c
extern int maxBlueError;    // defined in callSqueeze.c
extern int runNibblesSaved; // defined in putSqueeze.c

#else
#define PROGRAM_VERSION_STRING "2.0"
#endif

#define kPhysBlockSize (64 * 1024)
#define SoundChunkHdrSize ((sizeof (StreamSoundChunk) + 3) & -4) /* WSD */
#define kMinSoundChunkSize (SoundChunkHdrSize + 1024)
#define kMaxTrackCount 50

/******************************************/
/* Command line settable global variables */
/******************************************/

Boolean gEnableMarkerOutput
    = false;              /* default is no marker output file (set by -m) */
Boolean gLoopIt = false;  /* default for movie looping == FALSE */
Boolean gVerbose = false; /* default for verbose output == FALSE */

long gPhysBlockSize = kPhysBlockSize; /* default to 32k blocking factor */
long gFrameRate = 0;                  /* default frame rate == 30fps */
long gVideoChannel = -1;              /* default channel for video == 0 */
long gMarkerRate = 1; /* default markers to 1 second intervals */
Ptr gEmptyPtr;

/* Added (or modified) by WSD 5/xx/94 */

Boolean useAlphaChannel
    = false; /* set true if we should use the alpha channel for masking */
ClockType gStartTime = 0; /* default for a one clip movie */ // lla 12/6/93
ClockType totalMovieTime = 0;
    /* if non-0, then the target output time for film */ // WSD 5/16/94

/* Macro used to make chatty printf's go away unless user
 * specifies the "-v" verbose option.
 */
#define VFPRINTF                                                              \
  if (gVerbose)                                                               \
  fprintf

/* Forward Declarations */

WriteFillerChunk (short filmFile, long remainingBlockSize);

/* Utilities from WSD */

OSErr
truncateAndClose (short vRefNum)
{
  long filePos;
  OSErr oops;
  /* Routine to close AND truncate to current file position added by WSD */

  if ((oops = GetFPos (vRefNum, &filePos)) != 0)
    fprintf (stdout, "\nError in getting current file position = %d", oops);
  else if ((oops = SetEOF (vRefNum, filePos)) != 0)
    fprintf (stdout, "\nError in setting file EOF = %d", oops);
  return FSClose (vRefNum);
}

char *
typeToStr (char *str, long type)
{
  /* Convert 4 character type to string for printing */
  /* str must point to previously allocated buffer for result */

  sprintf (str, "\"%c%c%c%c\"", type >> 24, type >> 16, type >> 8, type);
  return (str);
}

OSErr
writeAndUpdate (short file, long size, void *data, int *countPtr)
{
  OSErr errorCode;
  long writeSize;
  writeSize = size;
  if ((errorCode = FSWrite (file, &writeSize, data)) != noErr)
    return (errorCode);
  if (writeSize != size)
    {
      fprintf (stdout,
               "\tBytes written does not match I/O request (Written = %d, "
               "requested = %d)\n",
               writeSize, size);
      return (OSErr)(-1);
    }
  if (countPtr != nil)
    *countPtr -= writeSize;
  return (noErr);
}

int
adjustOutputBlocking (short file, int size, int *countPtr)
{
  /* Return size of bytes to write (i.e. "chunk" size)
          Need room to write at least size bytes */

  int spaceLeft;

  if (size > *countPtr)
    {
      VFPRINTF (stdout, "\tFiller for %d bytes, block #%d\n", *countPtr,
                blockNumber);
      WriteFillerChunk (file, *countPtr);
      blockNumber++;
      *countPtr = gPhysBlockSize;
    }

  if ((spaceLeft = (*countPtr - size)) < sizeof (FillerChunk))
    {
      size += spaceLeft;
      fprintf (stdout, "\nWasted %d bytes, block #%d\n", spaceLeft,
               blockNumber);
    }

  return (size);
}

/*
        OpenMovie opens a movie file and returns the movie
*/

Movie
OpenMovie (Str255 MovieName)
{
  OSErr Oops;
  FSSpec Spec;
  short resRefNum;
  Movie theMovie;
  short resID;
  Str255 theStr;
  Boolean dataRefWasChanged;

  if (Oops = FSMakeFSSpec (0, 0, MovieName, &Spec))
    {
      fprintf (stdout, "### %d Occurred while FSMakeFSSpec for %P\n", Oops,
               MovieName);
      return 0;
    }
  if (Oops = OpenMovieFile (&Spec, &resRefNum, 0))
    {
      fprintf (stdout, "### %d Occurred while opening %P\n", Oops, MovieName);
      return 0;
    }
  resID = 0;
  if (Oops = NewMovieFromFile (&theMovie, resRefNum, &resID, theStr, 0,
                               &dataRefWasChanged))
    {
      fprintf (stdout, "### %d Occurred while opening movie in %P\n", Oops,
               theStr);
    }
  Oops = CloseMovieFile (resRefNum);

  return theMovie;
}

long
AlignSample (Ptr srcData, Ptr dstData, int width, int height, int imageDepth)
{
/* Added by WSD */
#ifdef UseDPCM
  int bytesCompressed;
  extern void showImage (int width, int height, int depth, long *pixmap,
                         WindowPtr myCWindow);
  extern int compressImage (int width, int height, int depth, char *pixels,
                            char *output, Boolean decimate,
                            DisplayInfo *monitor);
  extern void closePPM ();

  char windowName[256];
  DisplayInfo monitor;
  GrafPtr savePort;
  Rect r;

  frameCounter++;
  if (useMonitorWindow && (myCWindow == nil))
    {
      r.left = r.top = 50;
      r.right = r.left + (width + 2) * 2;
      r.bottom = r.top + (height + 2) * 2;
      sprintf (windowName, "Frame #%d", frameCounter);
      c2pstr (windowName);
      myCWindow
          = NewCWindow (0, &r, windowName, 0xFF, 0, (WindowPtr)-1, 0xFF, 0);
    }

  GetPort (&savePort);
  if (myCWindow != nil)
    {
      SetPort (myCWindow);
      EraseRect (&myCWindow->portRect);
      sprintf (windowName, "Frame #%d", frameCounter);
      c2pstr (windowName);
      SetWTitle (myCWindow, (ConstStr255Param)windowName);
      showImage (width, height, imageDepth, (long *)srcData, myCWindow);
    }

  monitor.window = myCWindow;
  monitor.r = myCWindow->portRect;
  monitor.GWD.initialized = 0;

  bytesCompressed = compressImage (width, height, imageDepth, (char *)srcData,
                                   (char *)dstData, false,
                                   useMonitorWindow ? &monitor : nil);
  SetPort (savePort);
  if ((bytesCompressed) == 0)
    {
      fprintf (stdout, "\nError in compression -- returned nil");
      return (0);
    }
  return bytesCompressed;

#else
  Ptr sd = srcData;
  Ptr dd = dstData;
  long frameType = *(unsigned char *)sd;
  Ptr frameEnd = sd + (*(long *)sd & 0x00FFFFFF);
  long tileCount = ((FrameHeaderPtr)sd)->frameTileCount;
  long tileType, atomType, atomSize, desiredSize, size;
  Ptr tileStart, tileEnd;
  short t;

  frameCounter++; // WSD -- 5/10/94

  *(FrameHeader *)dd = *(FrameHeader *)sd;
  dd += sizeof (FrameHeader);
  *((long *)dd)++ = 0xFE000006;
  *((short *)dd)++ = 0x0000;

  sd += sizeof (FrameHeader);

  // Loop through all tiles and expand every code book

  for (t = 0; t < tileCount; t++)
    {
      // parse atoms within frame looking for tiles

      do
        {
          tileType = *(unsigned char *)sd;
          if (tileType == kTileKeyType8 || tileType == kTileInterType8)
            break;
          sd += *(long *)sd & 0x00FFFFFF;
        }
      while (true);

      // skip tile header and get length of tile atom

      tileStart = dd;
      tileEnd = sd + (*(long *)sd & 0x00FFFFFF);
      *(TileHeader *)dd = *(TileHeader *)sd; // Copy the frame header
      dd += sizeof (TileHeader);
      sd += sizeof (TileHeader);

      // parse atoms within tile looking for codebooks

      do
        {
          atomSize = *(long *)sd & 0x00FFFFFF;
          atomType = *(unsigned char *)sd;
          desiredSize = (atomSize + 3) & 0x00FFFFFC;
          BlockMove (sd, dd, desiredSize);
          *(long *)dd = (atomType << 24) | desiredSize;
          dd += desiredSize;
          sd += *(long *)sd & 0x00FFFFFF;
        }
      while (sd < tileEnd);
      *(long *)tileStart = (tileType << 24) | (dd - tileStart);
    }
  size = (dd - dstData);
  *(long *)srcData = (frameType << 24) | size;
  return size;
#endif
}

char *
CurrTime (char *timeStr)
{
  unsigned long currTime;

  GetDateTime (&currTime);
  IUTimeString (currTime, true, timeStr);
  return p2cstr (timeStr);
}

/*	Write out a chunk of type 'FILL' for the
 *	remaining block data. This is done when no
 *	other block is small enough to fit in the
 *	remaining space.
 */
WriteFillerChunk (short filmFile, long remainingBlockSize)
{
  FillerChunk fill;
  long writeSize;
  OSErr Oops;

  if (remainingBlockSize < sizeof (FillerChunk))
    {
      fprintf (stdout, "### %d Too small for filler chunk --- wrote zeros.\n",
               remainingBlockSize);
      writeSize = remainingBlockSize;
      Oops = FSWrite (filmFile, &writeSize, gEmptyPtr);
      return;
    }

  fill.chunkType = kFillerChunkType;
  fill.chunkSize = remainingBlockSize;

  writeSize = sizeof (FillerChunk);
  if (Oops = FSWrite (filmFile, &writeSize, &fill))
    {
      fprintf (stdout, "### %d Occurred during WriteFillerChunk\n", Oops);
    }

  writeSize = remainingBlockSize - sizeof (FillerChunk);
  Oops = FSWrite (filmFile, &writeSize, gEmptyPtr);
}

/*	Write out a chunk of type 'CTRL' for the
 *	marker chunk. This is done in order to
 *	cause the Data Acq to seek back to the top
 *	of this data stream.
 */
WriteMarkerChunk (short filmFile, long channel, long *remainingBlockSize,
                  long offset)
{
  CtrlMarkerChunk mark;
  long writeSize;
  OSErr Oops;

  if (*remainingBlockSize < sizeof (CtrlMarkerChunk))
    {
      fprintf (stdout,
               "### remainingBlockSize less than CtrlMarkerChunk!!!\n");
      return;
    }

  mark.chunkType = CTRL_CHUNK_TYPE;
  mark.chunkSize = sizeof (CtrlMarkerChunk);
  mark.time = 0;
  mark.chunkFlags = 0;
  mark.channel = channel;
  mark.subChunkType = GOTO_CHUNK_TYPE;
  mark.marker = offset;

  writeSize = sizeof (CtrlMarkerChunk);
  if (Oops = FSWrite (filmFile, &writeSize, &mark))
    {
      fprintf (stdout, "### %d Occurred during WriteMarkerChunk\n", Oops);
    }

  *remainingBlockSize -= sizeof (CtrlMarkerChunk);
}

/* Convert the data if the format is not Twos Complement.
 * The Audio Folio needs samples in Twos Complement format.
 */
CvtToTwosComplement (StreamSoundHeaderPtr pSSndH, unsigned char *soundData,
                     long size)
{
  unsigned char *c;
  long i;

  if ((pSSndH->sampleSize == 8) && (pSSndH->dataFormat == 'raw '))
    {
      c = soundData;
      for (i = 0; i < size; i++)
        {
          *c++ = (*c ^ 0x80);
        }
    }
}

OSErr
DumpMovie (Movie theMovie, Str255 filmFileName, Str255 keyFileName)
{

#define AbortDumpFile(s)                                                      \
  {                                                                           \
    errorStr = (s);                                                           \
    goto DUMP_MOVIE_ABORT;                                                    \
  }
#define MovieTrack 0

  char tempStr[256];  // WSD
  char tempStr1[256]; // WSD
  char *errorStr;     // WSD
  Str255 originalName;

  short FilmFile;
  short firstAudioTrack, trackCount, trackNo, tracksInMovie;
  short defaultFrameRate;
  short sampleFlags;
  short sourceWidth, sourceHeight, imageDepth;

  long n, x;
  long size, remainingBlockSize;
  long SoundMediaCount, VideoMediaCount;

  long audioBytesPerSecond; // WSD
  long videoBytesPerSecond; // WSD

  extended timeWarpFactor; // WSD
  Ptr AlignedData;

  Handle sampledata;

  ClockType audioClock;                              // WSD
  ClockType filmTimeBase, output_Frame_TimeBase;     // WSD
  ClockType outputClock, startClock, outputDuration; // WSD

  FILE *keyFile;
  FilmHeader FilmH;
  FilmFrame FilmF;

  ImageDescription **IDesc;

  OSErr Oops;
  OSType originalManufacturer;

  SoundDescription **SDesc;

  TimeValue trackTime, sampleTime, durationPerSample;
  TimeValue lastMarkerTime = 0;

  Track track;                             // WSD
  TrackInfo trackInfo[kMaxTrackCount + 1]; // WSD: Track 0 is the movie

#ifndef IgnoreSound
  Boolean soundLeft = true;
  long soundScale;
  StreamSoundHeader SSndH;
  StreamSoundChunk SSndC;
  Handle sounddata;
#endif

  /* Create and open the output file, and initialize counter */
  Oops = Create (filmFileName, 0, 'CYBD', 'EZQF');
  if (((Oops = Create (filmFileName, 0, 'CYBD', 'EZQF')) != noErr)
          && (Oops != dupFNErr)
      || ((Oops = FSOpen (filmFileName, 0, &FilmFile)) != noErr))
    {
      fprintf (stdout, "### %d Occurred during Create or Open of %P\n", Oops,
               filmFileName);
      return Oops;
    }

  remainingBlockSize = gPhysBlockSize;
  blockNumber = 0;

  /* Open output marker text file if marker output is enabled */
  if (gEnableMarkerOutput)
    {
      p2cstr (keyFileName);
      if ((keyFile = fopen (keyFileName, "w")) == NULL)
        {
          fprintf (stdout, "### Error opening marker file: %s\n", keyFileName);
          return -1;
        }
    }

  // make sure it doesn't have too many tracks for us to handle
  if ((tracksInMovie = GetMovieTrackCount (theMovie)) >= kMaxTrackCount)
    {
      fprintf (stdout,
               "#### Can't convert this movie, it has more than %d tracks.  "
               "Sorry\n",
               kMaxTrackCount);
      return -1;
    }

  // First, get information about the movie (track 0 in our tables)

  trackInfo[MovieTrack].mediaType = 0;
  trackInfo[MovieTrack].mediaID = nil;
  trackInfo[MovieTrack].timeBase = MakeTimeBase (GetMovieTimeScale (theMovie));
  trackInfo[MovieTrack].length = GetMovieDuration (
      theMovie); /* Not exactly correct, but there is no sample count */
  trackInfo[MovieTrack].duration
      = trackInfo[MovieTrack].timeBase * trackInfo[MovieTrack].length;
  trackInfo[MovieTrack].startTime = 0;
  trackInfo[MovieTrack].channel = (gVideoChannel == -1) ? 0 : gVideoChannel;
  trackInfo[MovieTrack].userCode = 0;
  trackInfo[MovieTrack].userFlags = 0;

  trackCount = MovieTrack + 1;

  SpinCursor (32);

  IDesc = (ImageDescription **)NewHandle (sizeof (ImageDescription));
  SDesc = (SoundDescription **)NewHandle (sizeof (SoundDescription));

  SoundMediaCount = 0;
  VideoMediaCount = 0;
  defaultFrameRate = 0; // Fix2Long(GetMoviePreferredRate(theMovie));

  firstAudioTrack = 0;

  fprintf (stdout, "\n\tTrack         ID     mediaType     Start Time     "
                   "Time Scale     Duration     Type\n");

  fprintf (
      stdout, "\t%5d%11s%14s%15d%15.2f%13d%9s\n", MovieTrack, "Movie", "<NA>",
      (long)(trackInfo[MovieTrack].startTime / trackInfo[MovieTrack].timeBase),
      ClockUnitsPerSecond / trackInfo[MovieTrack].timeBase,
      (long)(trackInfo[MovieTrack].duration / trackInfo[MovieTrack].timeBase),
      "<NA>");

  for (n = 1; n <= tracksInMovie; n++)
    {
      track = GetMovieIndTrack (theMovie, n);
      trackInfo[n].mediaID = GetTrackMedia (track);
      GetMediaHandlerDescription (trackInfo[n].mediaID,
                                  &trackInfo[n].mediaType, originalName,
                                  &originalManufacturer);
      trackInfo[n].startTime
          = GetTrackOffset (track) * trackInfo[MovieTrack].timeBase;
      trackInfo[n].length = GetMediaSampleCount (trackInfo[n].mediaID);
      trackInfo[n].channel = (gVideoChannel == -1) ? 0 : gVideoChannel;
      trackInfo[n].userCode = 0;
      trackInfo[n].userFlags = 0;

      if (trackInfo[n].mediaType == VideoMediaType)
        {
          trackInfo[n].timeBase
              = MakeTimeBase (GetMediaTimeScale (trackInfo[n].mediaID));
          VideoMediaCount++;
          GetMediaSampleDescription (trackInfo[n].mediaID, 1,
                                     (SampleDescriptionHandle)IDesc);
          if ((*IDesc)->cType != 'raw ')
            {
              fprintf (
                  stdout,
                  "\t\t#### Track %d type is \"%s\", which is not supported\n",
                  n, typeToStr (tempStr, (*IDesc)->cType));
              fprintf (stdout, "\t\t#### Remake the movie in video type \"raw "
                               "\" and try again.\n");
              return -1;
            }

          /*
          fprintf(stdout, "\t*** Duration = %d, timeBase = %6.2g, length = %d,
          rate = %d***\n", GetMediaDuration(trackInfo[n].mediaID),
          trackInfo[n].timeBase, trackInfo[n].length,
                  (long)(ClockUnitsPerSecond/
          ((GetTrackDuration(track)*trackInfo[MovieTrack].timeBase -
          trackInfo[n].startTime)/trackInfo[n].length)
                          ));
                  */
          if (defaultFrameRate == 0)
            defaultFrameRate
                = ClockUnitsPerSecond
                  / ((GetTrackDuration (track) * trackInfo[MovieTrack].timeBase
                      - trackInfo[n].startTime)
                     / trackInfo[n].length); /* Choose first video rack rate
                                                for default */
        }
      else if (trackInfo[n].mediaType == SoundMediaType)
#ifdef IgnoreSound
        {
          trackInfo[n].timeBase = 0;
          fprintf (stdout, "\t*** Audio Track %n Ignored (%P)\n", n,
                   originalName);
        }
#else
        {
          if (firstAudioTrack == 0)
            firstAudioTrack = n;
          SoundMediaCount++;
          GetMediaSampleDescription (trackInfo[n].mediaID, 1,
                                     (SampleDescriptionHandle)SDesc);
          trackInfo[n].timeBase = MakeTimeBase (
              UFix2X ((*SDesc)->sampleRate)); /* Use actual sample rate instead
                                                 of media scale */
        }
#endif

      trackInfo[n].duration
          = trackInfo[n].timeBase * GetMediaDuration (trackInfo[n].mediaID);

      fprintf (stdout, "\t%5d%11X%14s%15d%15.2f%13d%9s\n", n,
               trackInfo[n].mediaID,
               typeToStr (tempStr1, trackInfo[n].mediaType),
               (long)(trackInfo[n].startTime / trackInfo[MovieTrack].timeBase),
               ClockUnitsPerSecond / trackInfo[n].timeBase,
               (long)(trackInfo[n].duration / trackInfo[n].timeBase),
               (trackInfo[n].mediaType == VideoMediaType)
                   ? typeToStr (tempStr, (*IDesc)->cType)
                   : "<NA>");
    }

  if (defaultFrameRate == 0)
    {
      fprintf (stdout, "\n##### Default frame could not be set -- assumed to "
                       "be 20 #####\n\n");
      defaultFrameRate = 20;
    }

  if (gFrameRate == 0)
    gFrameRate = defaultFrameRate;

  fprintf (stdout, "\n\tBegin Compression...\n");

  VFPRINTF (stdout, "\tDefault Frame Rate = %d, Output Frame Rate = %d\n",
            defaultFrameRate, gFrameRate);

  output_Frame_TimeBase = MakeTimeBase (gFrameRate);

  timeWarpFactor = 0;

  if (totalMovieTime != 0)
    {
      if (frameLimit
          != 0) // Force output to be exactly totalMovieTime in duration
        {
          timeWarpFactor
              = totalMovieTime / (frameLimit * output_Frame_TimeBase);
          output_Frame_TimeBase *= timeWarpFactor;
        }
      else
        {
          timeWarpFactor = totalMovieTime / trackInfo[MovieTrack].duration;
          dilateTimeFlag = true;
        }
      fprintf (stdout,
               "\tTime Warp Factor set to %6.2f to make output movie %6.2f "
               "seconds long\n",
               timeWarpFactor, totalMovieTime / ClockUnitsPerSecond);
    }

  if (dilateTimeFlag)
    {
      if (timeWarpFactor == 0)
        timeWarpFactor = (extended)defaultFrameRate / (extended)gFrameRate;
      else
        {
          gFrameRate = ((ClockType)defaultFrameRate) / timeWarpFactor;
          fprintf (stdout,
                   "\tFrame rate set to %d to make movie %6.2f seconds long\n",
                   gFrameRate, totalMovieTime / ClockUnitsPerSecond);
        }

      for (n = 0; n <= tracksInMovie; n++)
        {
          trackInfo[n].timeBase *= timeWarpFactor;
          trackInfo[n].startTime *= timeWarpFactor;
          trackInfo[n].duration *= timeWarpFactor;
        }
    }

  /* Now set up the output timing parameters */

  outputDuration = trackInfo[MovieTrack].duration;
  if (frameLimit != 0)
    outputDuration = Min (gStartTime * ClockUnitsPerSecond
                              + frameLimit * output_Frame_TimeBase,
                          trackInfo[MovieTrack].duration);

  //	Create the film header
  //-----------------------------------------------------------

  FilmH.chunkType = kVideoChunkType;
  FilmH.chunkSize = sizeof (FilmHeader);
  FilmH.subChunkType = kVideoHeaderType;
  FilmH.time = 0;
  FilmH.chunkFlags = 0;
  FilmH.channel
      = (gVideoChannel == -1) ? ChannelParm (trackNo) : gVideoChannel;
  FilmH.version = 0;
  FilmH.cType = (*IDesc)->cType;
  FilmH.height = (Min (MaxDisplayHeight, sourceHeight = (*IDesc)->height))
                 & -2; // must be even
  FilmH.width = (Min (MaxDisplayWidth, sourceWidth = (*IDesc)->width))
                & -WidthModulus; // must be multiple of WidthModulus
  FilmH.scale = _3DO_TimeScale;
  FilmH.count = outputDuration / _3DO_TimeBase;

  filmTimeBase = MakeTimeBase (_3DO_TimeScale);
  imageDepth = (*IDesc)->depth;

  fprintf (stdout, "\tDepth = %d\n", imageDepth);

  // And create a partial film frame header of fields which don't change

  FilmF.chunkType = kVideoChunkType;
  FilmF.chunkFlags = kKeyChunkFlag;
  FilmF.subChunkType = kVideoKeyFrameType;
  FilmF.channel
      = (gVideoChannel == -1) ? ChannelParm (trackNo) : gVideoChannel;

  // Now print out some progress and diagnostic information

  fprintf (stdout, "\tCompressing %d X %d", sourceWidth, sourceHeight);
  if ((sourceWidth != FilmH.width) || (sourceHeight != FilmH.height))
    fprintf (stdout, " as %d X %d image\n", FilmH.width, FilmH.height);
  else
    fprintf (stdout, " image\n", FilmH.width, FilmH.height);
  VFPRINTF (stdout, "\t  Output frame Rate = %d\n", gFrameRate);
  VFPRINTF (stdout, "\t  movieDuration: %d\n", trackInfo[MovieTrack].length);
  VFPRINTF (stdout, "\t  Output Duration: %d frames = %5.1f Seconds\n",
            (long)(outputDuration / output_Frame_TimeBase),
            outputDuration / ClockUnitsPerSecond);
  fflush (stdout);

  DisposeHandle ((Handle)IDesc);

  SpinCursor (32);

  //	Write the Film Header to disk
  //-----------------------------------------------------------
  if (Oops = writeAndUpdate (FilmFile, sizeof (FilmHeader), &FilmH,
                             &remainingBlockSize))
    AbortDumpFile ("writeAndUpdate");

    //	Create the Streamed Sound Header
    //-----------------------------------------------------------

#ifndef IgnoreSound

  if (SoundMediaCount > 0)
    {
      /* Cycle through each audio track and determine the number of frames in
       * each track's media. */

      VFPRINTF (stdout, "\t  SDesc dataFormat: '%.4s'\n",
                &((*SDesc)->dataFormat));
      VFPRINTF (stdout, "\t  SDesc numChannels: %d\n", (*SDesc)->numChannels);
      VFPRINTF (stdout, "\t  SDesc sampleSize: %d\n", (*SDesc)->sampleSize);
      VFPRINTF (stdout, "\t  SDesc packetSize: %d\n", (*SDesc)->packetSize);
      VFPRINTF (stdout, "\t  SDesc sampleRate: %d\n",
                (((UInt32)(*SDesc)->sampleRate) >> 16));

      sounddata = NewHandle (0);

      SSndH.chunkType = kSoundChunkType;
      SSndH.chunkSize = sizeof (StreamSoundHeader);
      SSndH.subChunkType = kSoundHeaderType;
      SSndH.time = 0;
      SSndH.channel
          = (gVideoChannel == -1) ? ChannelParm (trackNo) : gVideoChannel;
      SSndH.dataFormat = (*SDesc)->dataFormat;
      SSndH.numChannels = (*SDesc)->numChannels;
      SSndH.sampleSize = (*SDesc)->sampleSize;
      SSndH.sampleRate = ((UInt32)(*SDesc)->sampleRate);
      SSndH.sampleCount
          = GetMediaSampleCount (trackInfo[firstAudioTrack].mediaID);

      VFPRINTF (stdout, "\t  %d sound samples at %d khz\n", SSndH.sampleCount,
                soundScale);

      if (Oops = writeAndUpdate (FilmFile, sizeof (StreamSoundHeader), &SSndH,
                                 &remainingBlockSize))
        AbortDumpFile ("writeAndUpdate");
    }

#endif

  DisposeHandle ((Handle)SDesc);

  //	Prepare to Copy all the audio samples and video frames as chunks

  sampledata = NewHandle (0);
  if ((AlignedData = malloc (gPhysBlockSize)) == nil)
    return memFullErr;

  // ***START OF MAIN OUTPUT LOOP ****

  startClock
      = Min (outputDuration, ((ClockType)gStartTime) * ClockUnitsPerSecond);
  grandTotalBytes = 0;
  outputClock = startClock;

  while (outputClock < outputDuration)
    {
      /* Write out one second of sound followed by one second of video. */

      videoBytesPerSecond = 0;
      audioBytesPerSecond = 0;

      runNibblesSaved = 0;
      pass2_Ticks = 0;
      pass3_Ticks = 0;
      putTime = 0;

      // AUDIO LOOP -- Write out 1 second of Audio

#ifdef IgnoreSound
      audioClock = Min (outputDuration, outputClock + ClockUnitsPerSecond);
#else
      {
        ClockType audioDuration;
        long sampleCount;

        audioClock = Min (outputDuration, outputClock + ClockUnitsPerSecond);

        for (trackNo = tracksInMovie; trackNo > 0; trackNo--)
          if ((trackInfo[trackNo].mediaType == SoundMediaType)
              && (trackInfo[trackNo].startTime < audioClock)
              && ((trackInfo[trackNo].startTime + trackInfo[trackNo].duration)
                  > outputClock))
            {

              /* ******* THIS CODE HAS NEVER BEEN TESTED ******* */

              /* This sound track is current -- we need to convert some samples
               */

              audioDuration
                  = Min (audioClock, trackInfo[trackNo].startTime
                                         + trackInfo[trackNo].duration)
                    - outputClock;

              /* Perform necessary block boundary adjustments for output */

              size = (adjustOutputBlocking (FilmFile, kMinSoundChunkSize,
                                            &remainingBlockSize)
                      - SoundChunkHdrSize)
                     & -4;

              sampleCount
                  = Min (size / ((SSndH.sampleSize / 8) * SSndH.numChannels),
                         (long)(audioDuration / trackInfo[trackNo].timeBase));

              trackTime = (outputClock - trackInfo[trackNo].startTime)
                          / trackInfo[trackNo].timeBase;

              Oops = GetMediaSample (trackInfo[trackNo].mediaID, sounddata, 0,
                                     &size, trackTime, &sampleTime, 0, 0, 0,
                                     sampleCount, &sampleCount, &sampleFlags);
              if (Oops)
                AbortDumpFile ("GetMediaSample for sound");

              if (audioClock < startClock)
                continue;

              size = (size + 3) & -4; // force it to a longword boundary

              VFPRINTF (stdout, "trackTime %3d, sampleTime %6d, Size %5d\n",
                        trackTime, sampleTime, size);

              /* Prepare the sound chunk header */

              SSndC.chunkType = kSoundChunkType;
              SSndC.chunkSize = sizeof (StreamSoundChunk) + size;
              SSndC.subChunkType = kSoundSampleType;
              SSndC.channel = 0; // (gVideoChannel ==
                                 // -1)?ChannelParm(trackNo):gVideoChannel;
              SSndC.time = (trackInfo[trackNo].startTime
                            + sampleTime * trackInfo[trackNo].timeBase)
                           / filmTimeBase;
              /* SSndC.numSamples	= sampleCount; еее might need this
               * someday... */

              /* Convert the data if the format is not Twos Complement.
                      The Audio Folio needs samples in Twos Complement format
               */

              CvtToTwosComplement (&SSndH, *sounddata, size);

              /* Write out the Sound Chunk Header and sample Data. */

              if (Oops = writeAndUpdate (FilmFile, SoundChunkHdrSize, &SSndC,
                                         &remainingBlockSize))
                AbortDumpFile ("Write Sound Chunk Header");

              if (Oops = writeAndUpdate (FilmFile, size, *sounddata,
                                         &remainingBlockSize))
                AbortDumpFile ("Write Sound Data");

              audioBytesPerSecond += SoundChunkHdrSize + size;
            }
      }
#endif

      //	VIDEO LOOP -- Write out 1 second of video

      if (!gVerbose)
        fprintf (stdout, "\tSecond #%d ",
                 (long)((outputClock + output_Frame_TimeBase - 1
                         + ClockUnitsPerSecond / 2)
                        / ClockUnitsPerSecond));
      for (; (outputClock < audioClock) && (outputClock < outputDuration);
           outputClock += output_Frame_TimeBase)
        {
          SpinCursor (32);
          if (!gVerbose)
            fprintf (stdout, ".");
          /* Get the next video chunk from any media based on the current time
                  There may be more than one piece of media that has this time
             in it. Take the last one.
                  */

          for (trackNo = tracksInMovie; trackNo > 0; trackNo--)
            if ((trackInfo[trackNo].mediaType == VideoMediaType)
                && (trackInfo[trackNo].startTime <= outputClock))
              break; // take the latest one

          trackTime = (outputClock - trackInfo[trackNo].startTime)
                      / trackInfo[trackNo].timeBase;

          VFPRINTF (stdout, "\n\tFrame %d at movie time %d, Track time %d; ",
                    (long)(outputClock / output_Frame_TimeBase),
                    (long)(outputClock / trackInfo[MovieTrack].timeBase),
                    trackTime);
          VFPRINTF (stdout, "remainingBlockSize = %d/%d\n", remainingBlockSize,
                    gPhysBlockSize);

          Oops = GetMediaSample (trackInfo[trackNo].mediaID, sampledata, 0,
                                 &size, trackTime, &sampleTime,
                                 &durationPerSample, 0, 0, 1, 0, &sampleFlags);

          if (Oops)
            {
              sprintf (
                  tempStr,
                  "GetMediaSample (Track=%d, track time=%d, movie time=%d)", x,
                  trackTime,
                  (long)(outputClock / trackInfo[MovieTrack].timeBase));
              errorStr = tempStr;
              goto DUMP_MOVIE_EXIT; /* Shut down normally */
            }

          if ((size = AlignSample (*sampledata, AlignedData, sourceWidth,
                                   sourceHeight, imageDepth))
              == 0)
            goto DUMP_MOVIE_EXIT;

          VFPRINTF (
              stdout,
              "\t\tMedia Time %3d, sampleTime %6d, Duration %3d, Size %5d\n",
              trackTime, sampleTime, durationPerSample, size);

          /* Create a film frame header */

          FilmF.time
              = (timeOffset + trackInfo[trackNo].startTime
                 + sampleTime * trackInfo[trackNo].timeBase - startClock)
                / filmTimeBase;
          FilmF.duration = (durationPerSample * trackInfo[trackNo].timeBase)
                           / filmTimeBase;
          FilmF.frameSize = size;
          FilmF.chunkSize = sizeof (FilmFrame) + FilmF.frameSize;

#ifndef UseDPCM
          // lla 1/25/94 -- distinguish between key frames and difference
          // frames
          if (sampleFlags == mediaSampleNotSync)
            {
              FilmF.chunkFlags = kDependentChunkFlag;
              FilmF.subChunkType = kVideoDifferenceFrameType;
            }
          else
            {
              FilmF.chunkFlags = kKeyChunkFlag;
              FilmF.subChunkType = kVideoKeyFrameType;
            }
#endif

#ifdef DebugPrint
          VFPRINTF (stdout,
                    "\t\tFilm Time = %d, Channel = %d, duration = %d, "
                    "frameSize = %d\n\t\tchunkType = %c%c%c%c, chunkSize = "
                    "%d, chunkFlags = %08X, subChunkType = %c%c%c%c\n",
                    FilmF.time, FilmF.channel, FilmF.duration, FilmF.frameSize,
                    FilmF.chunkType >> 24, FilmF.chunkType >> 16,
                    FilmF.chunkType >> 8, FilmF.chunkType, FilmF.chunkSize,
                    FilmF.chunkFlags, FilmF.subChunkType >> 24,
                    FilmF.subChunkType >> 16, FilmF.subChunkType >> 8,
                    FilmF.subChunkType);
          fflush (stdout);
#endif

          // Note the occurances of key frames...per marker rate

          if (gEnableMarkerOutput && (sampleFlags != mediaSampleNotSync))
            {
              // added lla 12/5/93
              if ((FilmF.time - lastMarkerTime) >= (gMarkerRate * 240))
                {
                  // lla 12/6/93		fprintf( keyFile, "markertime %d\n",
                  // FilmF.time );
                  fprintf (keyFile, "markertime %d\n",
                           FilmF.time + gStartTime);
                  VFPRINTF (stdout,
                            "\t\t\tmarkertime %d, lastMarkerTime <%d>\n",
                            FilmF.time, lastMarkerTime);
                  lastMarkerTime = FilmF.time;
                }
              else if (FilmF.time == 0 && gStartTime != 0)
                {
                  fprintf (keyFile, "markertime %d\n",
                           FilmF.time + gStartTime);
                  VFPRINTF (stdout,
                            "\t\t\tmarkertime %d, lastMarkerTime <%d>\n",
                            FilmF.time, lastMarkerTime);
                  lastMarkerTime = FilmF.time;
                }
              //------------------
            }

          /* Perform necessary block boundary adjustments for output */

          FilmF.chunkSize = adjustOutputBlocking (FilmFile, FilmF.chunkSize,
                                                  &remainingBlockSize);

          /* Write out the Film Frame Header and Data. */

          if (Oops = writeAndUpdate (FilmFile, sizeof (FilmFrame), &FilmF,
                                     &remainingBlockSize))
            AbortDumpFile ("Write Film Frame Header");

          if (Oops
              = writeAndUpdate (FilmFile, FilmF.chunkSize - sizeof (FilmFrame),
                                AlignedData, &remainingBlockSize))
            AbortDumpFile ("Write Film Frame Data");
          videoBytesPerSecond += FilmF.chunkSize;
        };

      grandTotalBytes += videoBytesPerSecond + audioBytesPerSecond;
      grandTotalTime = outputClock;

      VFPRINTF (stdout, "\n\tSecond #%d",
                (long)((outputClock + output_Frame_TimeBase - 1
                        + ClockUnitsPerSecond / 2)
                       / ClockUnitsPerSecond));

      runNibblesSaved /= 2;
      fprintf (stdout,
               "\: %d bytes, (audio:video %d:%d), Bytes saved by runs = %d\n",
               audioBytesPerSecond + videoBytesPerSecond, audioBytesPerSecond,
               videoBytesPerSecond, runNibblesSaved);
      VFPRINTF (stdout,
                "\tFrame Time = %5.1f, Pass1 time = %5.1f, Pass2 time = "
                "%5.1f, , Pass3 time = %5.1f, put Time = %5.1f\n",
                (extended)(pass2_Ticks + pass1_Ticks + pass3_Ticks + putTime)
                    / 60.0,
                ((extended)pass1_Ticks) / 60.0, ((extended)pass2_Ticks) / 60.0,
                ((extended)pass3_Ticks) / 60.0, (extended)putTime / 60.0);

      runNibblesSaved = 0;

    } /* End of main output loop */

  Oops = noErr;

DUMP_MOVIE_EXIT: // WSD
  /* Write out a MRKR control chunk to send this stream back to the top */
  if (gLoopIt)
    WriteMarkerChunk (FilmFile, FilmH.channel, &remainingBlockSize, 0);

  /* Make sure we have a file containing complete phys. blocks */
  if (remainingBlockSize)
    WriteFillerChunk (FilmFile, remainingBlockSize);

DUMP_MOVIE_ABORT:
  if (Oops != noErr)
    fprintf (stdout,
             "\n### OS Error #%d in building file %P (Operation: %s)\n", Oops,
             filmFileName, errorStr ? errorStr : "Unknown");

  /* Close the FilmFile */ // WSD
  truncateAndClose (FilmFile);

  /* Close the marker output file if we opened one */
  if (gEnableMarkerOutput)
    fclose (keyFile);

  closePPM ();
  if (myCWindow != nil)
    {
      DisposeWindow (myCWindow);
      myCWindow = nil;
    }

  DisposHandle (sampledata);
  free (AlignedData);

  return Oops;
}

/*
 * MakeDestPath - make the movie destination name by pulling the source file
 *name from it's full path and adding the name to the destination folder path.
 *
 *		sourceFile	- Src Disk:Folder1:Folder2:source movie
 *		destPath	- Dest Disk:Films:
 *	gives:
 *					- Dest Disk:Films:source movie.EZQF
 */
void
MakeDestPath (StringPtr destPath, StringPtr sourceFile, StringPtr destDir,
              StringPtr newFileExt)
{
  short srcLastColon, destDirLen, fileNameLen;

  // first, find the last colon in the source file name
  for (srcLastColon = StrLength (sourceFile);
       srcLastColon > 0 && sourceFile[srcLastColon] != ':'; srcLastColon--)
    ;

  // do we have a dest path?  if not, copy the path of the original
  if ((srcLastColon > 0) && (StrLength (destDir) == 0))
    {
      BlockMove (sourceFile, destDir, srcLastColon + 1);
      destDir[0] = srcLastColon;
    }

  // make sure the dest path is a valid directory (last char = ':')
  destDirLen = *destDir;
  if (destDir[destDirLen] != ':')
    {
      destDir[++destDirLen] = ':';
      *destDir = destDirLen;
    }

  // construct the dest file
  BlockMove (destDir, destPath, destDirLen + 1);
  fileNameLen = sourceFile[0] - srcLastColon;
  BlockMove (&sourceFile[srcLastColon + 1], &destPath[destDirLen + 1],
             fileNameLen);
  destPath[0] += fileNameLen;

  // and finally, append the file extension
  BlockMove (&newFileExt[1], &destPath[destPath[0] + 1],
             StrLength (newFileExt));
  destPath[0] += StrLength (newFileExt);
}

/*
 * Usage - output command line options and flags
 */
static void
Usage (char *toolName)
{
  fprintf (stdout, "# Usage - %s\n", toolName);
  fprintf (stdout, "             [-a Alpha Channel Masking {off}]\n");
  fprintf (stdout, "             [-b blocking size]\n");
  fprintf (stdout, "             [-c channel no. {0}]\n");
  fprintf (stdout, "             [-d <outputDirectory]\n");
  fprintf (stdout, "             [-e Elapsed Time min:sec.msec]\n");
  fprintf (stdout, "             [-f frame rate (fps) {30}]\n");
  fprintf (stdout, "             [-g Don't encode black runs\n");
  fprintf (stdout, "             [-h Half Pixel Rate (approx 2.3 bits)]\n");
  fprintf (stdout, "             [-k Kill Frames {frame skipping on}]\n");
  fprintf (stdout, "             [-m marker rate {sec)]\n");
  fprintf (stdout,
           "             [-n Normalizing Factors (RGB) {1.0, 1.0, 1.0}]\n");
  fprintf (stdout, "             [-o Time offset {%d}]\n", 0);
  fprintf (stdout, "             [-p Pixel Run Threshold {%d}]\n",
           pixelThreshold);
  fprintf (stdout, "             [-q Error Quotient Tables]\n");
  fprintf (stdout, "             [-r Run Literal Threshold {%d}]\n",
           runLiteralThreshold);
  fprintf (stdout, "             [-s start time (seconds) {0}]\n");

  fprintf (stdout, "             [-t Elapsed Movie TIme (seconds) {0}]\n");

  fprintf (stdout, "             [-u Use Tables First Frame Only]\n");

#ifdef UseDPCM
  fprintf (stdout, "             [-w Open Monitor Windows {false}]\n");
  fprintf (stdout, "             [-x frame limit {infinity}]\n");
  fprintf (stdout, "             [-z R G B Error Thresholds {3 3 3}]\n");
#endif

  fprintf (stdout, "             [movieFile(s)╔].\n");
  fprintf (stdout, "# Version %s\n", PROGRAM_VERSION_STRING);
}

// Do the conversion
main (int argc, char *argv[])
{
  long status;
  long parms;
  long files;
  Movie theMovie;
  Str255 newFilmName = "\p";
  Str255 destFilmDir = "\p";
  Str255 keyFileName = "\p";
  Str255 timeStr;
  char *srcMovieName;
  char *pEnd;

  int elapsedMin, elapsedSec, elapsedms;

  status = files = 0;

  /* So we can spin cursor */
  InitCursorCtl (NULL);

  /* Set up rounding so that we can predict the errors */

  setround (TOWARDZERO);

  for (parms = 1; parms < argc; parms++)
    {
      if (*argv[parms] == '?')
        {
          Usage (argv[0]);
          return EXIT_FAILURE;
        }
      else if (*argv[parms] != '-')
        {
          // not an option? must be a file name.  move it down so all file
          // names are sequential
          argv[++files] = argv[parms];
        }
      else
        {
          if (strlen (argv[parms]) == 2)
            switch (tolower (*(argv[parms] + 1)))
              {
              case 'a':
                useAlphaChannel = true;
                break;
              case 'b':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid physical block size.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }

                gPhysBlockSize = strtol (argv[parms], &pEnd, 10);
                break;
              case 'c':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid channel number.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                gVideoChannel = strtol (argv[parms], &pEnd, 10);
                break;
              case 'd':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid output directory.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }

                strcpy (destFilmDir, argv[parms]);
                c2pstr (destFilmDir);
                break;
              case 'e': // -e min:sec:msec -- set elapsed time for output
                if ((++parms) >= argc)
                  {
                    fprintf (stdout,
                             "### %s - Missing elapsed time (min:sec:msec)\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                elapsedMin = elapsedSec = elapsedms = 0;
                switch (sscanf (argv[parms], "%d:%d.%d", &elapsedMin,
                                &elapsedSec, &elapsedms))
                  {
                  case 0:
                  case 1:
                    fprintf (stdout,
                             "### %s - Missing or bad format elapsed time "
                             "(min:sec.msec)\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }

                totalMovieTime
                    = ((ClockType)(elapsedMin * 60 + elapsedSec))
                          * ClockUnitsPerSecond
                      + (elapsedms * ClockUnitsPerSecond) / (ClockType)1000;
                break;
              case 'f':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid frame rate.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }

                gFrameRate = strtol (argv[parms], &pEnd, 10);
                break;
              case 'g':
                encodeBlackFlag = false;
                break;
              case 'h':
                pixelBitRate = 2;
                break;
              case 'k':
                dilateTimeFlag = false;
                break;
              case 'l':
                gLoopIt = true;
                break;
              case 'm':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid marker rate.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                gEnableMarkerOutput = true;
                gMarkerRate = strtol (argv[parms], &pEnd, 10);
                break;
              case 'n': // -n R G B <normalizing factors>
                if ((++parms + 2) >= argc)
                  {
                    fprintf (stdout, "### %s - Missing normalizing factors.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                redFactor = strtod (argv[parms], &pEnd);
                greenFactor = strtod (argv[++parms], &pEnd);
                blueFactor = strtod (argv[++parms], &pEnd);
                break;

              case 'o':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - Missing time offset.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                timeOffset = strtod (argv[parms], &pEnd);
                timeOffset *= ClockUnitsPerSecond;

                break;

              case 'p':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid Pixel Threshold.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                pixelThreshold = strtol (argv[parms], &pEnd, 10);
                VFPRINTF (stdout, "\tPixel Threshold set to %d\n",
                          pixelThreshold);
                break;
              case 'q':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - invalid Error Quotient Name\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                if (!scanErrorTables (argv[parms]))
                  fprintf (
                      stdout,
                      "\tError in scanning error quotients from file %s\n",
                      argv[parms]);
                break;
              case 'r':
                if (++parms == argc)
                  {
                    fprintf (stdout,
                             "### %s - invalid Run Literal Threshold.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                runLiteralThreshold = strtol (argv[parms], &pEnd, 10) & -4;
                fprintf (stdout, "\tRun Literal Threshold set to %d\n",
                         runLiteralThreshold);
                break;
              case 's':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - Missing start time.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                gStartTime = strtod (argv[parms], &pEnd);
                break;

              case 't':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - Missing Duration\n", argv[0]);
                    goto INVALID_ARG;
                  }
                totalMovieTime
                    = strtod (argv[parms], &pEnd) * ClockUnitsPerSecond;
                break;
              case 'u':
                tablesEveryFrame = !tablesEveryFrame;
                break;
              case 'v':
                gVerbose = true;
                break;
#ifdef UseDPCM
              case 'w':
                useMonitorWindow = true;
                break;
              case 'x':
                if (++parms == argc)
                  {
                    fprintf (stdout, "### %s - Missing Frame Limit.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                frameLimit = strtol (argv[parms], &pEnd, 10);
                break;
              case 'z': // -z R G B <error Thresholds>
                if ((++parms + 2) >= argc)
                  {
                    fprintf (stdout, "### %s - Missing RGB Thresholds.\n",
                             argv[0]);
                    goto INVALID_ARG;
                  }
                maxRedError = strtol (argv[parms], &pEnd, 10);
                maxGreenError = strtol (argv[++parms], &pEnd, 10);
                maxBlueError = strtol (argv[++parms], &pEnd, 10);
                break;
#endif
              default:
              INVALID_OPTION:
                fprintf (stdout, "### %s - \"%s\" is not an option.\n",
                         argv[0], argv[parms]);
              INVALID_ARG:
                Usage (argv[0]);
                return EXIT_FAILURE;
              }
          else
            goto INVALID_OPTION;
        }
    }

  if (pixelBitRate == 2)
    {
      pixelThreshold = -1;
      VFPRINTF (stdout, "\tPixel Threshold set to %d\n", pixelThreshold);
      fprintf (stdout,
               "\tPixel Run encoding disabled for 2 Bit per Pixel mode\n");
    }
  // process the files
  if (files == 0)
    {
      fprintf (stdout, "### No files specified\n");
      Usage (argv[0]);
    }
  else
    {
      unsigned long elapsedTime;

      for (parms = 1; parms <= files; parms++)
        {

          InitGraf ((Ptr)&qd.thePort); /* Need an a5 world to use Quicktime */
          srcMovieName = c2pstr (argv[parms]);
          MakeDestPath (newFilmName, srcMovieName, destFilmDir, "\p.EZQF");
          MakeDestPath (keyFileName, srcMovieName, destFilmDir, "\p.KEY");
          EnterMovies ();
          if ((theMovie = OpenMovie (srcMovieName)) != 0)
            {
              gEmptyPtr = NewPtrClear (gPhysBlockSize);
              fprintf (stdout, "%s - Begin converting \"%P\" to \"%P\"...\n\n",
                       CurrTime (timeStr), srcMovieName, newFilmName);
              // Added by WSD to print out some info
              fprintf (stdout,
                       "\tPhysical Block Size = %d, frame Rate = %d, Channel "
                       "= %d, Start Time = %5.1f\n",
                       gPhysBlockSize, gFrameRate, gVideoChannel, gStartTime);
              fprintf (stdout,
                       "\tMarkerOutput %s, MarkerRate = %d, Looping %s, "
                       "elapsed Time = %6.2f Seconds\n",
                       gEnableMarkerOutput ? "On" : "Off", gMarkerRate,
                       gLoopIt ? "On" : "Off",
                       totalMovieTime / ClockUnitsPerSecond);
              fprintf (stdout,
                       "\tRun Error Threshold = %d, ###%d Bits per "
                       "Pixel###\n\n*** Send Table%s Frame ***\n\n",
                       pixelThreshold, pixelBitRate,
                       tablesEveryFrame ? "s with Each" : " with First");
              elapsedTime = -clock (); // WSD
              DumpMovie (theMovie, newFilmName, keyFileName);
              elapsedTime += clock (); // WSD

              fprintf (stdout, "\n%s - Finished converting \"%P\"\n",
                       CurrTime (timeStr), newFilmName);

              grandTotalTime
                  = (grandTotalTime - gStartTime * ClockUnitsPerSecond)
                    / ClockUnitsPerSecond;
              if (grandTotalTime <= 0)
                grandTotalTime = 10e-20; // prevent divide by 0
              fprintf (stdout,
                       "\nConverted %d frames in %d.%d seconds at an average "
                       "data rate of %d bytes/second\n", // WSD
                       frameCounter, elapsedTime / 60,
                       ((elapsedTime % 60) * 100) / 60,
                       (long)(grandTotalBytes / grandTotalTime)); // WSD

              DisposPtr (gEmptyPtr);
            }
          ExitMovies ();
        }
    }
  return EXIT_SUCCESS;
}

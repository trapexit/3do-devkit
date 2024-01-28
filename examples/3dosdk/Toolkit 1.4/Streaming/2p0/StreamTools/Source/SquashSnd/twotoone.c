/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1
 *with squareroot-delta-exact
 **							encoding and write an AIFC file.
 *Copy all other chunks *
 *and encode samples at Markers as exacts for looping etc.
 **
 **	This File:		twotoone.c
 **
 ** Contains:		Compression Algorithm
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and
 *proprietary *	information of the 3DO Company and shall not be used by any
 *Person or for any *	purpose except as expressly authorized in writing by
 *the 3DO Company.
 **
 **
 **  History:
 **  -------
 **	 9/10/93	RDG		version 1.2
 **						Spin cursor.
 **	 6/17/93	RDG		version 1.0
 **	 6/17/93	RDG		overhaul to use block reads, encoding,
 *and writes *	 6/15/93	RDG		version .12 *	 6/15/93
 *RDG		include Shayes' fix for the encoder *	 6/15/93	RDG
 *modify to use MacFileIO routines *	 3/10/93	RDG	    Version .11
 **						  -new  support for stereo
 *samples
 **						  -fix  sets the creator and type of the
 *new AIFC file *	 3/10/93	RDG		Created DoMonoEncoding
 *and DoStereoEncoding
 **						Altered CompressSSND to take the
 *commchunk struct as
 **						  and arg and deal with mono and stereo
 *compression
 **	 2/16/93	RDG		Ported back to MPW 3.2.3, First release
 *.10 *	 2/6/93		RDG		Converted to Think C for development
 **	 2/7/93		RDG		Converted for use with Phil Burk's
 *aiff_tools
 **  1/?/92		RDG		got basic compression code from Steve
 *Hayes
 **
 **************************************************************************************/

#include "SquashSnd.h"
#include "iff_tools.h"
#include <StdLib.h>
#include <Strings.h>
#include <cursorctl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************
 ** Local typedefs.
 ********************************************************************/
typedef struct SDX2ContextBlk
{
  short *inBufferPtr;
  char *outBufferPtr;
  long bytesPerFrame;
  long framesInBuffer;
  long bytesInSample;
  short prevLeftSamp;
  short prevRightSamp;
  short prevMonoSamp;
  long avgLeftErr;
  long maxLeftErr;
  long avgRightErr;
  long maxRightErr;
  long avgMonoErr;
  long maxMonoErr;
} SDX2ContextBlk, *SDX2ContextBlkPtr;

/*********************************************************************
 ** Buffersize to use when encoding.
 ********************************************************************/
#define BUFFERSIZE (256 * 1024)

/*********************************************************************
 ** Comm chunk which is passed from SquashSnd.c
 ********************************************************************/
extern ExtCommonChunk CommChunk;

/*********************************************************************
 ** 		Local ANSI function prototypes
 *********************************************************************/
static long DoMonoEncoding (SDX2ContextBlkPtr ctx);
static long DoStereoEncoding (SDX2ContextBlkPtr ctx);
static signed char helpEncode (long CurSamp);
static signed char encode (long CurSamp, long PrevSamp);
static short decode (long CurSamp, long PrevSamp);

/*********************************************************************
 ** 		Macros
 *********************************************************************/
#define MAX(a, b) ((((a) < (b)) ? (b) : (a)))
#define MIN(a, b) ((((a) < (b)) ? (a) : (b)))
#define ABS(a) ((((a) < 0)) ? -(a) : (a))
#define dc(v) ((((short)v) * (short)(ABS (v))) << 1)

/*********************************************************************
 **		Decode a sample
 **
 **		On error: does nothing
 *********************************************************************/
static short
decode (long CurSamp, long PrevSamp)
{
  if ((CurSamp)&1)
    return (PrevSamp + dc (CurSamp));
  else
    return (dc (CurSamp));
}

/*********************************************************************
 **		Take Sqrt of a short and return a signed char
 **
 **		On error: does nothing
 *********************************************************************/
static signed char
helpEncode (long CurSamp)
{
  long neg;
  signed char EncSamp;

  neg = 0;
  if (CurSamp < 0)
    {
      neg = 1;
      CurSamp = -CurSamp;
    }

  EncSamp = sqrt ((float)(CurSamp >> 1));
  return (neg ? -EncSamp : EncSamp);
}

/*********************************************************************
 **		Encode a short as a compressed byte
 **
 **		On error: does nothing
 *********************************************************************/
static signed char
encode (long CurSamp, long PrevSamp)
{
  signed char Exact, Delta;
  long temp;

  Exact = helpEncode (CurSamp);
  Exact = Exact & ~1;
  temp = ABS (CurSamp - decode (Exact, PrevSamp));
  if (ABS (CurSamp - decode (Exact + 2, PrevSamp)) < temp)
    Exact += 2;
  else if (ABS (CurSamp - decode (Exact - 2, PrevSamp)) < temp)
    Exact -= 2;

  if (ABS (CurSamp - PrevSamp) > 32767)
    return Exact;

  Delta = helpEncode (CurSamp - PrevSamp);
  Delta = Delta | 1;

  temp = ABS (CurSamp - decode (Delta, PrevSamp));

  /* check for wraparound on the delta case */
  if (temp > 30000)
    {
      // we overflowed 16 bits on this delta.
      // Pull it closer to the center
      Delta = ((Delta < 0) ? (Delta + 2) : (Delta - 2));
      temp = ABS (CurSamp - decode (Delta, PrevSamp));
    }

  if (ABS (CurSamp - decode (Delta + 2, PrevSamp)) < temp)
    Delta += 2;
  else if (ABS (CurSamp - decode (Delta - 2, PrevSamp)) < temp)
    Delta -= 2;
  if (ABS (CurSamp - decode (Exact, PrevSamp))
      < ABS (CurSamp - decode (Delta, PrevSamp)))
    return Exact;
  else
    return Delta;
}

/*********************************************************************
 **		Encodes a block of sample data.
 **		If verbose mode is on, prints stats on encoding
 **
 **		On error: returns failure code
 *********************************************************************/
static long
DoMonoEncoding (SDX2ContextBlkPtr ctx)
{
  short *inBufferPtr = ctx->inBufferPtr;
  char *outBufferPtr = ctx->outBufferPtr;
  long Result = 0;
  long ix;
  long CurErr = 0;
  short CurSamp = 0;
  signed char CompSamp = 0;

  if (gVerbose)
    {
      printf ("\n  Encoding mono block...\n");
      fflush (stdout);
    }

  for (ix = 0; ix < ctx->framesInBuffer;
       ++ix) /* %Q (++ix) why did stever pre-increment counter */
    {
      /* let folks know every now and then that we're working on it */
      if (!(ix % 5000))
        SpinCursor (32);

      CurSamp = *inBufferPtr++;

      if (ix)
        CompSamp = encode ((long)CurSamp, (long)ctx->prevMonoSamp);
      else
        { /* force literal first time */
          CompSamp = helpEncode ((long)CurSamp);
          CompSamp &= ~1;
        }

      *outBufferPtr++ = CompSamp;

      if (gVerbose)
        {
          CurErr = decode ((long)CompSamp, (long)ctx->prevMonoSamp);
          CurErr = ABS ((long)CurSamp - CurErr);
          if (CurErr > ctx->maxMonoErr)
            {
              ctx->maxMonoErr = CurErr;
              printf ("   (New Max Err = %ld...  Curr:%d, Prev:%d, Comp:%d, "
                      "Deco:%d) \n",
                      ctx->maxMonoErr, CurSamp, ctx->prevMonoSamp, CompSamp,
                      decode ((long)CompSamp, (long)ctx->prevMonoSamp));
            }
          /* debug printf for looking at each sample */
          if (0)
            printf ("   DEBUG-- Comp:%d = Curr:%d, Prev:%d .. Deco:%d) \n",
                    CompSamp, CurSamp, ctx->prevMonoSamp,
                    decode ((long)CompSamp, (long)ctx->prevMonoSamp));

          ctx->avgMonoErr += CurErr;
          fflush (stdout);
        }

      ctx->prevMonoSamp
          = (long)decode ((long)CompSamp, (long)ctx->prevMonoSamp);
    }

error:
  return Result;
}
/*********************************************************************
 **		Loops through SSND chunk grabbing 16bits encoding them as 8 and
 **		writing them back out to new SSND chunk in outfile.
 **		Prints stats on encoding
 **
 **		On error: returns failure code
 *********************************************************************/
static long
DoStereoEncoding (SDX2ContextBlkPtr ctx)
{
  long Result = 0, ix;
  short *inBufferPtr = ctx->inBufferPtr;
  char *outBufferPtr = ctx->outBufferPtr;
  signed char CompSamp = 0;
  short CurLeftSamp = 0;
  short CurRightSamp = 0;
  long CurErr;

  if (gVerbose)
    {
      printf ("\n  Encoding stereo block...\n");
      fflush (stdout);
    }

  for (ix = 0; ix < ctx->framesInBuffer; ++ix)
    {
      /* let folks know every now and then that we're working on it */
      if (!(ix % 5000))
        SpinCursor (32);

      /* Process Left Sample */
      CurLeftSamp = *inBufferPtr++;

      if (ix)
        CompSamp = encode ((long)CurLeftSamp, (long)ctx->prevLeftSamp);
      else
        { /* force literal first time */
          CompSamp = helpEncode ((long)CurLeftSamp);
          CompSamp &= ~1;
        }

      *outBufferPtr++ = CompSamp;

      if (gVerbose)
        {
          CurErr = decode ((long)CompSamp, (long)ctx->prevLeftSamp);
          CurErr = ABS ((long)CurLeftSamp - CurErr);
          if (CurErr > ctx->maxLeftErr)
            {
              ctx->maxLeftErr = CurErr;
              printf ("   (New Max Err (Left) = %ld...  Curr:%d, Prev:%d, "
                      "Comp:%d, Deco:%d) \n",
                      ctx->maxLeftErr, CurLeftSamp, ctx->prevLeftSamp,
                      CompSamp,
                      decode ((long)CompSamp, (long)ctx->prevLeftSamp));
            }
          /* debug printf for looking at each sample */
          if (0)
            printf ("   DEBUG-- Comp:%d = Curr:%d, Prev:%d .. Deco:%d) \n",
                    CompSamp, CurLeftSamp, ctx->prevLeftSamp,
                    decode ((long)CompSamp, (long)ctx->prevLeftSamp));

          ctx->avgLeftErr += CurErr;
          fflush (stdout);
        }

      ctx->prevLeftSamp = decode ((long)CompSamp, (long)ctx->prevLeftSamp);

      /* Process Right Sample */
      CurRightSamp = *inBufferPtr++;

      if (ix)
        CompSamp = encode ((long)CurRightSamp, (long)ctx->prevRightSamp);
      else
        { /* force literal first time */
          CompSamp = helpEncode ((long)CurRightSamp);
          CompSamp &= ~1;
        }

      *outBufferPtr++ = CompSamp;

      if (gVerbose)
        {
          CurErr = decode ((long)CompSamp, (long)ctx->prevRightSamp);
          CurErr = ABS ((long)CurRightSamp - CurErr);
          if (CurErr > ctx->maxRightErr)
            {
              ctx->maxRightErr = CurErr;
              printf ("   (New Max Err (Right) = %ld...  Curr:%d, Prev:%d, "
                      "Comp:%d, Deco:%d) \n",
                      ctx->maxRightErr, CurRightSamp, ctx->prevRightSamp,
                      CompSamp,
                      decode ((long)CompSamp, (long)ctx->prevRightSamp));
            }
          /* debug printf for looking at each sample */
          if (0)
            printf ("   DEBUG-- Comp:%d = Curr:%d, Prev:%d .. Deco:%d) \n",
                    CompSamp, CurRightSamp, ctx->prevRightSamp,
                    decode ((long)CompSamp, (long)ctx->prevRightSamp));

          ctx->avgRightErr += CurErr;
          fflush (stdout);
        }

      ctx->prevRightSamp = decode ((long)CompSamp, (long)ctx->prevRightSamp);
    }

error:
  return Result;
}

/*********************************************************************
 **		Top of compression algorithm...  skips offest and blocksize
 **		in infile, writes 0s in outfile, encodes infile BUFFERSIZE K at
 **		a time.
 **
 **		On error: returns failure code
 *********************************************************************/
long
CompressSSND (iff_control_ptr in_ptr, iff_control_ptr out_ptr,
              ExtCommonChunk CommChunk)
{
  long Result;
  long ix;
  SDX2ContextBlk ctx;
  long currFilePos;
  long endSSNDChunkPos;
  long bytesRead;
  long bytesLeft;

  /* Initalize context block */
  ctx.inBufferPtr = NULL;
  ctx.outBufferPtr = NULL;
  ctx.bytesPerFrame
      = ((long)(CommChunk.sampleSize / 8) * (long)CommChunk.numChannels);
  ctx.framesInBuffer = 0;
  ctx.bytesInSample = ((long)(CommChunk.numSampleFrames * ctx.bytesPerFrame));
  ctx.prevLeftSamp = 0;
  ctx.prevRightSamp = 0;
  ctx.prevMonoSamp = 0;
  ctx.avgLeftErr = 0;
  ctx.maxLeftErr = 0;
  ctx.avgRightErr = 0;
  ctx.maxRightErr = 0;
  ctx.avgMonoErr = 0;
  ctx.maxMonoErr = 0;

  Result = iff_begin_ssnd (out_ptr);
  CHECKRESULT (Result, "_CompressSSND", "Couldn't start the new SSND chunk");

  /* Don't care about offset and blockSize as of now */
  Result = iff_skip_chunk_data (in_ptr, 8);
  CHECKRESULT (Result, "_CompressSSND",
               "Problems skipping the Offset and blockSize");

  currFilePos = iff_getpos (in_ptr); /* keep track of where we are */
  endSSNDChunkPos
      = currFilePos + ctx.bytesInSample; /* store end of sample data */
  bytesLeft = (endSSNDChunkPos - currFilePos);

  /* allocate buffers for processing sample data */
  ctx.inBufferPtr = malloc (BUFFERSIZE);
  ctx.outBufferPtr = malloc (BUFFERSIZE / 2);

  if ((ctx.inBufferPtr == 0) || (ctx.outBufferPtr == 0))
    CHECKRESULT (-1, "_CompressSSND", "couldn't allocate a buffer");

  while (bytesLeft)
    {
      /* we're still here */
      SpinCursor (32);

      /* If sample is smaller than desired buffersize, only read in
       * "bytesInSample"  number of bytes */
      if (bytesLeft < BUFFERSIZE)
        {
          bytesRead
              = iff_get_block (in_ptr, (char *)ctx.inBufferPtr, bytesLeft);
        }
      else
        {
          bytesRead
              = iff_get_block (in_ptr, (char *)ctx.inBufferPtr, BUFFERSIZE);
        }

      CHECKRESULT ((bytesRead == 0), "_CompressSSND",
                   "a block read returned 0 bytes!");
      currFilePos += bytesRead;
      bytesLeft = (endSSNDChunkPos - currFilePos);

      ctx.framesInBuffer = bytesRead / ctx.bytesPerFrame;

      if (gVerbose)
        {
          printf ("\n  Block read; size = %ld bytes\n", bytesRead);
          fflush (stdout);
        }

      if (CommChunk.numChannels == 1)
        {
          Result = DoMonoEncoding (&ctx);
          CHECKRESULT (Result, "_CompressSSND",
                       "Problems encoding the SSND chunk");
        }
      else
        {
          Result = DoStereoEncoding (&ctx);
          CHECKRESULT (Result, "_CompressSSND",
                       "Problems encoding the SSND chunk");
        }

      Result
          = iff_put_block (out_ptr, (char *)ctx.outBufferPtr, (bytesRead / 2));
      CHECKRESULT ((Result == 0), "_CompressSSND",
                   "Couldn't write buffer to output file!");

      if (gVerbose)
        {
          printf ("\n  Block written; size = %ld bytes\n", (bytesRead / 2));
          fflush (stdout);
        }
    }

  Result = iff_end_ssnd (out_ptr);
  CHECKRESULT (Result, "_CompressSSND",
               "Couldn't complete the new SSND chunk");

  if (gVerbose)
    {
      if (CommChunk.numChannels == 1)
        {
          printf ("\n Sample frames processed = %d\n",
                  CommChunk.numSampleFrames);
          printf (" Maximum absolute error = %d\n", ctx.maxMonoErr);
          printf (" Average absolute error = %d\n\n",
                  ctx.avgMonoErr / CommChunk.numSampleFrames);
        }
      else
        {
          printf ("\n Sample frames processed = %d\n", ix);
          printf (" Maximum absolute error for left channel = %d\n",
                  ctx.maxLeftErr);
          printf (" Average absolute error for left channel = %d\n\n",
                  ctx.avgLeftErr / CommChunk.numSampleFrames);
          printf (" Maximum absolute error for right channel = %d\n",
                  ctx.maxRightErr);
          printf (" Average absolute error for right channel = %d\n\n",
                  ctx.avgRightErr / CommChunk.numSampleFrames);
        }

      fflush (stdout);
    }

error:
  return (Result);
}

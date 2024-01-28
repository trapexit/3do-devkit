/*******************************************************************************************
 *	File:			SATrace.h
 *
 *	Contains:		Interface to debug and profile Streaming audio
 *subscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	10/14/93	rdg		Version 2.8  -  New Today
 *
 *
 * USAGE:
 *
 * This code allows the subscriber to leave timestamped trace data in a
 * circular buffer which can be examined using the debugger, or dumped
 * and converted to text with the MPW canon tool and the supplied
 * SAudioTrace.dict.
 *
 * This functionality is invaluable when analyzing the subscribers performance
 *and when debugging.  Nearly every signifigant event has a trace associated
 *with it.
 */

#ifndef __SATRACE_H__
#define __SATRACE_H__

/* Max number of entries in the buffer, much more than this and the
 * will overflow the debugger teminal windows scroll back buffer
 */
#define MAX_TRACE 256

#include "types.h"

/* Trace ID's */
enum
{
  kFlushedDataMsg = 1000,
  kFlushedBuffer,
  kMovedBuffer,
  kBufferCompleted,
  kFreedBufferFromFolio,
  kFoundBuffer,
  kFoundWrongBuffer,
  kNewDataMsgArrived,
  kMaskedSignalBits,
  kNewHeaderMsgArrived,
  kBeginSetChannelAmp,
  kBeginSetChannelPan,
  kEndSetChannelAmp,
  kEndSetChannelPan,
  kCurrentBufferOnStop,
  kWaitingOnSignal,
  kGotSignal,
  kBufferOrphaned,
  kLoadedTemplates,
  kChannelMuted,
  kChannelUnMuted
};

typedef struct TraceEntry
{
  long when;    /* time stamp */
  long what;    /* event code */
  long channel; /* logical channel, -1 if indeterminate */
  long avalue;  /* a value relavant to the trace, perhaps signal bits etc. */
  long bufPtr;  /* pointer to a buffer or subscriber message, else 0 */
} TraceEntry, *TraceEntryPtr;

#define ADD_TRACE(event, chan, value, ptr)                                    \
  {                                                                           \
    if (traceIndex >= MAX_TRACE)                                              \
      traceIndex = 0;                                                         \
    trace[traceIndex].when = (long)GetAudioTime ();                           \
    trace[traceIndex].what = event;                                           \
    trace[traceIndex].channel = chan;                                         \
    trace[traceIndex].avalue = value;                                         \
    trace[traceIndex].bufPtr = ptr;                                           \
    traceIndex++;                                                             \
  }

#define DUMP_RAW_TRACE_BUFFER()                                               \
  {                                                                           \
    savedTraceIndex = traceIndex;                                             \
    printf ("\n\n\nTrace Index is %ld!!!!\n", traceIndex);                    \
    for (traceIndex = 0; traceIndex < MAX_TRACE; traceIndex++)                \
      {                                                                       \
        printf ("Index: %ld", traceIndex);                                    \
        printf ("\tTime:  %ld", trace[traceIndex].when);                      \
        printf ("\tChan:  %ld", trace[traceIndex].channel);                   \
        printf ("\tValue:  0x%lx", trace[traceIndex].avalue);                 \
        printf ("\tAddr:  0x%lx", trace[traceIndex].bufPtr);                  \
        printf ("\t\tEvent: ec%ld\n", trace[traceIndex].what);                \
      }                                                                       \
    traceIndex = savedTraceIndex;                                             \
  }

/************************************************************
 * Routines used to dump buffer to debugger terminal window
 ************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

  long FindCompletion (long submissionIndex);
  void DumpBufferCompletionStats (void);

#ifdef __cplusplus
}
#endif

#endif /* __SATRACE_H__ */

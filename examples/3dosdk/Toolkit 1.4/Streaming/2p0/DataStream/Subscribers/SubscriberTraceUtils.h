/*******************************************************************************************
 *	File:			SubscriberTraceUtils.h
 *
 *	Contains:		Interface to debug and profile subscribers.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	3/29/94		rdg		Version 1.4
 *						Create a "TraceBuffer" struct which
 *contains the index and the data. 3/25/94		rdg
 *Version 1.3 Last implementation was fucked.  This time, let every subscriber
 *have it's own buffer.  The routines prototyped here now take a pointer to a
 *trace buffer and a file name to write the output to, instead of using
 *globals. 1/12/93		rdg		Version 1.2 Update for use with
 *multiple subscribers. 12/24/93	rdg		Version 1.1 Generalize
 *for use with other subscribers. 10/14/93	rdg		Version 1.0  -
 *New Today
 *
 *
 * USAGE:
 *
 * This code allows the subscriber to leave timestamped trace data in a
 * circular buffer which can be examined using the debugger, or dumped
 * and converted to text with the MPW canon tool and the supplied
 * TraceSubscriber.dict.
 *
 * This functionality is invaluable when analyzing the subscriber's
 * performance and when debugging a subscriber or multiple subscrier
 * interactions.
 *
 * IMPLEMENTATION NOTES:
 *
 * Each subscriber maintains a trace buffer and a set of event codes.  The
 * AddTrace() routine is used to jam a timestamped event into the buffer.
 * I use the audio timer for timestamps.
 *
 * The routines described below are used to dump the
 * contents of a trace buffer into a file in /remote.  You can then
 * run the MPW tool Canon using the appropriate subscriber specific
 * dictionary file to convert the event codes into usefull text strings.
 *
 * DumpRawTraceBuffer() will dump all of the events in the buffer to
 * a file with the specified name.
 *
 * DumpEventCompletionStats() will use the start and completed event
 * codes to traverse the buffer trying to match start events with
 * completion events.  It will then print some timing statistics.
 * If you passed in the event codes kSNDSMovedBuffer and
 * kSNDSWriteBufferCompleted, you would find out exactly how long
 * it took for the audio folio to return this data buffer.
 *
 *	A completion stats entry consists of:
 *		Start Index
 *		Completion Index
 *		Channel Num
 *		SignalBits/Value
 *		Submission Time
 *		Completion Time
 *		Dur Since Start
 *
 * The undocumented and unsupported Portfolio functions fopen()/fclose() etc.
 * don't do truncation.  SO DON'T FORGET TO TAKE YOUR OLD TRACE DUMPS OUT
 * OF /remote!!!!  Or you'll get confusing junk.
 *
 * ¥¥¥¥¥¥IMPORTANT¥¥¥¥¥¥
 *
 * The DumpEventCompletionStats() function finds a match by searching
 * for an event which...
 * 		occurred later than the startEvent,
 * 		has an event code matches the completionEventCode,
 * 		and which has an "aValue" field which matches the start event.
 *
 *		The aValue field is used by the Audio and MPEG subscribers
 *		to track buffer completions signals which come from the
 *AudioFolio and MPEG driver.  You don't necessarily have to use signals here
 *		but this feature is maintined for compatibility and becase a
 *		the AudioSubscriber already exists as a example of how to do
 *it.
 *
 *	If you want to use DumpEventCompletionStats() you MUST MAKE SURE
 *	that when you do your tracing, you make sure that the events you
 *	use for start and completion ALWAYS HAVE THE SAME VALUE FOR THIS
 *	FIELD!!!!!   Otherwise it won't work...
 **********************************************************************/

#ifndef __SUBSCRIBERTRACEUTILS_H__
#define __SUBSCRIBERTRACEUTILS_H__

/* The number of events in each trace buffer
 * Same for every subscriber
 */
#define MAX_TRACE 500

/**********************************************************************
 * Error codes
 **********************************************************************/

typedef enum ProtoErrorCode
{
  kTraceNoErr,             /* all is well */
  kTraceErrCouldntOpenFile /* fopen() failed for some reason */
};

/**********************************************************************
 * Data Structures
 **********************************************************************/

typedef struct TraceEntry
{
  long when;    /* time stamp */
  long what;    /* event code */
  long channel; /* logical channel, -1 if indeterminate */
  long aValue;  /* a value relavant to the trace, perhaps signal bits etc. */
  void *aPtr;   /* pointer to a buffer or subscriber message, else 0 */
} TraceEntry, *TraceEntryPtr;

typedef struct TraceBuffer
{
  long index;                   /* index of current entry */
  TraceEntry evList[MAX_TRACE]; /* The list of trace events */
} TraceBuffer, *TraceBufferPtr;

/**********************************************************************
 * Public routine prototypes
 **********************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

  void AddTrace (TraceBufferPtr traceBuffer, long event, long chan, long value,
                 void *ptr);
  long DumpRawTraceBuffer (TraceBufferPtr traceBuffer, char *filename);
  long DumpTraceCompletionStats (TraceBufferPtr traceBuffer,
                                 long startEventCode, long completionEventCode,
                                 char *filename);

#ifdef __cplusplus
}
#endif

#endif /* __SUBSCRIBERTRACEUTILS_H__ */

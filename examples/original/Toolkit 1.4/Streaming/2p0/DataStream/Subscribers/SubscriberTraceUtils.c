/*******************************************************************************************
 *	File:			TraceSubscriberUtils.c
 *
 *	Contains:		Code to debug and profile subscribers.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	3/29/94		rdg		Version 1.4
 *						Create a "TraceBuffer" struct which
 *contains the index and the data. Change the "ec" trace code prefix to "tc".
 *	3/25/94		rdg		Version 1.3
 *						Last implementation was fucked.  This
 *time, let every subscriber have it's own traceBuffer.  The routines
 *implemented here now take a pointer to any valid trace buffer and a file name
 *to write the output to, instead of using globals. Should be better. 1/12/94
 *rdg		Version 1.2 Make useable by all subscribers. Write trace output
 *to a file and not do printf(). 12/24/93	rdg		Version 1.1
 *						Generalize for use with other
 *subscribers. 10/14/93	rdg		Version 1.0  -  New Today
 *
 *
 */

#include "SubscriberTraceUtils.h"
#include "audio.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

/*****************************************************************************
 * Local Prototypes
 *****************************************************************************/

static long FindCompletion (TraceBufferPtr traceBuffer, long startEventIndex,
                            long completionEventCode);

/*****************************************************************************
 * Implementations
 *****************************************************************************/

/* Routine to add a trace entry into a trace buffer */
void
AddTrace (TraceBufferPtr traceBuffer, long event, long chan, long value,
          void *ptr)
{
  TraceEntryPtr entryPtr; // pointer to current trace entry

  /* Sanity check on index into trace buffer, so we don't stomp memory */
  if (traceBuffer->index < MAX_TRACE)
    {
      /* Point to the current trace entry */
      entryPtr = traceBuffer->evList + traceBuffer->index;

      /* Make the entry */
      entryPtr->when = (long)GetAudioTime ();
      entryPtr->what = event;
      entryPtr->channel = chan;
      entryPtr->aValue = value;
      entryPtr->aPtr = ptr;
    }

  /* Increment search and wrap around circular buffer if appropriate */
  if (++traceBuffer->index == MAX_TRACE)
    traceBuffer->index = 0;
}

/* Dump out everything in the trace buffer to the file. */
long
DumpRawTraceBuffer (TraceBufferPtr traceBuffer, char *filename)
{
  char str[127];               // string to pass to fputs
  long index;                  // for traversing the trace buffer
  TraceEntryPtr curTraceEntry; // pointer to current trace entry
  FILE *LogFile;               // output file

  /* Open the log file */
  LogFile = fopen (filename, "w");
  if ((long)LogFile <= 0)
    return kTraceErrCouldntOpenFile;

  /* Write out the last position in the trace buffer that was written to */
  sprintf (str, "Last valid index was: %ld\n\n\n", traceBuffer->index);
  fputs (str, LogFile);

  for (index = 0; index < MAX_TRACE; index++)
    {
      /* Point to the entry */
      curTraceEntry = traceBuffer->evList + index;

      /* The index number of the entry */
      sprintf (str, "Index: %ld", index);
      fputs (str, LogFile);

      /* The timestamp of the entry */
      sprintf (str, "\tTime:  %ld", curTraceEntry->when);
      fputs (str, LogFile);

      /* The channel number of the entry */
      sprintf (str, "\tChan:  %ld", curTraceEntry->channel);
      fputs (str, LogFile);

      /* The signals/value field of the entry */
      sprintf (str, "\tValue:  0x%lx", curTraceEntry->aValue);
      fputs (str, LogFile);

      /* The pointer field of the entry */
      sprintf (str, "\tAddr:  0x%lx", (long)curTraceEntry->aPtr);
      fputs (str, LogFile);

      /* The event type code of the entry */
      sprintf (str, "\tEvent: tc%ld\n", curTraceEntry->what);
      fputs (str, LogFile);

      /* If the audio time for an event is 0 then it was never used, so
       * we know the buffer never filled up.
       * No point in more printing.
       */
      if (curTraceEntry->when == 0)
        break;
    }

  fclose (LogFile);
  return kTraceNoErr;
}

/* Given the index of some start event, search the trace buffer for the
 * the next completion event which matches the submission. We
 * find a match by searching for an event which...
 * 		occurred later than the startEvent,
 * 		has an event code matches the completionEventCode,
 * 		and which has an aValue field which matches the start event.
 *
 *		The aValue field is used by the Audio and MPEG subscribers
 *		to track buffer completions signals which come from the
 *AudioFolio and MPEG driver.
 */
static long
FindCompletion (TraceBufferPtr traceBuffer, long startEventIndex,
                long completionEventCode)
{
  long tmpIndex; // for traversing the buffer
  TraceEntryPtr
      startEventEntry; // ptr to the trace entry where we start searching
  TraceEntryPtr curTraceEntry; // ptr to current trace entry

  /* Keep a pointer to the start event */
  startEventEntry = traceBuffer->evList + startEventIndex;

  /* Begin searching at the start event */
  tmpIndex = startEventIndex + 1;

  /* Search until we wrap back around to the start event */
  while (tmpIndex != startEventIndex)
    {
      /* Point to the current TraceEntry */
      curTraceEntry = traceBuffer->evList + tmpIndex;

      /* If this is a completion event, the values matche,
       * and the completion event happened later in time than the
       * start event (we need this because of the circular buffer,
       * this is it...
       */
      if ((curTraceEntry->what == completionEventCode)
          && /* is at a completion */
          (curTraceEntry->aValue == startEventEntry->aValue)
          && /* do the values match */
          (curTraceEntry->when > startEventEntry->when)) /* check times */
        return tmpIndex;

      /* If the audio time for an event is 0 then it was never used, so
       * we know the buffer never filled up.
       * No point in more searching.
       */
      if (curTraceEntry->when == 0)
        break;

      /* Increment search and wrap around circular buffer if appropriate */
      if (++tmpIndex == MAX_TRACE)
        tmpIndex = 0;
    }

  /* We failed to find one */
  return -1;
}

/* Search the trace buffer and dump statistics on event completion times.
 *
 * DumpEventCompletionStats() will use the start and completed event
 * codes to traverse the buffer trying to match start events with
 * completion events.  It will then print some timing statistics.
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
 * If, for example, you were dumping a trace from the SAudioSubscriber and
 * you passed in the event codes kSNDSMovedBuffer and
 * kSNDSWriteBufferCompleted, you would find out exactly how long
 * it took for the audio folio to return this data buffer to you
 * after you queued it.
 */
long
DumpTraceCompletionStats (TraceBufferPtr traceBuffer, long startEventCode,
                          long completionEventCode, char *filename)
{
  char str[127];
  long tmpIndex;
  long completionIndex;
  TraceEntryPtr curTraceEntry;
  TraceEntryPtr compTraceEntry;
  FILE *LogFile;

  /* Open the log file */
  LogFile = fopen (filename, "w");
  if ((long)LogFile <= 0)
    return kTraceErrCouldntOpenFile;

  sprintf (str, "\nStart Index\tCompletion Index\tChannel "
                "Num\tSignalBits/Value\tSubmission Time\tCompletion Time\tDur "
                "Since Submission\n\n");
  fputs (str, LogFile);

  /* Start searching from startIndex + 1
   * where startIndex is the last valid event entry.
   */
  tmpIndex = traceBuffer->index + 1;

  /* Get a ptr to what we assume is the oldest event in the buffer */
  curTraceEntry = traceBuffer->evList + tmpIndex;

  /* Check to see if the next event is all zeros.  If it is, that means we
   * never filled the buffer so we can just start our search from the
   * beginning of the buffer.
   * If it's not all zeros, then we have wrapped, so we start the search
   * from the least recent event.  i.e. tmpIndex.
   */
  if (curTraceEntry->when == 0)
    tmpIndex = 0;

  /* Traverse the buffer looking for start events.  When we find one, we
   * start searching for a matching completion.
   */
  while (tmpIndex != traceBuffer->index)
    {
      curTraceEntry = traceBuffer->evList + tmpIndex;

      if (curTraceEntry->what == startEventCode)
        {
          completionIndex
              = FindCompletion (traceBuffer, tmpIndex, completionEventCode);

          /* -1 means couldn't find completion */
          if (completionIndex < 0)
            {
              sprintf (str,
                       "\nCouldn't find completion event for event start at "
                       "index %ld...\n",
                       tmpIndex);
              fputs (str, LogFile);
            }
          else
            {
              /* Get a pointer to the completion trace event */
              compTraceEntry = traceBuffer->evList + completionIndex;

              /* Write out the stats for this completion.  Space with
               * single tabs for cut/paste into Excell or InControl.
               */
              /* Start Index */
              sprintf (str, "SIdx: %ld", tmpIndex);
              fputs (str, LogFile);

              /* Completion Index */
              sprintf (str, "\tCIdx: %ld", completionIndex);
              fputs (str, LogFile);

              /* Channel Number */
              sprintf (str, "\tChan: %ld", curTraceEntry->channel);
              fputs (str, LogFile);

              /* Signals */
              sprintf (str, "\tSig/Val: %lx", curTraceEntry->aValue);
              fputs (str, LogFile);

              /* Start time */
              sprintf (str, "\tSTime: %ld", curTraceEntry->when);
              fputs (str, LogFile);

              /* Completion time */
              sprintf (str, "\tCTime: %ld", compTraceEntry->when);
              fputs (str, LogFile);

              /* Time between submission and completion */
              sprintf (str, "\tDur: %ld\n",
                       (compTraceEntry->when - curTraceEntry->when));
              fputs (str, LogFile);
            }
        }

      /* Increment search and wrap around circular buffer if appropriate */
      if (++tmpIndex == MAX_TRACE)
        tmpIndex = 0;
    }

  /* Close the log file */
  fclose (LogFile);

  return kTraceNoErr;
}

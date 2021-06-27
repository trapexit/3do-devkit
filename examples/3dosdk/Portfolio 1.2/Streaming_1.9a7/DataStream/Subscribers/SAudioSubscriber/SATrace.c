/*******************************************************************************************
 *	File:			SATrace.c
 *
 *	Contains:		Code to debug and profile Streaming audio subscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/14/93	rdg		Version 2.8  -  New Today
 *
 *
 * USAGE:
 *
 * This functionality is invaluable when analyzing the subscribers performance and
 * when debugging.  Nearly every signifigant event has a trace associated with it.
 * Examine the code compiled with TRACE_ENABLE for details.
 */
 
 #include "types.h"
 #include "stdio.h"
 #include "SATrace.h"
 
long traceIndex = 0;
long savedTraceIndex = 0;

TraceEntry	trace[ MAX_TRACE ];

/* Given an index into the trace buffer of a buffer submission event, search the
 * buffer for the next buffer completion event whose signal matches the submission.
 */
long	FindCompletion( long submissionIndex )
	{
	long	tmpIndex;
	
	tmpIndex = submissionIndex;
	
	while (true)
		{															
		/* Searched the whole buffer and found no match */
		if ( (tmpIndex + 1) == submissionIndex )
			break;

		/* If all of the fields are 0 then the buffer never filled up 
		 * so stop searching for a completion 
		 */
		if(	(trace[ tmpIndex ].when == 0)		&&
			(trace[ tmpIndex ].what == 0)		&&
			(trace[ tmpIndex ].channel == 0)	&&
			(trace[ tmpIndex ].avalue == 0)		&&
			(trace[ tmpIndex ].bufPtr == 0)   )	
		   break;

		/* If this is a completion event, and the signals match and the channels match,
		 * this is it...
		 */
		if ( (trace[ tmpIndex ].what == kFreedBufferFromFolio )  &&
			 (trace[ tmpIndex ].avalue == trace[ submissionIndex ].avalue ) )
			{
			return tmpIndex;
			}

		/* increment search */
		tmpIndex++;

		/* Wrap the circular buffer */
		if (tmpIndex == MAX_TRACE)
			tmpIndex = 0;
		}
		
	/* Never found a match */
	return -1;
	}
	
/* Search the trace buffer and dump statistics on buffer completion times */
void		DumpBufferCompletionStats ( void )
	{
	long	completionIndex;
	long	tmpIndex;
	long	prevCompletionTime[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	printf("\nINDx\tCHANNEL#\tSIGNALBITS\tSUB_TIME\tCOMP_TIME\tDUR_SINCE_SUB\tDUR_SINCE_LAST_COMP\n\n");

	tmpIndex = traceIndex;

	while (true)
		{															
		if ( (tmpIndex + 1) == traceIndex )
			break;
		
		if (trace[ tmpIndex ].what == kMovedBuffer)
			{
			completionIndex = FindCompletion( tmpIndex );
			/* -1 means couldn't find completion */
			if ( completionIndex < 0 )
				{
				printf("\nCouldn't find match for buffer submission at index %ld...\n",tmpIndex);
				}
			else
				{
				/* print out -- IND    CHANNEL#    SIGNALBITS    SUB_TIME     COMP_TIME     DURATION */
				printf("%ld", tmpIndex);		
				printf("\t\t%ld", trace[ tmpIndex ].channel);		
				printf("\t\t%lx", trace[ tmpIndex ].avalue);	
				printf("\t\t\t%ld", trace[ tmpIndex ].when);			
				printf("\t\t\t%ld", trace[ completionIndex ].when);		
				printf("\t\t\t%ld", (trace[ completionIndex ].when - trace[ tmpIndex ].when));	
				printf("\t\t\t%ld\n", (trace[ completionIndex ].when - prevCompletionTime[trace[ tmpIndex ].channel]));	
				
				/* Save completion time */
				prevCompletionTime[trace[ tmpIndex ].channel] = trace[ completionIndex ].when;
				}
			}
	
		/* increment search */
		tmpIndex++;
		
		/* Wrap around circular buffer */
		if (tmpIndex == MAX_TRACE)
			tmpIndex = 0;
			
		}															
	}


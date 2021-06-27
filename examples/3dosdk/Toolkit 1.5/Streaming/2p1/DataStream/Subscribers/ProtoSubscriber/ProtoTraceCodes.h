/*******************************************************************************************
 *	File:			ProtoTraceCodes.h
 *
 *	Contains:		Interface to debug and profile the proto subscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993-94 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	3/25/94		rdg		New Today
 *
 *
 * USAGE:
 *
 * These trace codes are for use with the SubscriberTrace system.  Define
 * one for each event you with to keep track of in your subscriber.  Make
 * sure to keep these codes in sync with ProtoTrace.dict!
 */

#ifndef __PROTOTRACECODES_H__
#define __PROTOTRACECODES_H__

#include "types.h"

/* Trace ID's */
enum {
	kTraceInitSubscriber 		= 1000,		
	kTraceCloseSubscriber		= 1001,
	kTraceNewSubscriber			= 1002,
	kTraceDisposeSubscriber		= 1003,	
	kTraceWaitingOnSignal		= 1004,
	kTraceGotSignal				= 1005,
	kTraceDataMsg				= 1006,
	kTraceGetChanMsg			= 1007,
	kTraceSetChanMsg			= 1008,
	kTraceControlMsg			= 1009,
	kTraceSyncMsg				= 1010,
	kTraceOpeningMsg			= 1011,
	kTraceClosingMsg			= 1012,
	kTraceStopMsg				= 1013,
	kTraceStartMsg				= 1014,
	kTraceEOFMsg				= 1015,
	kTraceAbortMsg				= 1016,
	kTraceDataSubmitted			= 1017,
	kTraceDataCompleted			= 1018,
	
	kTraceChannelInit				= 1019,	
	kTraceChannelClose				= 1020,
	kTraceChannelStart				= 1021,
	kTraceChannelStop				= 1022,
	kTraceChannelFlush				= 1023,

	kTraceChannelNewDataArrived		= 1024,
	
	kFlushedDataMsg					= 1025,

	kHaltChunkArrived				= 1026,
	kRepliedToHaltChunk				= 1027
	
	};
	
#endif	/* __PROTOTRACECODES_H__ */

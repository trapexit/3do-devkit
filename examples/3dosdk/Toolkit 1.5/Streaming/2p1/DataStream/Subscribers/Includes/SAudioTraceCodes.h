/*******************************************************************************************
 *	File:			SAudioTraceCodes.h
 *
 *	Contains:		Interface to debug and profile the SAudio
 *subscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993-94 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/27/94		rdg		Add pause code
 *	3/29/94		rdg		New Today
 *
 *
 * USAGE:
 *
 * These trace codes are for use with the SubscriberTrace system.  Define
 * one for each event you with to keep track of in your subscriber.  Make
 * sure to keep these codes in sync with SAudioTrace.dict!
 */

#ifndef __SAUDIOTRACECODES_H__
#define __SAUDIOTRACECODES_H__

#include "types.h"

/* Trace ID's */
enum
{
  kTraceInitSubscriber = 1000,
  kTraceCloseSubscriber = 1001,
  kTraceNewSubscriber = 1002,
  kTraceDisposeSubscriber = 1003,
  kTraceWaitingOnSignal = 1004,
  kTraceGotSignal = 1005,
  kTraceDataMsg = 1006,
  kTraceGetChanMsg = 1007,
  kTraceSetChanMsg = 1008,
  kTraceControlMsg = 1009,
  kTraceSyncMsg = 1010,
  kTraceOpeningMsg = 1011,
  kTraceClosingMsg = 1012,
  kTraceStopMsg = 1013,
  kTraceStartMsg = 1014,
  kTraceEOFMsg = 1015,
  kTraceAbortMsg = 1016,
  kTraceDataSubmited = 1017,
  kTraceDataCompleted = 1018,

  kTraceChannelInit = 1019,
  kTraceChannelClose = 1020,
  kTraceChannelStart = 1021,
  kTraceChannelStop = 1022,
  kTraceChannelFlush = 1023,

  kFlushedDataMsg = 1024,
  kFlushedBuffer = 1025,
  kMovedBuffer = 1026,
  kBufferCompleted = 1027,
  kFreedBufferFromFolio = 1028,
  kFoundBuffer = 1029,
  kFoundWrongBuffer = 1030,
  kNewDataMsgArrived = 1031,
  kMaskedSignalBits = 1032,
  kNewHeaderMsgArrived = 1033,
  kBeginSetChannelAmp = 1034,
  kBeginSetChannelPan = 1035,
  kEndSetChannelAmp = 1036,
  kEndSetChannelPan = 1037,
  kCurrentBufferOnStop = 1038,
  kBufferOrphaned = 1039,
  kLoadedTemplates = 1040,
  kChannelMuted = 1041,
  kChannelUnMuted = 1042,
  kTraceInternalChannelPause = 1043,
  kTraceInternalChannelStop = 1044,
  kBeginPlayback = 1045,
  kStartAttachment = 1046,
  kResumeInstrument = 1047,
  kStartWFlushWhilePaused = 1048,
  kStopWFlushWhilePaused = 1049,
  kAttachCrossedWhileStopping = 1050,
  kAttachCrossedWhilePausing = 1051,
  kCurrentBufferOnPause = 1052

};

#endif /* __SAUDIOTRACECODES_H__ */

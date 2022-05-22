#pragma include_only_once

#include "types.h"

/* Trace ID's */
enum
  {
   kTraceInitSubscriber        = 1000,
   kTraceCloseSubscriber       = 1001,
   kTraceNewSubscriber         = 1002,
   kTraceDisposeSubscriber     = 1003,
   kTraceWaitingOnSignal       = 1004,
   kTraceGotSignal             = 1005,
   kTraceDataMsg               = 1006,
   kTraceGetChanMsg            = 1007,
   kTraceSetChanMsg            = 1008,
   kTraceControlMsg            = 1009,
   kTraceSyncMsg               = 1010,
   kTraceOpeningMsg            = 1011,
   kTraceClosingMsg            = 1012,
   kTraceStopMsg               = 1013,
   kTraceStartMsg              = 1014,
   kTraceEOFMsg                = 1015,
   kTraceAbortMsg              = 1016,
   kTraceDataSubmitted         = 1017,
   kTraceDataCompleted         = 1018,
   kTraceChannelInit           = 1019,
   kTraceChannelClose          = 1020,
   kTraceChannelStart          = 1021,
   kTraceChannelStop           = 1022,
   kTraceChannelFlush          = 1023,
   kTraceChannelNewDataArrived = 1024,
   kFlushedDataMsg	       = 1025,
   kHaltChunkArrived           = 1026,
   kRepliedToHaltChunk         = 1027
  };

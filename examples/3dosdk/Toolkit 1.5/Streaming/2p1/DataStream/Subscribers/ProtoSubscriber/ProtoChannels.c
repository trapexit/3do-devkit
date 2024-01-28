/*******************************************************************************************
 *	File:			ProtoChannels.c
 *
 *	Contains:		Routines for logical channel specific
 *functions.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	09/07/94	dtc		Version 2.0.1d4
 *						Replaced printf with ERR macro defined in
 *Debug3DO.h.
 *	7/07/94		rdg		Use unique #define names for trace
 *switching
 *	6/10/94		rdg		Make the Macros for testing channel
 *status have unique names.
 *	5/31/94		rdg		Make "Channel" functions have unique
 *names 3/25/94		rdg		New Today
 *
 *******************************************************************************************/

/*****************************************************************************
 * Header file includes
 *****************************************************************************/

#include "ProtoChannels.h"
#include "Debug3DO.h"
#include "ProtoErrors.h"
#include "ProtoSubscriber.h"
#include "types.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if PROTO_TRACE_CHANNELS

#include "ProtoTraceCodes.h"
#include "SubscriberTraceUtils.h"

/* This switch is a way to quickly change the level of tracing.
 * 	TRACE_LEVEL_1 gives you minimal trace info,
 *	TRACE_LEVEL_2 gives you more info (includes level 1 trace),
 *	TRACE_LEVEL_3 gives you  maximal info (includes levels 1 and 2 trace).
 */

#define PROTO_TRACE_LEVEL 3

/* Locate the trace buffer.  It's declared in ProtoSubscriber.c.
 */
extern TraceBufferPtr ProtoTraceBufPtr;

/* Allow for multiple levels of tracing */
#if (PROTO_TRACE_LEVEL >= 1)
#define ADD_PROTO_TRACE_L1(bufPtr, event, chan, value, ptr)                   \
  AddTrace (bufPtr, event, chan, value, ptr)
#else
#define ADD_PROTO_TRACE_L1(bufPtr, event, chan, value, ptr)
#endif

#if (PROTO_TRACE_LEVEL >= 2)
#define ADD_PROTO_TRACE_L2(bufPtr, event, chan, value, ptr)                   \
  AddTrace (bufPtr, event, chan, value, ptr)
#else
#define ADD_PROTO_TRACE_L2(bufPtr, event, chan, value, ptr)
#endif

#if (PROTO_TRACE_LEVEL >= 3)
#define ADD_PROTO_TRACE_L3(bufPtr, event, chan, value, ptr)                   \
  AddTrace (bufPtr, event, chan, value, ptr)
#else
#define ADD_PROTO_TRACE_L3(bufPtr, event, chan, value, ptr)
#endif

#else /* PROTO_TRACE_CHANNELS */

/* Trace is off */
#define ADD_PROTO_TRACE_L1(bufPtr, event, chan, value, ptr)
#define ADD_PROTO_TRACE_L2(bufPtr, event, chan, value, ptr)
#define ADD_PROTO_TRACE_L3(bufPtr, event, chan, value, ptr)

#endif /* PROTO_TRACE_CHANNELS */

/*******************************************************************************************
 * Routine to initalize a channel for a given context.  This routine is
 * called when a new header chunk is received on a given channel.  The only
 * way a channel can become "un-initalized" is by calling CloseProtoChannel().
 *******************************************************************************************/
long
InitProtoChannel (ProtoContextPtr ctx, ProtoHeaderChunkPtr headerPtr)
{
  long status;
  ProtoChannelPtr chanPtr;

  status = 0;
  chanPtr = ctx->channel + headerPtr->channel;

  ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kTraceChannelInit, headerPtr->channel,
                      0, 0);

  /* Sanity check:  Don't attempt to re-initalize a channel which has already
   * recieved a header.  This will happen frequently when streams are looped
   * or when lots of branching is going on so it doesn't really constitute an
   * error.
   *
   * There is no danger of screwing things up with the prototype code below,
   * but when real init code is put here you won't want to be re-executing it
   * everytime a header comes through.
   */
  if (!IsProtoChanInitalized (chanPtr))
    {
      /* Make sure this version of Proto files are compatible with this
       * subscriber */
      if (headerPtr->version > PROTO_STREAM_VERSION)
        return ProtoErrStreamVersionInvalid;

      /* Set the initalization flag. */
      chanPtr->status |= PROTO_CHAN_INITALIZED;

      /* Default channel 0 to enabled.  All other channels must be explicitly
       * enabled with a SetChan message.
       */
      if (headerPtr->channel == 0)
        chanPtr->status |= CHAN_ENABLED;
    }

  return status;
}

/*******************************************************************************************
 * Routine to begin data flow for the given channel.
 *******************************************************************************************/
long
StartProtoChannel (ProtoContextPtr ctx, long channelNumber)
{
  long status;
  ProtoChannelPtr chanPtr;

  status = 0;
  chanPtr = ctx->channel + channelNumber;

  ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kTraceChannelStart, channelNumber, 0,
                      0);

  /* CHAN_ACTIVE simply means that we have received the "stream started" msg */
  chanPtr->status |= CHAN_ACTIVE;

  /* If the channel is disabled, don't bother to do anything else */
  if (IsProtoChanEnabled (chanPtr))
    {
      /* Do whatever you need to do to get things started */
    }
  return status;
}

/*******************************************************************************************
 * Routine to halt data flow for the given channel.
 *******************************************************************************************/
long
StopProtoChannel (ProtoContextPtr ctx, long channelNumber)
{
  long status;
  ProtoChannelPtr chanPtr;

  status = 0;
  chanPtr = ctx->channel + channelNumber;

  ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kTraceChannelStop, channelNumber, 0,
                      0);

  /* The stream has stopped */
  chanPtr->status &= ~CHAN_ACTIVE;

  /* If the channel is disabled, don't bother to do anything else */
  if (IsProtoChanEnabled (chanPtr))
    {
      /* Do whatever you need to do to make things stop */
    }
  return status;
}

/*******************************************************************************************
 * Routine to disable further data flow for the given channel, and to cause
 * any queued data to be flushed.
 *******************************************************************************************/
long
FlushProtoChannel (ProtoContextPtr ctx, long channelNumber)
{
  long status;
  ProtoChannelPtr chanPtr;
  SubscriberMsgPtr msgPtr;

  status = 0;
  chanPtr = ctx->channel + channelNumber;

  ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kTraceChannelFlush, channelNumber, 0,
                      0);

  /* If the channel is disabled, don't bother to do anything else */
  if (IsProtoChanEnabled (chanPtr))
    {
      /* Halt whatever activity is associated with the channel */
      status = StopProtoChannel (ctx, channelNumber);
      CHECKRESULT ("FlushProtoChannel() - StopProtoChannel() failed", status);

      /* Give back all queued chunks for this channel to the
       * stream parser. We do this by replying to all the
       * "chunk arrived" messages that we have queued.
       */
      while ((msgPtr = GetNextDataMsg (&chanPtr->dataMsgQueue)) != NULL)
        {
          /* Reply to a chunk, i.e. return it to the streamer. */
          ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kFlushedDataMsg, channelNumber,
                              0, msgPtr);
          status = ReplyMsg (msgPtr->msgItem, kDSNoErr, msgPtr,
                             sizeof (SubscriberMsg));
          CHECKRESULT ("FlushProtoChannel() - ReplyMsg()", status);
        }

      /* Flush subscriber specific stuff here.
       */
    }

  return status;
}

/*******************************************************************************************
 * Stop and Flush a channel; Then free up all it's resources.  Should leave a
 *channel in pre-initalized state.
 *******************************************************************************************/
long
CloseProtoChannel (ProtoContextPtr ctx, long channelNumber)
{
  long status;
  ProtoChannelPtr chanPtr;

  status = 0;
  chanPtr = ctx->channel + channelNumber;

  ADD_PROTO_TRACE_L2 (ProtoTraceBufPtr, kTraceChannelClose, channelNumber, 0,
                      0);

  /* If channel was never enabled and initalized, don't bother */
  if (IsProtoChanInitalized (chanPtr) && IsProtoChanEnabled (chanPtr))
    {
      /* Stop any activity and flush pending buffers if any */
      status = FlushProtoChannel (ctx, channelNumber);

      /* Reset all of the channel's variables */
      chanPtr->status = 0;
      chanPtr->dataMsgQueue.head = NULL;
      chanPtr->dataMsgQueue.tail = NULL;
    }

  return status;
}

/*******************************************************************************************
 * A valid data chunk has arrived.  Do whatever needs to happen.
 * Chances are that you will want to pass this chunk to some routine that
 *interface with the part of the system that this subscriber drives.  Having
 *been down this path before let me take a a minute recommend that you
 *implement a separate module which contains a set of interfaces to the part of
 *the system that you want to use. The SAudioSubscriber was converted to do
 *this way too late in the game and so is more unclean than necessary.
 *******************************************************************************************/
long
ProcessNewProtoDataChunk (ProtoContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long status;
  ProtoDataChunkPtr protoData;
  long channelNumber;
  ProtoChannelPtr chanPtr;

  status = 0;
  protoData = (ProtoDataChunkPtr)subMsg->msg.data.buffer;
  channelNumber = protoData->channel;
  chanPtr = ctx->channel + channelNumber;

  ADD_PROTO_TRACE_L3 (ProtoTraceBufPtr, kTraceChannelNewDataArrived,
                      channelNumber, 0, subMsg);

  /* Don't do anything if this channel was never enabled.  Channels can be
   * enabled with a SetChan call, channel 0 is enabled by default.
   */
  if (IsProtoChanEnabled (chanPtr))
    {
      /* If this were a real subscriber here is where we would actually
       * do something with the data.  Since this is just a prototype, we
       * simply reply to msg.
       *
       * Here is how you would add the new chunk to
       * the end of this channel's data msg queue:
       *   AddDataMsgToTail( &chanPtr->dataMsgQueue, newSubMsg );
       *
       */
      status = ReplyMsg (subMsg->msgItem, kDSNoErr, subMsg,
                         sizeof (SubscriberMsg));
      CHECKRESULT ("ProcessNewProtoDataChunk() - ReplyMsg()", status);
    }
  else
    {
      /* Somehow we got a data chunk but never got a header chunk to init
       * the channel.  This is bad news and should not happen.  Perhaps
       * someone caused the streamer to branch into the middle of a
       * stream?  Or somone disabled a channel inapropriately?
       */
      status = ReplyMsg (subMsg->msgItem, kDSNoErr, subMsg,
                         sizeof (SubscriberMsg));
      CHECKRESULT ("ProcessNewProtoDataChunk() - ReplyMsg()", status);

      ERR (("Got a data msg for an disabled channel!!\n"));
    }

  return status;
}

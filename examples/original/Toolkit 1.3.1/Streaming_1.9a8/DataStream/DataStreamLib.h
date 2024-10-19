/*******************************************************************************************
 *	File:			DataStreamLib.h
 *
 *	Contains:		Interface routines for DataStreamLib.h & data
 *stream threads.
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	12/14/93	jb		Added DSSetBranchDest() routine.
 *	10/4/93		jb		Add DSWaitEndOfStream() for end of
 *stream registration.
 *	8/16/93		jb		Change API to accept a DSRequestMsgPtr
 *to indicate synchronous or asynchronous operation. If NULL is specified, the
 *operation is performed syncrhonously, using a message buffer allocated on the
 *caller's stack. Otherwise, the caller must not reuse the buffer until the
 *request is replied to by the streamer. 8/5/93		jb Add
 *_SubscriberBroadcast() routine. Change interface to DSClockSync() to use new
 *async broadcast mechanism.
 *	7/9/93		jb		Add 'fAsync' flag to all streamer
 *implemented functions to allow the caller to specify asynchronous operation.
 *If set to TRUE, the routine returns as soon as the request is sent to the
 *streamer and the result code indicates success of _sending_ the request. The
 *						caller must examine the reply message
 *result field for the result of the actual operation. If set to FALSE, then
 *control does not return to the caller until the operation is completed, and
 *the result is the result of the actual operation. 6/24/93		jb
 *Add options argument to DSGoMarker()
 *	6/22/93		jb		Added options to start and stop
 *subscriber messages to allow data flushing options to be communicated when a
 *stream is started or stopped. Added 'exemptStreamType' to DSClockSync() to
 *prevent deadlock. 6/15/93		jb		Add DSIsMarker() 6/8/93
 *jb		Add DSSetClock() and DSGetClock() 5/20/93		jb
 *Add DSConnect() 5/18/93		jb		Remove reference to
 *DSHCloseStream() 5/12/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __DATASTREAMLIB_H__
#define __DATASTREAMLIB_H__

#ifndef __DATASTREAM_H__
#include "DataStream.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  int32 _SendRequestToDSThread (Item msgItem, Boolean fAsync,
                                DSStreamCBPtr streamCBPtr,
                                DSRequestMsgPtr reqMsg);

  int32 _SubscriberBroadcast (DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr,
                              SubscriberMsgPtr subMsg);

  Boolean FillPoolWithMsgItems (MemPoolPtr memPool, Item replyPort);

  int32 DSSubscribe (Item msgItem, DSRequestMsgPtr reqMsg,
                     DSStreamCBPtr streamCBPtr, DSDataType dataType,
                     Item subscriberPort);

  int32 DSPreRollStream (Item msgItem, DSRequestMsgPtr reqMsg,
                         DSStreamCBPtr streamCBPtr);

  int32 DSStartStream (Item msgItem, DSRequestMsgPtr reqMsg,
                       DSStreamCBPtr streamCBPtr, unsigned long startOptions);

  int32 DSStopStream (Item msgItem, DSRequestMsgPtr reqMsg,
                      DSStreamCBPtr streamCBPtr, unsigned long stopOptions);

  int32 DSClockSync (DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr,
                     unsigned long nowTime);

  int32 DSGoMarker (Item msgItem, DSRequestMsgPtr reqMsg,
                    DSStreamCBPtr streamCBPtr, unsigned long markerValue,
                    unsigned long options);

  int32 DSGetChannel (Item msgItem, DSRequestMsgPtr reqMsg,
                      DSStreamCBPtr streamCBPtr, DSDataType streamType,
                      long channelNumber, long *channelStatusPtr);

  int32 DSSetChannel (Item msgItem, DSRequestMsgPtr reqMsg,
                      DSStreamCBPtr streamCBPtr, DSDataType streamType,
                      long channelNumber, long channelStatus);

  int32 DSControl (Item msgItem, DSRequestMsgPtr reqMsg,
                   DSStreamCBPtr streamCBPtr, DSDataType streamType,
                   long userDefinedOpcode, void *userDefinedArgPtr);

  int32 DSConnect (Item msgItem, DSRequestMsgPtr reqMsg,
                   DSStreamCBPtr streamCBPtr, Item acquirePort);

  int32 DSSetClock (DSStreamCBPtr streamCBPtr, uint32 newStreamClock);

  int32 DSGetClock (DSStreamCBPtr streamCBPtr, uint32 *streamClock);

  int32 DSIsMarker (DSStreamCBPtr streamCBPtr, uint32 markerValue,
                    Boolean *fIsMarker);

  int32 DSWaitEndOfStream (Item msgItem, DSRequestMsgPtr reqMsg,
                           DSStreamCBPtr streamCBPtr);

  int32 DSSetBranchDest (DSStreamCBPtr streamCBPtr, uint32 branchTimeDest);

#ifdef __cplusplus
}
#endif

#endif

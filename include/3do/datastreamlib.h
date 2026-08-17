/*******************************************************************************************
 *	File:			DataStreamLib.h
 *
 *	Contains:		Interface routines for DataStreamLib.h & data stream threads.
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright (c) 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  7/09/94		dtc		Version 2.0.1d1
 *						Deleted ifndef __DATASTREAM_H__.  This define should be part
 *						of the DataStream.h header.
 *	5/19/94		fyp		Version 2.0
 *						Integrated dlg's changes (3.30.94).  Added DSSetBranchType() routine.
 *	3/14/94		lla		Replaced IsKeyFrame() with DSGetChunkFlag() routine to enable
 *						reading chunkFlags as well as chunk cues.
 *	3/4/94		lla		Added IsKeyFrame() routine. & Added DSSetSkipMode proto.
 *	1/20/94		rdg		make C++ compatible
 *	12/14/93	jb		Added DSSetBranchDest() routine.
 *	10/4/93		jb		Add DSWaitEndOfStream() for end of stream registration.
 *	8/16/93		jb		Change API to accept a DSRequestMsgPtr to indicate synchronous
 *						or asynchronous operation. If NULL is specified, the operation
 *						is performed syncrhonously, using a message buffer allocated
 *						on the caller's stack. Otherwise, the caller must not reuse
 *						the buffer until the request is replied to by the streamer.
 *	8/5/93		jb		Add _SubscriberBroadcast() routine. Change interface to
 *						DSClockSync() to use new async broadcast mechanism.
 *	7/9/93		jb		Add 'fAsync' flag to all streamer implemented functions to allow
 *						the caller to specify asynchronous operation. If set to TRUE, the
 *						routine returns as soon as the request is sent to the streamer and
 *						the result code indicates success of _sending_ the request. The
 *						caller must examine the reply message result field for the result
 *						of the actual operation. If set to FALSE, then control does not
 *						return to the caller until the operation is completed, and the
 *						result is the result of the actual operation.
 *	6/24/93		jb		Add options argument to DSGoMarker()
 *	6/22/93		jb		Added options to start and stop subscriber messages to allow
 *						data flushing options to be communicated when a stream is
 *						started or stopped.
 *						Added 'exemptStreamType' to DSClockSync() to prevent deadlock.
 *	6/15/93		jb		Add DSIsMarker()
 *	6/8/93		jb		Add DSSetClock() and DSGetClock()
 *	5/20/93		jb		Add DSConnect()
 *	5/18/93		jb		Remove reference to DSHCloseStream()
 *	5/12/93		jb		New today.
 *
 *******************************************************************************************/

#pragma once

#include "extern_c.h"

#include "datastream.h"

EXTERN_C_BEGIN

s32	_SendRequestToDSThread( Item            msgItem, boolean fAsync, DSStreamCBPtr streamCBPtr,
                                DSRequestMsgPtr reqMsg );

s32	_SubscriberBroadcast( DSStreamCBPtr    streamCBPtr, MemPoolPtr msgPoolPtr,
                              SubscriberMsgPtr subMsg );

boolean FillPoolWithMsgItems( MemPoolPtr memPool, Item replyPort );

s32	DSSubscribe( Item       msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                     DSDataType dataType, Item subscriberPort );

s32	DSPreRollStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr );

s32	DSStartStream( Item          msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                       unsigned long startOptions );

s32	DSStopStream( Item          msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                      unsigned long stopOptions );

s32	DSClockSync( DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr,
                     unsigned long nowTime );

s32	DSGoMarker( Item          msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                    unsigned long markerValue, unsigned long options );

s32	DSGetChannel( Item       msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                      DSDataType streamType, long channelNumber,
                      long*      channelStatusPtr );

s32	DSSetChannel( Item       msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                      DSDataType streamType, long channelNumber, long channelStatus );

s32	DSControl( Item       msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                   DSDataType streamType, long userDefinedOpcode,
                   void*      userDefinedArgPtr );

s32	DSConnect( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                   Item acquirePort );

s32	DSSetClock( DSStreamCBPtr streamCBPtr, u32 newStreamClock );

s32	DSGetClock( DSStreamCBPtr streamCBPtr, u32* streamClock );

s32	DSIsMarker( DSStreamCBPtr streamCBPtr, u32 markerValue, boolean* fIsMarker );

s32	DSWaitEndOfStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr );

s32	DSSetBranchDest( DSStreamCBPtr streamCBPtr, u32 branchTimeDest );

s32	DSSetBranchType( DSStreamCBPtr streamCBPtr );


#if RELATIVE_BRANCHING
s32 DSSetSkipMode( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
                       unsigned long markerValue, unsigned long options );
s32 FlushAllSubscribers( DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr, StreamChunkPtr cp );

s32 DSGetChunkFlag( StreamChunkPtr cp );
#endif

EXTERN_C_END

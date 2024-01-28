/*******************************************************************************************
 *	File:			PlayEZQStream.h
 *
 *	Contains:		definitions for high level stream playback
 *function
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  7/6/94			WSD		Added frameCount to Player
 *	10/25/93		jb		Add userContext to user function
 *calling sequence 10/14/93		jb		New today
 *
 *******************************************************************************************/

#ifndef _PLAYSTREAM_H_
#define _PLAYSTREAM_H_

#ifndef __GRAPHICS_H
#include "Graphics.h"
#endif

#ifndef _TYPES_H
#include "Types.h"
#endif

#ifndef _PREPARESTREAM_H_
#include "PrepareStream.h"
#endif

#include "ControlSubscriber.h"
#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "EZQSubscriber.h"
#include "SAudioSubscriber.h"

#define kMovieOK 0
#define kRewindMovie 1
#define kStopMovie 2
#define kDSContinuousFrame 65
#define kDSStepFrame 66
#define kDSDrawSingleFrame 67

typedef long (*PlayEZQUserFn) (void *ctx, void *userContext);

/**************************************/
/* Internal player context descriptor */
/**************************************/
typedef struct Player
{

  PlayEZQUserFn userFn; /* user callback function pointer */
  void *userContext;    /* for use by user callback function */

  DSHeaderChunk hdr; /* Copy of stream header from stream file */

  DSDataBufPtr bufferList; /* ptr to linked list of buffers used by streamer */
  AcqContextPtr acqContext;  /* ptr to data acquisition thread's context */
  DSStreamCBPtr streamCBPtr; /* ptr to stream thread's context */

  ScreenContext *screenContextPtr; /* for drawing to the screen */
  Item VBLIOReq;                   /* ioreq for calling WaitVBL() */

  Item messagePort;            /* port for receiving end of stream message */
  Item messageItem;            /* msg item for sending streamer requests */
  Item endOfStreamMessageItem; /* msg item that is replied to as end of stream
                                */

  CtrlContextPtr controlContextPtr; /* Control subscriber context ptr */

  SAudioContextPtr audioContextPtr; /* Audio subscriber context ptr */

  EZQContextPtr ezqContextPtr; /* Cinepak subscriber context ptr */
  EZQRecPtr ezqChannelPtr;     /* Cinepak subscriber channel ptr */
  int32 frameCount;            /* WSD */
} Player, *PlayerPtr;

#ifndef _MESG_POOL_H_
#define _MESG_POOL_H_

#include "PlayEZQStream.h"

typedef struct PlayerFuntionContext
{
  PlayerPtr playerCtx;
  MemPoolPtr dsMsgPool;
  Item replyPort;
} PlayerFuntionContext, *PlayerFuntionContextPtr;

#endif /* _MESG_POOL_H_ */

/*****************************/
/* Public routine prototypes */
/*****************************/

int32 PlayEZQStream (ScreenContext *screenContextPtr, char *streamFileName,
                     PlayEZQUserFn userFn, void *userContext,
                     PlayerFuntionContext *pfc);
void DismantlePlayer (PlayerPtr ctx);

#endif /* _PLAYSTREAM_H_ */

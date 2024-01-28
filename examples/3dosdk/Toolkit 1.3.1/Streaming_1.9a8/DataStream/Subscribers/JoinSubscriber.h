/*******************************************************************************************
 *	File:			JoinSubscriber.h
 *
 *	Contains:		definitions for JoinSubscriber.c
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe
 *Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	11/29/93	jb		Integrated changes from Neil Cormia's
 *version of the code. 11/23/93	jb		Added prototype for
 *CloseJoinSubscriber() 11/18/93	njc		Renamed _FlushJoin to
 *FlushJoinSubscriber and made it global in scope. 6/16/93		jb
 *Added JOIN_CHUNK_TYPE, JOIN_HEADER_SUBTYPE, JOIN_DATA_SUBTYPE 5/16/93
 *njc		New today.
 *
 *******************************************************************************************/
#ifndef __JOINSUBSCRIBER_H__
#define __JOINSUBSCRIBER_H__

#include "DataStreamLib.h"
#include "SubscriberUtils.h"

/**********************/
/* Internal constants */
/**********************/

#define JOIN_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('J', 'O', 'I', 'N') /* chunk type for this subscriber */
#define JOIN_HEADER_SUBTYPE                                                   \
  CHAR4LITERAL ('J', 'H', 'D', 'R') /* subtype for header blocks */
#define JOIN_DATA_SUBTYPE                                                     \
  CHAR4LITERAL ('J', 'D', 'A', 'T') /* subtype for continuation blocks */

#define DATA_MAX_SUBSCRIPTIONS                                                \
  2 /* max # of streams that can use this subscriber */
#define DATA_MAX_ELEMENTS 64 /* max # of data elements per subscriber */
#define MAX_PORTS 8          /* max # of different data types supported */

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct JoinElementMsg
{
  DS_MSG_HEADER
  int32 dataType;    /* 4 character data type (e.g. ANIM) */
  int32 dataChannel; /* entities within dataTypes are identified by channel */
  void *dataPtr;     /* pointer to local mem containing assembled data */
  int32 dataSize;    /* size of final assembled data */
  int32 dataOffset; /* current offset into the dataPtr. Put next chunk here. */
  int32 dataTime;   /* the stream time contained in the first join chunk */
} JoinElementMsg, *JoinElementMsgPtr;

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct JoinContext
{
  Item creatorTask;     /* who to signal when we're done initializing */
  uint32 creatorSignal; /* signal to send for synchronous completion */
  int32 creatorStatus;  /* result code for creator */

  Item threadItem;        /* subscriber thread item */
  void *threadStackBlock; /* pointer to thread's stack memory block */

  Item requestPort;         /* message port item for subscriber requests */
  uint32 requestPortSignal; /* signal to detect request port messages */

  MemPoolPtr joinElemMsgPool; /* Pool of msgs for sending data elements to
                                 display task */

  Item portListSem; /* semaphore to arbitrate access to the dataPort info */
  Item dataPort[MAX_PORTS];       /* message port item to send data elements */
  DSDataType dataType[MAX_PORTS]; /* message type for this port */
  Boolean userPort[MAX_PORTS];    /* true if client allocated the port for
                                     InitJoinPort */
  int32 numPorts;                 /* number of ports currently in use */

  Item replyPort;         /* message port item for subscriber requests */
  uint32 replyPortSignal; /* signal to detect request port messages */

  int32 localTimeOrigin; /* local version of the time */

  JoinElementMsgPtr
      dataMsgHead; /* ptr to a list of data that is not yet full */

  Boolean streamStopped; /* TRUE if kOpStopStream has been sent */

} JoinContext, *JoinContextPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct JoinChunkFirst
{
  SUBS_CHUNK_COMMON
  int32 joinChunkType; /* 'JHDR' for JoinChunkFirst  or 'JDAT' for
                          JoinChunkData */
  int32 totalDataSize; /* the total size of the data in all chunks */
  int32 ramType;       /* AllocMem flags for this type of data */
  int32 compType;      /* type of compression used on this data */
  int32 dataSize;      /* the size of the data in this chunk */
                       /* char		data[4];				   the data goes here...
                        */
} JoinChunkFirst, *JoinChunkFirstPtr;

typedef struct JoinChunkData
{
  SUBS_CHUNK_COMMON
  int32 joinChunkType; /* 'JHDR' for JoinChunkFirst  or 'JDAT' for
                          JoinChunkData */
  int32 dataSize;      /* the size of the data in this chunk */
                       /* char		data[4];				   the data goes here...
                        */
} JoinChunkData, *JoinChunkDataPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  /* Subscriber one-time init/shutdown routines */

  int32 InitJoinSubscriber (void);
  int32 CloseJoinSubscriber (void);

  /* New/dispose subscriber instance */

  int32 NewJoinSubscriber (JoinContextPtr *pCtx, int32 priority);
  int32 DisposeJoinSubscriber (JoinContextPtr ctx);

  /* Data port create/destroy */

  Int32 InitJoinPort (JoinContextPtr ctx, Item *joinPort, int32 dataType);
  void DestroyJoinPort (JoinContextPtr ctx, DSDataType dataType);

  /* Get/release data element */

  JoinElementMsgPtr GetJoinElement (JoinContextPtr ctx, Item joinPort);
  void ReleaseJoinElement (JoinContextPtr ctx, JoinElementMsgPtr joinElemPtr);

  /* Miscellaneous */

  int32 FlushJoinSubscriber (JoinContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif /* __JOINSUBSCRIBER_H__ */

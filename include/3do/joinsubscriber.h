#pragma once

#include "extern_c.h"

#include "datastreamlib.h"
#include "subscriberutils.h"

/**********************/
/* Internal constants */
/**********************/

#define	JOIN_CHUNK_TYPE	    CHAR4LITERAL('J','O','I','N') /* chunk type for this subscriber */
#define	JOIN_HEADER_SUBTYPE CHAR4LITERAL('J','H','D','R') /* subtype for header blocks */
#define	JOIN_DATA_SUBTYPE   CHAR4LITERAL('J','D','A','T') /* subtype for continuation blocks */

#define	DATA_MAX_SUBSCRIPTIONS 2 /* max # of streams that can use this subscriber */
#define	DATA_MAX_ELEMENTS      64 /* max # of data elements per subscriber */
#define	MAX_PORTS	       8 /* max # of different data types supported */


/* flag bits for the join element message */
#define JF_RELEASEMEMORY	0x0000 /* the default */
#define JF_RELEASERESOURCESONLY	0x0001 /* release everything but the data block */


/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct JoinElementMsg {
  DS_MSG_HEADER;
  s32	dataType;		/* 4 character data type (e.g. ANIM) */
  s32	dataChannel;		/* entities within dataTypes are identified by channel */
  void*	dataPtr;		/* pointer to local mem containing assembled data */
  s32	dataSize;		/* size of final assembled data */
  s32	dataOffset;		/* current offset into the dataPtr. Put next chunk here. */
  s32	dataTime;		/* the stream time contained in the first join chunk */
  s32	flags;
} JoinElementMsg, *JoinElementMsgPtr;


/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct JoinContext {
  Item              creatorTask; /* who to signal when we're done initializing */
  u32            creatorSignal; /* signal to send for synchronous completion */
  s32             creatorStatus; /* result code for creator */
  Item              threadItem; /* subscriber thread item */
  void*             threadStackBlock; /* pointer to thread's stack memory block */
  Item              requestPort; /* message port item for subscriber requests */
  u32            requestPortSignal; /* signal to detect request port messages */
  MemPoolPtr        joinElemMsgPool; /* Pool of msgs for sending data elements to display task */
  Item              portListSem; /* semaphore to arbitrate access to the dataPort info */
  Item              dataPort[MAX_PORTS]; /* message port item to send data elements */
  DSDataType        dataType[MAX_PORTS]; /* message type for this port */
  boolean           userPort[MAX_PORTS]; /* true if client allocated the port for InitJoinPort */
  s32             numPorts;   /* number of ports currently in use */
  Item              replyPort;  /* message port item for subscriber requests */
  u32            replyPortSignal; /* signal to detect request port messages */
  s32             localTimeOrigin; /* local version of the time */
  JoinElementMsgPtr dataMsgHead; /* ptr to a list of data that is not yet full */
  boolean           streamStopped; /* TRUE if kOpStopStream has been sent */
  JoinElementMsgPtr JoinElemMsgWaitingPtr; /* Unfinish message */
  SubscriberMsgPtr  psubMsgWaiting; /* Info to finish the unfinished message */
  boolean 	    bRequestSignal; /* boolean indicating a request needs to be processed */
} JoinContext, *JoinContextPtr;


/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct JoinChunkFirst {
  SUBS_CHUNK_COMMON;
  s32	joinChunkType;		/* 'JHDR' for JoinChunkFirst  or 'JDAT' for JoinChunkData */
  s32	totalDataSize;		/* the total size of the data in all chunks */
  s32	ramType;		/* AllocMem flags for this type of data */
  s32	compType;		/* type of compression used on this data */
  s32	dataSize;		/* the size of the data in this chunk */
  /* char		data[4];				   the data goes here... */
} JoinChunkFirst, *JoinChunkFirstPtr;

typedef struct JoinChunkData {
  SUBS_CHUNK_COMMON;
  s32	joinChunkType;		/* 'JHDR' for JoinChunkFirst  or 'JDAT' for JoinChunkData */
  s32	dataSize;		/* the size of the data in this chunk */
  /* char		data[4];				   the data goes here... */
} JoinChunkData, *JoinChunkDataPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

EXTERN_C_BEGIN

/* Subscriber one-time init/shutdown routines */

s32 InitJoinSubscriber( void );
s32 CloseJoinSubscriber( void );

/* New/dispose subscriber instance */

s32 NewJoinSubscriber( JoinContextPtr *pCtx, s32 priority );
s32 DisposeJoinSubscriber( JoinContextPtr ctx );

/* Data port create/destroy */

s32 InitJoinPort( JoinContextPtr ctx, Item *joinPort, s32 dataType );

void  DestroyJoinPort( JoinContextPtr ctx, DSDataType dataType);

/* Get/release data element */

JoinElementMsgPtr GetJoinElement( JoinContextPtr ctx, Item joinPort );
void		  ReleaseJoinElement( JoinContextPtr ctx, JoinElementMsgPtr joinElemPtr );
void		  ReleaseJoinElementResources( JoinContextPtr ctx, JoinElementMsgPtr joinElemPtr );

/* Miscellaneous */

s32 FlushJoinSubscriber( JoinContextPtr ctx );

EXTERN_C_END

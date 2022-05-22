#pragma include_only_once

#include "extern_c.h"

#include "datastream.h"
#include "subscriberutils.h"

#include "protoerrors.h"
#include "prototracecodes.h"
#include "protochannels.h"

#include "haltchunk.h"

/* Max # of streams that can use this subscriber */
#define	PR_SUBS_MAX_SUBSCRIPTIONS	4

/* Stream version supported by this subscriber.  Used for sanity
 * checking when a header chunk is received.
 */
#define PROTO_STREAM_VERSION		0

/* Subscriber data type.  This ID is used is used when registering a subscriber with
 * the streamer so that the streamer will know what sort of data gets delivered
 * to our thread.
 */
#define	PRTO_CHUNK_TYPE	CHAR4LITERAL('P','R','T','O')

/* Header subchunk type.  Header chunks contain one time initalization and
 * configuration stuff for a given channel.
 */
#define	PHDR_CHUNK_TYPE	CHAR4LITERAL('P','H','D','R')

/* Data subchunk type. The actual data that will be consumed. */
#define	PDAT_CHUNK_TYPE	CHAR4LITERAL('P','D','A','T')

/* HALT subchunk type.  Used to halt data flow in the streamer until we
 * reply.  Useful for time consuming subscriber level initalization tasks. */
#define	PHLT_CHUNK_TYPE	CHAR4LITERAL('P','H','L','T')

/************************************************/
/* Subscriber Header and Data chunk definitions */
/************************************************/

typedef	struct ProtoHeaderChunk {
  SUBS_CHUNK_COMMON;
  long version;			/*	version control for proto stream data */
} ProtoHeaderChunk, *ProtoHeaderChunkPtr;

typedef struct ProtoDataChunk {
  SUBS_CHUNK_COMMON;
  long actualDataSize;		/* Number of actual data bytes in chunk,
                                   i.e. not including the chunk header */
  char data[4];		        /* start of test data */
} ProtoDataChunk, *ProtoDataChunkPtr;

typedef struct ProtoHaltChunk {
  SUBS_CHUNK_COMMON;		/* Just the subscriber part of the halt chunk */
  long haltDuration;		/* How long to wait before replying */
} ProtoHaltChunk, *ProtoHaltChunkPtr;

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct ProtoContext {
  Item	creatorTask;		/* who to signal when we're done initializing */
  ulong	creatorSignal;		/* signal to send for synchronous completion */
  long	creatorStatus;		/* result code for creator */

  Item	threadItem;		/* subscriber thread item */
  void*	threadStackBlock;	/* pointer to thread's stack memory block */

  Item	requestPort;		/* message port item for subscriber requests */
  ulong	requestPortSignal;	/* signal to detect request port messages */

  DSStreamCBPtr	streamCBPtr;	/* stream this subscriber belongs to */

  ProtoChannel channel[PR_SUBS_MAX_CHANNELS]; /* an array of channels */

} ProtoContext, *ProtoContextPtr;


/***********************************************/
/* Opcode values as passed in control messages */
/***********************************************/
enum ProtoControlOpcode
  {
   kProtoCtlOpTest = 0
  };

/**************************************/
/* A Control block 					  */
/**************************************/
typedef union ProtoCtlBlock {
  struct {			/* kProtoCtlOpTest */
    long channelNumber;         /* Logical channel to send value to */
    long aValue;		/* test value */
  } test;
} ProtoCtlBlock, *ProtoCtlBlockPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

EXTERN_C_BEGIN

long InitProtoSubscriber(void);
long CloseProtoSubscriber(void);
long NewProtoSubscriber(ProtoContextPtr *pCtx,
                        DSStreamCBPtr    streamCBPtr,
                        long             deltaPriority);
long DisposeProtoSubscriber(ProtoContextPtr ctx);

EXTERN_C_END

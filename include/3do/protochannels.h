#pragma include_only_once

#include "extern_c.h"

#include "types.h"
#include "mempool.h"
#include "subscriberutils.h"

/* Max number of logical channels per subscription */
#define	PR_SUBS_MAX_CHANNELS		8

/* Defines for dealing with channel status bits.
 *
 * Channel Enabled 		- means that a channel is prepared to queue data msgs.
 *					 	  Channel 0 defaults to enabled, but all other channels
 *						  must be explicitly set with a DoSetChan() call.
 *
 * Channel Active  		- means that a "stream started" msg has been received.
 *
 * Channel Initalized 	- means that a header chunk has arrived for this channel.
 */

/* Define a channel status flag to indicate wheather a channel
 * has been initalized.
 * Bits 0-15 are reserved by the streamer; bit 0 (CHAN_ENABLED) is
 * R/W by the user with DoSetChan(), but bits 1-15 are R/O to the user.
 * See DataStream.h.
 */
#define	PROTO_CHAN_INITALIZED (1<<16)

/* Define a mask for subscriber reserved R/O bits.
 * Make sure that the user can't set/unset these bit(s)
 * using DoSetChan().  This mask is ORed with CHAN_SYSBITS
 * in the subscriber's implementation of DoSetChan().
 */
#define PROTO_CHAN_SUBSBITS (0x10000)

#define IsProtoChanEnabled(x)	 (x->status & CHAN_ENABLED)
#define IsProtoChanActive(x)	 (x->status & CHAN_ACTIVE)
#define IsProtoChanInitalized(x) (x->status & PROTO_CHAN_INITALIZED)

/****************************************************************/
/* Subscriber logical channel, SA_SUBS_MAX_CHANNELS per context */
/****************************************************************/

typedef struct ProtoChannel {
  ulong	    status;		/* state bits */
  SubsQueue dataMsgQueue;	/* queued data chunks */
} ProtoChannel, *ProtoChannelPtr;

/****************************************************************
 * We need the following forward references because the routines
 * we prototype here will be called with pointers to structs
 * that will be defined in ProtoSubscriber.h
 ****************************************************************/

struct ProtoContext;
struct ProtoHeaderChunk;

/**********************************************/
/* Public function prototypes for this module */
/**********************************************/

EXTERN_C_BEGIN

long InitProtoChannel(struct ProtoContext* ctx, struct ProtoHeaderChunk* headerPtr);
long StartProtoChannel(struct ProtoContext* ctx, long channelNumber);
long StopProtoChannel(struct ProtoContext* ctx, long channelNumber);
long FlushProtoChannel(struct ProtoContext* ctx, long channelNumber);
long CloseProtoChannel(struct ProtoContext* ctx, long channelNumber);

long ProcessNewProtoDataChunk(struct ProtoContext* ctx, SubscriberMsgPtr subMsg);

EXTERN_C_END

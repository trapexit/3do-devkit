#pragma include_only_once

#include "extern_c.h"

//#include "satastream.h"
#include "subscriberutils.h"
#include "sastreamchunks.h"
#include "sachannel.h"
#include "satemplates.h"
#include "sacontrolmsgs.h"


#define SAUDIO_STREAM_VERSION 0	/* stream version supported by subscriber */

#define	SNDS_CHUNK_TYPE	CHAR4LITERAL('S','N','D','S') /* subscriber data type */
#define	SHDR_CHUNK_TYPE	CHAR4LITERAL('S','H','D','R') /* header subchunk type */
#define	SSMP_CHUNK_TYPE	CHAR4LITERAL('S','S','M','P') /* sample subchunk type */

#define	SA_SUBS_MAX_SUBSCRIPTIONS 4 /* max # of streams that can use this subscriber */



typedef struct SAudioContext {
  Item           creatorTask;	/* who to signal when we're done initializing */
  ulong          creatorSignal;	/* signal to send for synchronous completion */
  long           creatorStatus;	/* result code for creator */
  Item           threadItem;	/* subscriber thread item */
  void*          threadStackBlock; /* pointer to thread's stack memory block */
  Item           requestPort;	/* message port item for subscriber requests */
  ulong          requestPortSignal; /* signal to detect request port messages */
  DSStreamCBPtr  streamCBPtr;	/* stream this subscriber belongs to */
  TemplateRecPtr templateArray;	/* ptr to array of template records */
  ulong		 allBufferSignals; /* or's signals for all instruments on all channels */
  Item		 outputTemplateItem; /* template item for making channel's output instrument */
  Item		 envelopeTemplateItem; /* template item for making channel's envelope instrument(s) */
  Item		 decodeADPCMIns; /* Support for playing ADPCM data */
  long		 clockChannel;	/* which logical channel to use for clock */
  SAudioChannel  channel[SA_SUBS_MAX_CHANNELS]; /* an array of channels */
} SAudioContext, *SAudioContextPtr;

EXTERN_C_BEGIN

long InitSAudioSubscriber(void);
long CloseSAudioSubscriber(void);

long NewSAudioSubscriber(SAudioContextPtr *pCtx, DSStreamCBPtr streamCBPtr, long deltaPriority);
long DisposeSAudioSubscriber(SAudioContextPtr ctx);

EXTERN_C_END

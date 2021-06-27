#ifndef __SUBSCRIBERUTILS_H__
#define __SUBSCRIBERUTILS_H__

#ifdef __CC_NORCROFT
#include "datastreamlib.h"
#endif

#include "subschunkcommon.h"


#define	CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)

#ifdef __CC_NORCROFT
typedef struct SubsQueue {
  SubscriberMsgPtr	head;			/* head of message queue */
  SubscriberMsgPtr	tail;			/* tail of message queue */
} SubsQueue, *SubsQueuePtr;


typedef struct SubsChannel {
  unsigned long		status;			/* state bits */
  SubsQueue			msgQueue;		/* queue of subscriber messages */
} SubsChannel, *SubsChannelPtr;


#ifdef __cplusplus
extern "C" {
#endif

  boolean	   AddDataMsgToTail( SubsQueuePtr subsQueue, SubscriberMsgPtr subMsg );
  SubscriberMsgPtr GetNextDataMsg( SubsQueuePtr subsQueue );

#ifdef __cplusplus
}
#endif

#endif  /* __CC_NORCROFT */

#endif	/* __SUBSCRIBERUTILS_H__ */

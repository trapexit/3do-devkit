/*******************************************************************************************
 *	File:			SubscriberUtils.h
 *
 *	Contains:		Common definitions for data stream subscribers
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	6/15/93		jb		Include ";" at the end of the SUBS_CHUNK_COMMON macro.
 *						Make queuing routines generalized to not require common channel
 *						header in order to use them. Removed AUDIO_TICKS_PER_SEC since
 *						the audio clock time base can be set by users (hence, this is not
 *						really a constant).
 *	6/2/93		jb		Removed extraneous #include of "graphics.h"
 *	5/15/93		njc		Removed underscores from the _AddDataMsgToTail and _GetNextDataMsg 
 *						routines.
 *	5/5/93		njc		New today.
 *
 *******************************************************************************************/
#ifndef __SUBSCRIBERUTILS_H__
#define __SUBSCRIBERUTILS_H__

#ifdef __CC_NORCROFT
	#ifndef __DATASTREAMLIB_H__
	#include "DataStreamLib.h"
	#endif
#endif

/**********************/
/* Internal constants */
/**********************/

/* The following macro makes a 32-bit unsigned long scalar out of 4
 * char's as input. This macro is included to avoid compiler warnings from 
 * compilers that object to 4 character literals, for example, 'IMAG'.
 */

#define	CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)


/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define	SUBS_CHUNK_COMMON	\
	long		chunkType;		/* chunk type */					\
	long		chunkSize;		/* chunk size including header */	\
	long		time;			/* position in stream time */		\
	long		channel;		/* logical channel number */		\
	long		subChunkType;	/* data sub-type */

typedef	struct SubsChunkData {
	SUBS_CHUNK_COMMON
} SubsChunkData, *SubsChunkDataPtr;


#ifdef __CC_NORCROFT
/************************************************************/
/* General subscriber message queue structure with support	*/
/* for fast queuing of entries in a singly linked list.		*/
/************************************************************/
typedef struct SubsQueue {
	SubscriberMsgPtr	head;			/* head of message queue */
	SubscriberMsgPtr	tail;			/* tail of message queue */
	} SubsQueue, *SubsQueuePtr;


/************************************************/
/* Channel context, one per channel, per stream	*/
/************************************************/
typedef struct SubsChannel {
	unsigned long		status;			/* state bits */
	SubsQueue			msgQueue;		/* queue of subscriber messages */
	} SubsChannel, *SubsChannelPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

Boolean				AddDataMsgToTail( SubsQueuePtr subsQueue, SubscriberMsgPtr subMsg );
SubscriberMsgPtr	GetNextDataMsg( SubsQueuePtr subsQueue );

#ifdef __cplusplus
}
#endif

#endif  /* __CC_NORCROFT */

#endif	/* __SUBSCRIBERUTILS_H__ */


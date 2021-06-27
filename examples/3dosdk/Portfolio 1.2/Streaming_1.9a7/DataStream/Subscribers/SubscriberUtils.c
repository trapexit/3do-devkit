/*******************************************************************************************
 *	File:			Subscriber.c
 *
 *	Contains:		Common routines for all subscribers
 *
 *	Written by:		Neil Cormia
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/15/93		jb		Switch to NOT assuming queue's position in a channel structure
 *						to allow reuse of this code for other subscriber queues. Now
 *						use a generic SubsQueuePtr instead of a channel ptr as input.
 *	5/5/93		njc		New today, copied here from separate subscriber code.
 *
 *******************************************************************************************/

#include "SubscriberUtils.h"


/*******************************************************************************************
 * Utility routine to add a data message onto the tail of the queue
 *******************************************************************************************/
Boolean		AddDataMsgToTail( SubsQueuePtr queue, SubscriberMsgPtr subMsg )
	{
	Boolean		fQueueWasEmpty;

	subMsg->link = NULL;

	if ( queue->head != NULL )
		{
		/* Add the new message onto the end of the 
		 * existing list of queued messages.
		 */
		queue->tail->link = (void *) subMsg;
		queue->tail = subMsg;
		fQueueWasEmpty = false;
		}
	else
		{
		/* Add the message to an empty queue  */
		queue->head = subMsg;
		queue->tail = subMsg;
		fQueueWasEmpty = true;
		}

	return fQueueWasEmpty;
	}


/*******************************************************************************************
 * Utility routine to remove a data message from the head of the queue of data messages
 * for the given channel.
 *******************************************************************************************/
SubscriberMsgPtr		GetNextDataMsg( SubsQueuePtr queue )
	{
	SubscriberMsgPtr	subMsg;

	if ( (subMsg = queue->head) != NULL )
		{
		/* Advance the head pointer to point to the next entry
		 * in the list.
		 */
		queue->head = (SubscriberMsgPtr) subMsg->link;

		/* If we are removing the tail entry from the list, set the
		 * tail pointer to NULL.
		 */
		if ( queue->tail == subMsg )
			queue->tail = NULL;
		}

	return subMsg;
	}



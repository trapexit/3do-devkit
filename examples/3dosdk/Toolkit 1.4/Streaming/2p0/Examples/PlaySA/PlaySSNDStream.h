/*******************************************************************************************
 *	File:			PlaySSNDStream.h
 *
 *	Contains:		definitions for high level stream playback function
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/25/93		rdg		Modify into SSND only stream player
 *	10/14/93		jb		New today
 *
 *******************************************************************************************/

#ifndef	_PLAYSTREAM_H_
#define	_PLAYSTREAM_H_

#ifndef _TYPES_H
#include "Types.h"
#endif

#ifndef	_PREPARESTREAM_H_
#include "PrepareStream.h"
#endif

#include "DataAcq.h"
#include "SAudioSubscriber.h"
#include "ControlSubscriber.h"
#include "DataStreamDebug.h"


typedef long (*PlaySSNDUserFn)( void *ctx );

/**************************************/
/* Internal player context descriptor */
/**************************************/
typedef struct Player {

	PlaySSNDUserFn		userFn;				/* user callback function pointer */
	void*				userCB;				/* Context for the user function */

	DSHeaderChunk		hdr;				/* Copy of stream header from stream file */

	DSDataBufPtr		bufferList;			/* ptr to linked list of buffers used by streamer */
	AcqContextPtr		acqContext;			/* ptr to data acquisition thread's context */
	DSStreamCBPtr		streamCBPtr;		/* ptr to stream thread's context */

	Item				messagePort;		/* port for receiving end of stream message */
	Item				messageItem;		/* msg item for sending streamer requests */
	Item				endOfStreamMessageItem; /* msg item that is replied to as end of stream */

	CtrlContextPtr		controlContextPtr;	/* Control subscriber context ptr */

	SAudioContextPtr	audioContextPtr;	/* Audio subscriber context ptr */

	} Player, *PlayerPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

int32	PlaySSNDStream( char* streamFileName, PlaySSNDUserFn userFn, void* userCB, 
							int32 callBackInterval );
void	DismantlePlayer( PlayerPtr ctx );

#endif	/* _PLAYSTREAM_H_ */

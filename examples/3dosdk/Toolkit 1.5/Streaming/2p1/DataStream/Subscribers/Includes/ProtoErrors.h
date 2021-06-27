/*********************************************************************************
 *	File:			ProtoErrors.h
 *
 *	Contains:		Error codes returned by ProtoSubscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993-94 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	3/23/94		rdg		New Today
 */
 
#ifndef __PROTOERRORS_H__
#define __PROTOERRORS_H__

#include "types.h"

enum ProtoErrorCode {
	ProtoErrStreamVersionInvalid,		/* Subscriber not compatible this version */
	ProtoErrChanOutOfRange				/* channel number not valid in context */
	};

#endif 	/* __PROTOERRORS_H__ */

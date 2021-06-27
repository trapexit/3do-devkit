#ifndef __MSGUTILS_H
#define __MSGUTILS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: msgutils.h,v 1.3 1994/10/05 17:34:41 vertex Exp $
**
**  Lib3DO message-related utilities.
**
******************************************************************************/


#include "msgport.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef union MsgValueTypes {	/* a variety of ways that the value fields in */
	int32		msgid;			/* a message can be interpreted.  many of these */
	int32		status;			/* are int32 synonyms which make your code */
	int32		result;			/* a little more readable by indicating how  */
	int32		num;			/* you're interpreting a value conceptually. */
	int32		inum;
	uint32		unum;
	Err			err;
	void *		ptr;
	Item		item;
} MsgValueTypes;

typedef int32 MSEventHandle;

typedef struct MSEventData {
	char *			name;
	int32			(*handler)(struct MSEventData *eventData, void *userContext);
	void *			userData;
	int32			signal;
	Item			port;
	Item			msgItem;
	Message *		msgPtr;
	MsgValueTypes *	msgValues;
	MSEventHandle	backLink;
} MSEventData;

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

int32			DispatchMSEvents(MSEventHandle mseHandle, void *userContext, int32 reserved);
void			CleanupMSEvents(MSEventHandle mseHandle);
MSEventHandle	SetupMSEvents(MSEventData eventData[], int32 numEvents, int32 reserved);
void			DisableMSEvent(MSEventData *theEvent, int32 reserved);
void			EnableMSEvent(MSEventData *theEvent, int32 reserved);

#ifdef __cplusplus
  }
#endif

#endif /* __MSGUTILS_H */

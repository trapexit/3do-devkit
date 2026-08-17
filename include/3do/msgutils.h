#pragma once

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

#include "extern_c.h"

#include "msgport.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef union MsgValueTypes {	/* a variety of ways that the value fields in */
  s32	 msgid;			/* a message can be interpreted.  many of these */
  s32	 status;		/* are s32 synonyms which make your code */
  s32	 result;		/* a little more readable by indicating how  */
  s32	 num;			/* you're interpreting a value conceptually. */
  s32	 inum;
  u32 unum;
  Err	 err;
  void * ptr;
  Item	 item;
} MsgValueTypes;

typedef s32 MSEventHandle;

typedef struct MSEventData {
  char *	  name;
  s32           (*handler)(struct MSEventData *eventData, void *userContext);
  void *	  userData;
  s32		  signal;
  Item		  port;
  Item		  msgItem;
  Message *	  msgPtr;
  MsgValueTypes * msgValues;
  MSEventHandle   backLink;
} MSEventData;

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

EXTERN_C_BEGIN

s32	      DispatchMSEvents(MSEventHandle mseHandle, void *userContext, s32 reserved);
void	      CleanupMSEvents(MSEventHandle mseHandle);
MSEventHandle SetupMSEvents(MSEventData eventData[], s32 numEvents, s32 reserved);
void	      DisableMSEvent(MSEventData *theEvent, s32 reserved);
void	      EnableMSEvent(MSEventData *theEvent, s32 reserved);

EXTERN_C_END

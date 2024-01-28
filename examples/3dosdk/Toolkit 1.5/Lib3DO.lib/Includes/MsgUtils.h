/*****************************************************************************
 *	File:			MsgUtils.h
 *
 *	Contains:		Message-related utilities.
 *
 *	Copyright (c) 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	06/04/94  Ian	Added MSEvents stuff to replace old MsgportMulti stuff.
 *	11/01/93  Ian 	New today.
 *
 ****************************************************************************/

#pragma include_only_once

#ifndef MSGUTILS_H
#define MSGUTILS_H

#include "msgport.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef union MsgValueTypes
{               // a variety of ways that the value fields in
  int32 msgid;  // a message can be interpreted.  many of these
  int32 status; // are int32 synonyms which make your code
  int32 result; // a little more readable by indicating how
  int32 num;    // you're interpreting a value conceptually.
  int32 inum;
  uint32 unum;
  Err err;
  void *ptr;
  Item item;
} MsgValueTypes;

typedef int32 MSEventHandle;

typedef struct MSEventData
{
  char *name;
  int32 (*handler) (struct MSEventData *eventData, void *userContext);
  void *userData;
  int32 signal;
  Item port;
  Item msgItem;
  Message *msgPtr;
  MsgValueTypes *msgValues;
  MSEventHandle backLink;
} MSEventData;

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

  int32 DispatchMSEvents (MSEventHandle mseHandle, void *userContext,
                          int32 reserved);
  void CleanupMSEvents (MSEventHandle mseHandle);
  MSEventHandle SetupMSEvents (MSEventData eventData[], int32 numEvents,
                               int32 reserved);
  void DisableMSEvent (MSEventData *theEvent, int32 reserved);
  void EnableMSEvent (MSEventData *theEvent, int32 reserved);

#ifdef __cplusplus
}
#endif

#endif

#ifndef __JUGGLER_H
#define __JUGGLER_H

/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: juggler.h,v 1.15 1994/09/10 00:17:48 peabody Exp $
**
**  Juggler Includes
**
**  By: Phil Burk
**
****************************************************************************/


#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "string.h"
#include "cobj.h"

typedef  uint32 Time;

/* Define basic root class Instance Variables for Juggler. */
#define JuggleeIV \
	COBObjectIV; \
	void   *jglr_Parent;  /* Who started you in hierarchy. */ \
	Time    jglr_StartTime; \
	Time    jglr_NextTime; \
	int32   jglr_RepeatCount; /* Decremented each time till zero */ \
	int32   jglr_Active; /* True if currently executing. */ \
	int32 (*jglr_StartFunction)(); \
	int32 (*jglr_RepeatFunction)(); \
	int32 (*jglr_StopFunction)(); \
	uint32  jglr_StartDelay; \
	uint32  jglr_RepeatDelay; \
	uint32  jglr_StopDelay; \
	int32   jglr_CurrentIndex;   /* Index of current thing. */ \
	void   *jglr_UserContext; \
	int32   jglr_Many;       /* Number of valid subunits */ \
	uint32  jglr_Flags

typedef struct
{
		JuggleeIV;
} Jugglee;

/* Flags for Juggler */
#define JGLR_FLAG_MUTE (0x0001)

extern COBClass JuggleeClass;

/* Sequence Structure */
#define SequenceIV \
	JuggleeIV; \
	int32 (*seq_InterpFunction)(); \
	int32   seq_Max;        /* Number of Events allocated. */ \
	int32   seq_EventSize;  /* Size in bytes of an event */ \
	char   *seq_Events     /* Pointer to event data */

typedef struct
{
	SequenceIV;
} Sequence;

extern COBClass SequenceClass;

/* Collection Structure ***************************************/
#define CollectionIV \
	JuggleeIV; \
	int32 (*col_SelectorFunction)(); \
	List    col_Children; \
	int32   col_Pending

typedef struct
{
	CollectionIV;
} Collection;


extern COBClass CollectionClass;

typedef struct
{
	List	jcon_ActiveObjects;
	Time	jcon_NextTime;
	Time	jcon_CurrentTime;
	Time	jcon_NextSignals;
	Time	jcon_CurrentSignals;
} JugglerContext;

extern JugglerContext JugglerCon;

typedef struct PlaceHolder
{
	Node     plch_Node;
	Jugglee *plch_Thing;
	int32    plch_NumRepeats;
} PlaceHolder;

/* Define TAG ARGS */
enum juggler_tags
{
        JGLR_TAG_CONTEXT = TAG_ITEM_LAST+1,
        JGLR_TAG_START_DELAY,
        JGLR_TAG_REPEAT_DELAY,
        JGLR_TAG_STOP_DELAY,
        JGLR_TAG_START_FUNCTION,
        JGLR_TAG_REPEAT_FUNCTION,
        JGLR_TAG_STOP_FUNCTION,
        JGLR_TAG_SELECTOR_FUNCTION,
        JGLR_TAG_INTERPRETER_FUNCTION,
        JGLR_TAG_DURATION,
        JGLR_TAG_MAX,
        JGLR_TAG_EVENTS,
        JGLR_TAG_EVENT_SIZE,
        JGLR_TAG_MANY,
        JGLR_TAG_MUTE
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 InitJuggler( void );
int32 TermJuggler( void );
int32 BumpJuggler( Time CurrentTime, Time *NextTime,
	int32 CurrentSignals, int32 *NextSignals);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __JUGGLER_H */

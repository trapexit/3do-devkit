#pragma once

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

#include "extern_c.h"

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

typedef  u32 Time;

/* Define basic root class Instance Variables for Juggler. */
#define JuggleeIV                                                       \
  COBObjectIV;                                                          \
  void   *jglr_Parent;  /* Who started you in hierarchy. */             \
  Time    jglr_StartTime;                                               \
  Time    jglr_NextTime;                                                \
  s32   jglr_RepeatCount; /* Decremented each time till zero */       \
  s32   jglr_Active; /* True if currently executing. */               \
  s32 (*jglr_StartFunction)();                                        \
  s32 (*jglr_RepeatFunction)();                                       \
  s32 (*jglr_StopFunction)();                                         \
  u32  jglr_StartDelay;                                              \
  u32  jglr_RepeatDelay;                                             \
  u32  jglr_StopDelay;                                               \
  s32   jglr_CurrentIndex;   /* Index of current thing. */            \
  void   *jglr_UserContext;                                             \
  s32   jglr_Many;       /* Number of valid subunits */               \
  u32  jglr_Flags

typedef struct
{
  JuggleeIV;
} Jugglee;

/* Flags for Juggler */
#define JGLR_FLAG_MUTE (0x0001)

extern COBClass JuggleeClass;

/* Sequence Structure */
#define SequenceIV                                              \
  JuggleeIV;                                                    \
  s32 (*seq_InterpFunction)();                                \
  s32   seq_Max;        /* Number of Events allocated. */     \
  s32   seq_EventSize;  /* Size in bytes of an event */       \
  char   *seq_Events     /* Pointer to event data */

typedef struct
{
  SequenceIV;
} Sequence;

extern COBClass SequenceClass;

/* Collection Structure ***************************************/
#define CollectionIV                            \
  JuggleeIV;                                    \
  s32 (*col_SelectorFunction)();              \
  List    col_Children;                         \
  s32   col_Pending

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
  s32    plch_NumRepeats;
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


EXTERN_C_BEGIN

s32 InitJuggler( void );
s32 TermJuggler( void );
s32 BumpJuggler( Time CurrentTime, Time *NextTime,
                   s32 CurrentSignals, s32 *NextSignals);

EXTERN_C_END

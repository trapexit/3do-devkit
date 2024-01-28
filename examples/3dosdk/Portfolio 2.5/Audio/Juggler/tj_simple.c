
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tj_simple.c,v 1.11 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/tj_simple
|||	tj_simple - Uses the juggler to play two sequences.
|||
|||	  Synopsis
|||
|||	    tj_simple
|||
|||	  Description
|||
|||	    This program tests the juggler using non-musical events and
software-based
|||	    "timing." It constructs two synthetic sequences based on the
TestData data
|||	    structure. It then "plays" the sequences via a simple print
function,
|||	    processed at the time of each event in each sequence.
|||
|||	  Associated Files
|||
|||	    tj_simple.c
|||
|||	  Location
|||
|||	    examples/Audio/Juggler
|||
**/

#include "audio.h"
#include "juggler.h"
#include "operror.h"
#include "stdio.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) PRT (x)

/* Macro to simplify error checking. */
#define CHECKPTR(ptr, name)                                                   \
  if (ptr == NULL)                                                            \
    {                                                                         \
      ERR (("Failure in %s\n", name));                                        \
      goto cleanup;                                                           \
    }

/****************************************************************************/
typedef struct
{
  Time te_TimeStamp;
  uint32 te_Data;
} TestEvent;

/* Define callback functions for juggler. */
int32
UserStartFunc (Jugglee *Self, Time StartTime)
{
  PRT (("Start function for 0x%x at Time %d\n", Self, StartTime));
  return 0;
}
int32
UserInterpFunc (Jugglee *Self, TestEvent *te, void *UserContext)
{
  PRT (("TestEvent: Self = 0x%x, Time = %d, Data = %d\n", Self,
        te->te_TimeStamp, te->te_Data));
  return 0;
}

TestEvent TestData[] = {
  /*   Time, Value */
  { 5, 567 },
  { 7, 910 },
  { 12, 400 }
};

#define STARTTIME (2000)
int
main ()
{
  Sequence *seq1 = NULL;
  Sequence *seq2 = NULL;
  int32 Result;
  Time CurTime, NextTime;
  int32 NextSignals;

  /* Initialize audio, return if error. */
  /* This is required in early versions of the Juggler.
     It should not be required later. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  PRT (("Test Objects\n"));
  /* Initialize the Juggler.  Define the classes. */
  InitJuggler ();

  /* Create and use objects of the Class 'SequenceClass'. */
  seq1 = (Sequence *)CreateObject (&SequenceClass);
  CHECKPTR (seq1, "CreateObject");
  seq2 = (Sequence *)CreateObject (&SequenceClass);
  CHECKPTR (seq2, "CreateObject");

  /* Internal data is accessed by using SetObjectInfo or GetObjectInfo
     This allows us to control access to internal data, enforce ranges,
     filter illegal values, etc.
  */
  {
    TagArg Tags[7];

    Tags[0].ta_Tag = JGLR_TAG_START_FUNCTION;
    Tags[0].ta_Arg = (TagData)UserStartFunc;
    Tags[1].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;
    Tags[1].ta_Arg = (TagData)UserInterpFunc;
    Tags[2].ta_Tag = JGLR_TAG_MAX;
    Tags[2].ta_Arg = (TagData)3;
    Tags[3].ta_Tag = JGLR_TAG_MANY;
    Tags[3].ta_Arg = (TagData)3;
    Tags[4].ta_Tag = JGLR_TAG_EVENTS;
    Tags[4].ta_Arg = (TagData)&TestData[0];
    Tags[5].ta_Tag = JGLR_TAG_EVENT_SIZE;
    Tags[5].ta_Arg = (TagData)sizeof (TestEvent);
    Tags[6].ta_Tag = 0;

    DBUG (("SetObjectInfo\n"));
    SetObjectInfo (seq1, Tags);
    SetObjectInfo (seq2, Tags);
  }

  DBUG (("StartObject\n"));
  /* Tell the Juggler to start the sequences at the given times.
          Nothing will happen until you call BumpJuggler() */
  Result = StartObject (seq1, STARTTIME, 5, NULL);
  if (Result)
    {
      PRT (("Start returned 0x%x\n", Result));
    }
  Result = StartObject (seq2, STARTTIME + 2, 5, NULL);
  if (Result)
    {
      PRT (("Start returned 0x%x\n", Result));
    }

  PrintObject (seq1);
  PrintObject (seq2);

  /* Drive Juggler using fake time. */
  NextTime = STARTTIME - 2;
  do
    {
      CurTime = NextTime; /* Pretend that we have waited until NextTime. */
      PRT (("CurTime = %d\n", CurTime));
      Result = BumpJuggler (CurTime, &NextTime, 0, &NextSignals);
    }
  while (Result == 0); /* Will be > 0 when done, < 0 if error */

  StopObject (seq1, NULL);
  StopObject (seq2, NULL);

cleanup:
  DestroyObject ((COBObject *)seq1);
  DestroyObject ((COBObject *)seq2);

  TermJuggler ();
}

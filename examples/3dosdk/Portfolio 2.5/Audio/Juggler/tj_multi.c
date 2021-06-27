
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tj_multi.c,v 1.11 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/tj_multi
|||	tj_multi - Uses the juggler to play a collection.
|||
|||	  Synopsis
|||
|||	    tj_multi
|||
|||	  Description
|||
|||	    This program tests the juggler using non-musical events and software-based
|||	    "timing." It constructs two synthetic sequences based on the TestData data
|||	    structure, and creates a collection based on these two sequences. It then
|||	    "plays" the collection via a simple print function, processed at the time
|||	    of each event in each sequence.
|||
|||	  Associated Files
|||
|||	    tj_multi.c
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

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x) /* 	PRT(x) */

/* Macro to simplify error checking. */
#define CHECKPTR(ptr,name) \
	if (ptr == NULL) \
	{ \
		ERR(("Failure in %s\n", name)); \
		goto cleanup; \
	}


/****************************************************************************/
/* Define our basic event type. Each sequence could be different. */
typedef struct
{
	Time   te_TimeStamp;
	uint32 te_Data;
} TestEvent;

/* This function is called when the Sequence repeats. */
int32 UserRepeatFunc ( Jugglee *Self, Time RepeatTime )
{
	PRT(("==========Repeat function for 0x%x at Time %d\n", Self, RepeatTime));
	return 0;
}

/* This function is called by the Sequence to interpret the current event. */
int32 UserInterpFunc ( Jugglee *Self, TestEvent *te )
{
	PRT(("TestEvent: Self = 0x%x, Time = %d, Data = %d\n",
		 Self, te->te_TimeStamp, te->te_Data));
	return 0;
}

TestEvent TestData[] =
{
	{  5, 567 },
	{  7, 910 },
	{ 12, 400 }
};

#define STARTTIME (2000)
int main()
{
	Sequence *seq1 = NULL;
	Sequence *seq2 = NULL;
	Collection *col = NULL;
	int32 Result;
	Time CurTime, NextTime;
	int32 NextSignals;

/* Initialize audio, return if error. */
/* This is required in early versions of the Juggler.
   It should not be required later. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	PRT(("Test Objects\n"));
	InitJuggler();

/* Instantiate objects from class. */
	seq1 = (Sequence *) CreateObject( &SequenceClass );
	CHECKPTR(seq1,"CreateObject" );

	seq2 = (Sequence *) CreateObject( &SequenceClass );
	CHECKPTR(seq2,"CreateObject" );

	col = (Collection *) CreateObject( &CollectionClass );
	CHECKPTR(col,"CreateObject" );

    {
        TagArg Tags[8];

            /* define TagList */
        Tags[0].ta_Tag = JGLR_TAG_REPEAT_FUNCTION;
        Tags[0].ta_Arg = (TagData) UserRepeatFunc;
        Tags[1].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;
        Tags[1].ta_Arg = (TagData) UserInterpFunc;
        Tags[2].ta_Tag = JGLR_TAG_MAX;
        Tags[2].ta_Arg = (TagData) 3;
        Tags[3].ta_Tag = JGLR_TAG_MANY;
        Tags[3].ta_Arg = (TagData) 3;
        Tags[4].ta_Tag = JGLR_TAG_EVENTS;
        Tags[4].ta_Arg = (TagData) &TestData[0];
        Tags[5].ta_Tag = JGLR_TAG_EVENT_SIZE;
        Tags[5].ta_Arg = (TagData) sizeof(TestEvent);
        Tags[6].ta_Tag = JGLR_TAG_START_DELAY;
        Tags[6].ta_Arg = (TagData) 2;
        Tags[7].ta_Tag =  TAG_END;

            /* Set various parameters in object by using TagList */
        SetObjectInfo(seq1, Tags);
        Tags[6].ta_Tag = JGLR_TAG_START_DELAY;
        Tags[6].ta_Arg = (TagData) 6;
        SetObjectInfo(seq2, Tags);
    }

	PrintObject( seq1 );
	PrintObject( seq2 );

/* Add Sequences to Collection for parallel play */
	Result = col->Class->Add(col, seq1, 3);
	if (Result)
	{
		PRT(("Add returned 0x%x\n", Result));
	}
	Result = col->Class->Add(col, seq2, 4);
	if (Result)
	{
		PRT(("Add returned 0x%x\n", Result));
	}

/* Start Collection which starts both Sequences. */
	Result = StartObject(col, STARTTIME, 3, NULL);
	if (Result)
	{
		PRT(("Start returned 0x%x\n", Result));
	}

/* Drive Juggler using fake time. */
	NextTime = STARTTIME - 2;
	do
	{
		CurTime = NextTime;
		PRT(("CurTime = %d\n", CurTime ));
		Result = BumpJuggler( CurTime, &NextTime, 0, &NextSignals );

	} while ( Result == 0);


	StopObject(seq1, NULL);
	StopObject(seq2, NULL);

cleanup:
	DestroyObject( (COBObject *) seq1 );
	DestroyObject( (COBObject *) seq2 );
	DestroyObject( (COBObject *) col );

	TermJuggler();
}

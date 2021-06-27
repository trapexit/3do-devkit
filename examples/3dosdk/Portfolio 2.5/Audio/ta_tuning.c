
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_tuning.c,v 1.19 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_tuning
|||	ta_tuning - Demonstrates custom tuning a DSP instrument.
|||
|||	  Synopsis
|||
|||	    ta_tuning
|||
|||	  Description
|||
|||	    Demonstrates how to create a tuning table, how to create a tuning, and how
|||	    to apply a tuning to an instrument.
|||
|||	  Associated Files
|||
|||	    ta_tuning.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"
#include "operamath.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define TEST_TEMPLATE
#define SAWINSNAME "sawenv.dsp"
#define OUTPUTNAME "directout.dsp"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr( val ); \
		goto cleanup; \
	}

#define NUMINTERVALS (5)
#define NOTESPEROCTAVE NUMINTERVALS
#define BASENOTE (AF_A440_PITCH)
#define BASEFREQ (440)  /* A440 */
ufrac16 TuningTable[NOTESPEROCTAVE];

/***************************************************************/
int32 PlayPitchNote ( Item Instrument, int32 Note, int32 Velocity )
{
	int32 Result;

	/*
        Notes:
            . Error trapping has been removed for brevity.
            . Use of SleepAudioTicks() is not a real good way to do
              this sort of delay in real code (see Caveats).
    */

	Result = StartInstrumentVA (Instrument,
                                AF_TAG_VELOCITY, Velocity,
                                AF_TAG_PITCH,    Note,
                                TAG_END);
	Result = SleepAudioTicks( 20 );

	ReleaseInstrument( Instrument, NULL);
	Result = SleepAudioTicks( 30 );
	return 0;
}

/***************************************************************/
int main( int argc, char *argv[] )
{
	Item  SawTmp = 0;
	Item  SawIns = 0;
	Item  OutputIns;
	Item  Slendro;
	int32 Result;
	int32 i;
	frac16 BaseFreq;

	PRT(("\n%s V1.0\n", argv[0]));

/* FindMathFolio to get MathBase */
	Result = OpenMathFolio();
	if (Result < 0)
	{
		PrintError(0,"open math folio",0,Result);
		ERR(("Did you run operamath?\n"));
		return (int)Result;
	}

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Use directout instead of mixer. */
	OutputIns = LoadInstrument( OUTPUTNAME,  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load template of Sawtooth instrument */
	SawTmp = LoadInsTemplate( SAWINSNAME, 0);
	CHECKRESULT(SawTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	SawIns = AllocInstrument(SawTmp, 100);
	CHECKRESULT(SawIns,"AllocInstrument");

/* Connect Sawtooth to both sides of Mixer */
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

/*	TraceAudio(TRACE_ENVELOPE); */

/* Play an ascending scale using the default 12 toned equal tempered tuning. */
	PRT(("12 tone equal tempered scale.\n"));
	for (i=50; i<(BASENOTE + (2*NOTESPEROCTAVE)); i++)
	{
		PlayPitchNote( SawIns, i, 80 );
	}

/* Create a custom just intoned pentatonic tuning. */
/* Calculate frequencies as ratios from the base frequency. */
	BaseFreq = Convert32_F16(BASEFREQ);
	TuningTable[0] = BaseFreq;   /* 1:1 */
	TuningTable[1] = MulUF16(BaseFreq, DivUF16(8,7));
	TuningTable[2] = MulUF16(BaseFreq, DivUF16(5,4));
	TuningTable[3] = MulUF16(BaseFreq, DivUF16(3,2));
	TuningTable[4] = MulUF16(BaseFreq, DivUF16(7,4));

/* Create a tuning item that can be used with many instruments. */
	Slendro = CreateTuning( TuningTable, NUMINTERVALS, NOTESPEROCTAVE, BASENOTE );
	CHECKRESULT(Slendro,"CreateTuning");

/* Tell an instrument to use this tuning. */
#ifdef TEST_TEMPLATE
	PRT(("Tune Template\n"));
	Result = TuneInsTemplate( SawTmp, Slendro );
	CHECKRESULT(Result,"TuneInsTemplate");
/* Make a new instrument based on template. */
	FreeInstrument( SawIns );
	SawIns = AllocInstrument(SawTmp, 100);
	CHECKRESULT(SawIns,"AllocInstrument");
#else
	PRT(("Tune Instrument\n"));
	Result = TuneInstrument( SawIns, Slendro );
	CHECKRESULT(Result,"TuneInstrument");
#endif

/* Play the same ascending scale using the custom tuning. */
	PRT(("Custom pentatonic scale.\n"));
	for (i=50; i<(BASENOTE + (2*NOTESPEROCTAVE)); i++)
	{
		PlayPitchNote( SawIns, i, 80 );
	}

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	FreeInstrument( SawIns );
	UnloadInsTemplate( SawTmp );

	UnloadInstrument( OutputIns );
	PRT(("%s all done.\n", argv[0]));
	TraceAudio(0);
	CloseAudioFolio();
	return((int) Result);
}

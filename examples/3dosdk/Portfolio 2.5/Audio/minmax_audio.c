
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: minmax_audio.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/minmax_audio
|||	minmax_audio - Measures the maximum and minimum output from the DSP.
|||
|||	  Synopsis
|||
|||	    minmax_audio
|||
|||	  Description
|||
|||	    Samples the output of the DSP and returns its maximum and minimum output
|||	    values. You can use minmax_audio to check that your program outputs are
|||	    reasonable, non-clipping levels of audio.
|||
|||	    The program creates instances of probes from the instruments minimum.dsp
|||	    and maximum.dsp. The probes are queried with ReadProbe() every five
|||	    seconds, or 5*240 audio ticks.
|||
|||	    In Opera hardware, the DSP implementation is 16-bit; therefore, the output
|||	    for the left and right channels range from 32767 to -32768.
|||
|||	  Caveats
|||
|||	    The loudness of audio is often subjective. Check the loudness of your
|||	    program by first playing a standard audio CD on a development station, and
|||	    adjusting the volume of your sound system to a reasonable level. The 3DO
|||	    program should then produce sounds at a similar volume.
|||
|||	  Associated Files
|||
|||	    minmax_audio.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include "types.h"
#include "mem.h"
#include "debug.h"
#include "operror.h"
#include "stdarg.h"
#include "stdio.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define NUMCHANNELS (2)   /* Stereo */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

/***********************************************************************/
int main(int argc, char *argv[])
{
/* Declare local variables */
	Item SleepCue = 0;
	int32 Result;
	Item TapIns;
	Item LeftMaxIns = 0, LeftMinIns = 0;
	Item LeftMaxProbe = 0, LeftMinProbe = 0;
	int32 LeftMaxVal = 0, LeftMinVal = 0;

	Item RightMaxIns = 0, RightMinIns = 0;
	Item RightMaxProbe = 0, RightMinProbe = 0;
	int32 RightMaxVal = 0, RightMinVal = 0;

	PRT(("%s\n", argv[0] ));


/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Load instrument to tap output.  Zero priority to be at end. */
	TapIns = LoadInstrument( "tapoutput.dsp",  0, 0);
	CHECKRESULT(TapIns,"LoadInstrument");

/* Create a Cue for signalback */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue, "CreateCue");

	LeftMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(LeftMinIns,"LoadInstrument");
	LeftMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(LeftMaxIns,"LoadInstrument");

	LeftMaxProbe = CreateProbe( LeftMaxIns, "Output", NULL );
	CHECKRESULT(LeftMaxProbe,"CreateProbe");
	LeftMinProbe = CreateProbe( LeftMinIns, "Output", NULL );
	CHECKRESULT(LeftMinProbe,"CreateProbe");

	Result = ConnectInstruments( TapIns, "LeftOutput", LeftMaxIns, "InputA" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( LeftMaxIns, "Output", LeftMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( TapIns, "LeftOutput", LeftMinIns, "InputA" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( LeftMinIns, "Output", LeftMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");


	RightMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(RightMinIns,"LoadInstrument");
	RightMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(RightMaxIns,"LoadInstrument");

	RightMaxProbe = CreateProbe( RightMaxIns, "Output", NULL );
	CHECKRESULT(RightMaxProbe,"CreateProbe");
	RightMinProbe = CreateProbe( RightMinIns, "Output", NULL );
	CHECKRESULT(RightMinProbe,"CreateProbe");

	Result = ConnectInstruments( TapIns, "RightOutput", RightMaxIns, "InputA" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( RightMaxIns, "Output", RightMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( TapIns, "RightOutput", RightMinIns, "InputA" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments( RightMinIns, "Output", RightMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");

/* Start capturing sound. */
	Result = StartInstrument( TapIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrument( LeftMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( LeftMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrument( RightMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( RightMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Loop while reading probe. */
	while(1)
	{
/* This is the right way. */
		Result = ReadProbe( LeftMaxProbe, &LeftMaxVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe( LeftMinProbe, &LeftMinVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe(RightMaxProbe, &RightMaxVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe(RightMinProbe, &RightMinVal );
		CHECKRESULT( Result, "ReadProbe" );

		PRT(("\n----\nLeftMin  = %8d, LeftMax  = %8d\n", LeftMinVal, LeftMaxVal ));
		PRT(("RightMin = %8d, RightMax = %8d\n", RightMinVal,RightMaxVal ));
		SleepUntilTime( SleepCue, GetAudioTime() + 5*240 );
	}

cleanup:
	UnloadInstrument( LeftMaxIns );
	UnloadInstrument( LeftMinIns );
	DeleteProbe( LeftMaxProbe );
	DeleteProbe( LeftMinProbe );

	UnloadInstrument( RightMaxIns );
	UnloadInstrument( RightMinIns );
	DeleteProbe( RightMaxProbe );
	DeleteProbe( RightMinProbe );

	DeleteCue( SleepCue );
	UnloadInstrument( TapIns );

	CloseAudioFolio();
	PRT(("All done.\n"));
	return((int) Result);
}

/***************************************************************
**
** ta_dcsq.c - test decompression of an audio sample using
**    DCSQXDMONO - DeCompress SQuare eXact/Delta MONOphonic sample.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
**
**	History:
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote, /dsp, and /aiff removed
** 	930315 		PLB	 	Conforms to new API
***************************************************************/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Define Tags for StartInstrument */
TagArg SamplerTags[] =
	{
		{ AF_TAG_AMPLITUDE, 0},
        { 0, 0 }
    };


/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
/*		PrintfSysErr(Result); */ \
		goto cleanup; \
	}

/* Variables local to this file. */
static	Item 	MixerIns;
static	Item	LeftGainKnob;
static	Item 	RightGainKnob;

/* Local function prototypes. */
static	int32 	SetupMixer( void );

int main(int argc, char *argv[])
{
/* Declare local variables */
	Item SamplerIns;
	Item SampleItem;
	Item Attachment;
	int32 dur;
	int32 Result;
	int32 Rate;

/* Initalize local variables */
	SamplerIns = 0;
	SampleItem = 0;
	Attachment = 0;
	Result = -1;

	PRT(("Usage: %s <samplefile> <seconds>\n", argv[0]));

/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	if (SetupMixer()) return -1;
	
/* Load Sampler instrument */
	SamplerIns = LoadInstrument("dcsqxdmono.dsp", 0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");
	
/* Load digital audio Sample from disk. */
	if (argc > 1)
	{
		SampleItem = LoadSample(argv[1]);  /* from command line? */
	}
	else
	{
		SampleItem = LoadSample("polemama.cp.aiff");
	}
	CHECKRESULT(SampleItem,"LoadSample");
	
	
	if (argc > 2)
	{
		dur=atoi(argv[2]);
	}
	else
	{
		dur = 5;
	}
DBUG(("Duration = %d seconds.\n", dur));

/* Connect Sampler to Mixer */
DBUG(("Connect Instruments, Sampler -> Mixer\n"));
	Result = ConnectInstruments (SamplerIns, "Output", MixerIns, "Input0");
	CHECKRESULT(Result,"ConnectInstruments");
		
/* Attach the sample to the instrument. */
	Attachment = AttachSample(SamplerIns, SampleItem, 0);
	CHECKRESULT(Attachment,"AttachSample");

/* Look at sample information. */
	DebugSample(SampleItem);
	
/* Play one note for the specified time time. */
DBUG(("Play sample for %d seconds.\n", dur));
	SamplerTags[0].ta_Arg = (int32 *) 0x7FFF;
	Result = StartInstrument( SamplerIns, &SamplerTags[0] );
	
	Rate = ConvertF16_32(GetAudioRate());
	Result = SleepAudioTicks( Rate*dur );
	
	ReleaseInstrument( SamplerIns, NULL);
	Result = SleepAudioTicks( Rate );
	
/* The Audio Folio is immune to passing NULL values as Items. */
	Result = DetachSample( Attachment );
	CHECKRESULT(Result,"DetachSample");
	
cleanup:
	UnloadSample( SampleItem );
	UnloadInstrument( SamplerIns );
	
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	
PRT(("%s finished.\n", argv[0]));
	CloseAudioFolio();
	return((int) Result);
}

/*********************************************************************/
int32 SetupMixer( )
{
/* Declare local variables */
	int32 Result;

/* Initalize local variables */
	Result = 0;

/* Initalize global variables */
	MixerIns = -1;
	LeftGainKnob = -1;
	RightGainKnob = -1;

	
	MixerIns = LoadInstrument("mixer4x2.dsp", 0, 0);
	CHECKRESULT(MixerIns,"LoadInstrument");
	
/* Attach the Left and Right gain knobs. */
	LeftGainKnob = GrabKnob( MixerIns, "LeftGain0" );
	CHECKRESULT(LeftGainKnob,"GrabKnob");
	RightGainKnob = GrabKnob( MixerIns, "RightGain0" );
	CHECKRESULT(RightGainKnob,"GrabKnob");
	
/* Set Mixer Levels */
	TweakKnob ( LeftGainKnob, 0x2000 );
	TweakKnob ( RightGainKnob, 0x2000 );
/* Mixer must be started */
	Result = StartInstrument( MixerIns, NULL );
	return Result;
	
cleanup:
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	return Result;
}

/* $Id: ta_attach.c,v 1.9 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Experiment with Sample Attachments - ta_attach.c
** Build two samples via software synthesis.
** Connect them together in a circle.
** Play them in a way that could simulate double buffering.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
**	History:
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote, /dsp, and /aiff removed
***************************************************************/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define NUMCHANNELS (2)   /* Stereo */
#define NUMFRAMES (64*1024)
#define SAMPSIZE (sizeof(int16)*NUMFRAMES*NUMCHANNELS)

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
	

/* Variables local to this file. */
static	Item SamplerIns;

/* Local function prototypes. */
static	int32 BuildSample ( Item *SampPtr, Item *CuePtr, Item *AttPtr, int32 Octave );
	
int main(int argc, char *argv[])
{
/* Declare local variables */
	Item OutputIns;
	Item Samp1, Samp2;
	Item Att1, Att2;
	int32 Signal1, Signal2, SignalHit;
	Item Cue1, Cue2;
	int32 atnum, i;
	int32 Result;
	TagArg Tags[4];

/* Initalize global variables */
	SamplerIns = 0;

/* Initalize local variables */
	OutputIns = 0;
	Samp1 = 0;
	Samp2 = 0;
	Att1 = 0;
	Att2 = 0;
	Signal1 = 0;
	Signal2 = 0;
	Cue1 = 0;
	Cue2 = 0;
	Result = -1;
	

/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}
	
/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("directout.dsp",  0, 0);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load fixed rate stereo sample player instrument */
	SamplerIns = LoadInstrument("fixedstereosample.dsp",  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");
	
/* Connect Sampler Instrument to DirectOut */
	Result = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

/* Build samples via software synthesis. */
	BuildSample( &Samp1, &Cue1, &Att1, 2 );
	BuildSample( &Samp2, &Cue2, &Att2, 0 );
	
/* Get signals from Cues so we can call WaitSignal() */
	Signal1 = GetCueSignal( Cue1 );
	Signal2 = GetCueSignal( Cue2 );
	
/* Link the Attachments in a circle to simulate double buffering. */
	LinkAttachments( Att1, Att2 );
	LinkAttachments( Att2, Att1 );
	
/* Request a signal when the sample ends. */
	Result = MonitorAttachment( Att1, Cue1, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	Result = MonitorAttachment( Att2, Cue2, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	Result = StartInstrument( SamplerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	
/* Let it loop several times. */
	for (i=0; i<7; i++)
	{
		SignalHit = WaitSignal( Signal1|Signal2 );
		atnum = (SignalHit & Signal1) ? 1 : 2 ;
		PRT(("Received signal from attachment %d!\n", atnum));
	}
	
/* Unlink so that current one just finishes. */
	LinkAttachments( Att1, 0 );
	LinkAttachments( Att2, 0 );
	
/* Wait for current one to finish. */
	SignalHit = WaitSignal( Signal1|Signal2 );
	atnum = (SignalHit & Signal1) ? 1 : 2 ;
	PRT(("Received FINAL signal from attachment %d!\n", atnum));
	
	ReleaseInstrument( SamplerIns, NULL);
	
cleanup:
	TraceAudio(0);
	DeleteItem( Cue1 );
	DeleteItem( Cue2 );
	UnloadInstrument( SamplerIns );
	UnloadSample( Samp1 );
	UnloadSample( Samp2 );
	DetachSample( Att1 );
	UnloadInstrument( OutputIns );
/*	TraceAudio(0); */
	CloseAudioFolio();
	PRT(("All done.\n"));
	return((int) Result);
}

/************************************************************************/
/* Fill in a sample with sawtooth waves. */
static	int32 BuildSample ( Item *SampPtr, Item *CuePtr, Item *AttPtr, int32 Octave )
{
	int16 *Data;
	Item Samp, Cue, Att;
	int32 Result, i;
	TagArg Tags[4];
	
/* Make it stereo */
	Tags[0].ta_Tag = AF_TAG_CHANNELS;
	Tags[0].ta_Arg = (int32 *) 2;
	Tags[1].ta_Tag = TAG_END;
	
	Samp = MakeSample ( SAMPSIZE , Tags);
	CHECKRESULT(Samp,"MakeSample");

/* Create a Cue for signalback */	
	Cue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(Cue, "CreateItem Cue");
	
/* Attach the sample to the instrument. */
	Att = AttachSample(SamplerIns, Samp, 0);
	CHECKRESULT(Att,"AttachSample");

/* Get data from sample by sending a Taglist that gets filled in with the
*  current values. */
	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = NULL;
	Tags[1].ta_Tag = TAG_END;
	Result = GetAudioItemInfo(Samp, Tags);
	CHECKRESULT(Result,"GetAudioItemInfo");
	Data = Tags[0].ta_Arg;   /* Points to actual sample data. */
	
/* Fill sample with a sawtooth at a high and low frequency in different channels. */
	for (i=0; i<NUMFRAMES; i++)
	{
		*Data++ = (int16) ((((i << (8+Octave)) & 0xFFFF) * i) / NUMFRAMES);
		*Data++ = (int16) ((((i << (6+Octave)) & 0xFFFF) * (NUMFRAMES - i)) / NUMFRAMES);  /* Lower frequency. */
	}
	
/* Return values to caller. */
	*SampPtr = Samp;
	*CuePtr = Cue;
	*AttPtr = Att;
cleanup:
	return Result;
}

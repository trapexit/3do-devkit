
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_customdelay.c,v 1.15 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_customdelay
|||	ta_customdelay - Demonstrates a delay line attachment.
|||
|||	  Synopsis
|||
|||	    ta_customdelay [\<sample file> [\<delay ticks>]]
|||
|||	  Description
|||
|||	    Demonstrates how to create and use a delay line to get real-time echo
|||	    effects in your program. Loads the specified AIFF file and plays it into a
|||	    delay line. By tweaking the knobs on the output mixer, you can control the
|||	    mix of delay sound versus original sound, and the speed at which the echo
|||	    will die down.
|||
|||	  Arguments
|||
|||	    sample file                  Name of an AIFF file to play.
|||
|||	    delay ticks                  Amount of time, in audio clock ticks, to
|||	                                 hold each note. Defaults to 240 ticks.
|||
|||	  Caveats
|||
|||	    SleepAudioTicks() is an inefficient way to delay a program, and it is used
|||	    in this program's PlayPitchNote() function. Avoid or rewrite the
|||	    PlayPitchNote() function for performance-critical code.
|||
|||	  Associated Files
|||
|||	    ta_customdelay.c
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
#include "stdlib.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

#define INSNAME "sampler.dsp"
/* #define INSNAME "varmono8.dsp" */

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}


int32 PlayPitchNote ( Item Instrument, int32 Note, int32 Velocity, int32 Duration);

#define NUM_MIXER_CHANNELS (4)
typedef struct DelayContext
{
	Item dlc_MixerIns;
	Item dlc_OutputIns;
	Item dlc_DelayIns;
	Item dlc_DelayLine;
	Item dlc_TapIns;
	Item dlc_OriginalSend;
	Item dlc_OriginalMix;
	Item dlc_DelayedSend;
	Item dlc_DelayedMix;
} DelayContext;

int32 InitCustomDelay( DelayContext *dlc, int32 DelaySize, int32 DelayFrames);
int32 TermCustomDelay( DelayContext *dlc );

#define DELAY_FRAMES   (100000)

/********************************************************************/
int main(int argc, char *argv[])
{
	Item SamplerIns = 0;
	Item SampleItem = 0;
	Item Attachment = 0;
	DelayContext DelayCon;
	int32 Duration;
	int32 Result = -1;
	char *SampleName;
	int32 i;

	PRT(("%s <samplefile> <duration>\n", argv[0]));

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

TraceAudio(0);
	Result = InitCustomDelay( &DelayCon, DELAY_FRAMES*sizeof(int16), DELAY_FRAMES - 100);
	CHECKRESULT(Result,"InitCustomDelay");

	DelayCon.dlc_OutputIns = LoadInstrument("directout.dsp", 0, 100);
	CHECKRESULT(DelayCon.dlc_OutputIns,"LoadInstrument");

/* Load Sampler instrument */
	SamplerIns = LoadInstrument(INSNAME, 0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Load digital audio Sample from disk. */
	SampleName = (argc > 1) ? argv[1] : "$samples/PitchedL/SynthBass/BassSynth.C3LM44k.aiff";
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");

	Duration = (argc > 2) ? atoi(argv[2]) : 240 ;
	PRT(("Duration = %d\n", Duration));

/* Look at sample information. */
	/* DebugSample(SampleItem); */

/* Connect Sampler to Mixer */
	Result = ConnectInstruments (SamplerIns, "Output",
		DelayCon.dlc_MixerIns, "Input0");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (DelayCon.dlc_MixerIns, "RightOutput",
		DelayCon.dlc_OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (DelayCon.dlc_MixerIns, "RightOutput",
		DelayCon.dlc_OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

/* Attach the sample to the instrument. */
	Attachment = AttachSample(SamplerIns, SampleItem, 0);
	CHECKRESULT(Attachment,"AttachSample");

/* Instruments must be started */
	Result = StartInstrument( DelayCon.dlc_MixerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
/* Start Delay first to test START_AT.  It is safer to start
** the tap after the delay.
*/
	Result = StartInstrument( DelayCon.dlc_DelayIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( DelayCon.dlc_TapIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( DelayCon.dlc_OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Play several notes using a conveniance routine. */
	PlayPitchNote( SamplerIns, 40, 100, Duration );
	PlayPitchNote( SamplerIns, 44,  64, Duration );
	PlayPitchNote( SamplerIns, 47,  64, Duration );
	PlayPitchNote( SamplerIns, 52,  64, Duration );
	PlayPitchNote( SamplerIns, 59, 100, Duration );
	PlayPitchNote( SamplerIns, 64,  64, Duration*2 );
	for( i=0; i<16; i++)
	{
		PlayPitchNote( SamplerIns, i + 48,  64, Duration>>2 );
	}


/* Let echoes die down. */
	SleepAudioTicks(2000);

cleanup:
	TermCustomDelay( &DelayCon );
	DetachSample( Attachment );
	UnloadInstrument( SamplerIns );
	UnloadSample( SampleItem );

PRT(("All Done---------------------------------\n"));
	CloseAudioFolio();
	return((int) Result);
}

/********************************************************************/
/***** Play a note based on MIDI pitch. *****************************/
/********************************************************************/

int32 PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration)
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
	Result = SleepAudioTicks( Duration>>1 );

	ReleaseInstrument( Instrument, NULL);
	Result = SleepAudioTicks( Duration>>1 );

	return 0;
}

/*********************************************************************/
int32 InitCustomDelay( DelayContext *dlc, int32 DelaySize, int32 DelayFrames)
{
	int32 Result=0;
	Item Att;

/*
** Create a delay line.  This must be allocated by the AudioFolio
** because the memory is written to by hardware.  A delay line
** is just a sample with a special write permission.
** 1=channel, TR loop
*/
#define NUM_CHANNELS (1)
#define IF_LOOP      (TRUE)
	dlc->dlc_DelayLine = CreateDelayLine( DelaySize, NUM_CHANNELS, IF_LOOP );
	CHECKRESULT(dlc->dlc_DelayLine,"CreateDelayLine");

/*
** Load the basic delay instrument which just writes data to
** an output DMA channel of the DSP.
*/
	dlc->dlc_DelayIns = LoadInstrument("delaymono.dsp", 0, 100);
	CHECKRESULT(dlc->dlc_DelayIns,"LoadInstrument");

/* Attach the delay line to the delay instrument output. */
	Att = AttachSample( dlc->dlc_DelayIns, dlc->dlc_DelayLine, "OutFIFO" );
	CHECKRESULT(Att,"AttachDelay");
	Result = SetAudioItemInfoVA( Att,
                                 AF_TAG_START_AT, DelayFrames,
                                 TAG_END );
	CHECKRESULT(Result,"SetAudioItemInfo: START_AT");

/*
** Load an instrument to read the output of the delay.
*/
	dlc->dlc_TapIns = LoadInstrument("fixedmonosample.dsp", 0, 100);
	CHECKRESULT(dlc->dlc_TapIns,"LoadInstrument");

/* Attach the delay line to the delay tap. */
	Att = AttachSample( dlc->dlc_TapIns, dlc->dlc_DelayLine, "InFIFO" );
	CHECKRESULT(Att,"AttachSample");
/*
** Load a submixer that we can use to mix delayed and original signal.
** We will use the left side to mix for the delay, and the right side
** for the output from the circuit.
*/
	dlc->dlc_MixerIns = LoadInstrument("submixer4x2.dsp", 0, 100);
	CHECKRESULT(dlc->dlc_MixerIns,"LoadInstrument");

/* Attach the knobs. */
	dlc->dlc_OriginalSend = GrabKnob( dlc->dlc_MixerIns, "LeftGain0" );
	CHECKRESULT(dlc->dlc_OriginalSend,"GrabKnob");
	dlc->dlc_OriginalMix = GrabKnob( dlc->dlc_MixerIns, "RightGain0" );
	CHECKRESULT(dlc->dlc_OriginalMix,"GrabKnob");
	dlc->dlc_DelayedSend = GrabKnob( dlc->dlc_MixerIns, "LeftGain1" );
	CHECKRESULT(dlc->dlc_DelayedSend,"GrabKnob");
	dlc->dlc_DelayedMix = GrabKnob( dlc->dlc_MixerIns, "RightGain1" );
	CHECKRESULT(dlc->dlc_DelayedMix,"GrabKnob");

/* Connect the output of Tap0 to channel 1 of the mixer. */
	Result = ConnectInstruments (dlc->dlc_TapIns, "Output",
		dlc->dlc_MixerIns, "Input1");
	CHECKRESULT(Result,"ConnectInstruments");

/* Connect the left output of the mixer to the delay. */
	Result = ConnectInstruments (dlc->dlc_MixerIns, "LeftOutput",
		dlc->dlc_DelayIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");

/* Mix for the Delay Line connected to Left channel. */
	TweakKnob( dlc->dlc_OriginalSend, 0x3000 );
	TweakKnob( dlc->dlc_DelayedSend, 0x4000 );

/* Mix for the Output instrument connected to Right channel. */
	TweakKnob( dlc->dlc_OriginalMix, 0x3fff );
	TweakKnob( dlc->dlc_DelayedMix, 0x4000 );

	return Result;

cleanup:
	TermCustomDelay( dlc );
	return Result;
}

int32 TermCustomDelay( DelayContext *dlc )
{
	UnloadInstrument( dlc->dlc_MixerIns );
	UnloadInstrument( dlc->dlc_OutputIns );
	UnloadInstrument( dlc->dlc_DelayIns );
	UnloadInstrument( dlc->dlc_TapIns );
	UnloadSample( dlc->dlc_DelayLine );
	ReleaseKnob( dlc->dlc_OriginalSend );
	ReleaseKnob( dlc->dlc_OriginalMix );
	ReleaseKnob( dlc->dlc_DelayedSend );
	ReleaseKnob( dlc->dlc_DelayedMix );
	return 0;
}















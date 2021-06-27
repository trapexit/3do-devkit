/****************************************************************
**
** Audio Folio all in one test.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/
/*
** 930126 PLB Change to use .dsp files instead of .ofx
** 930315 PLB Conforms to new API
*/

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"
#include "event.h"

/* Include this when using the Audio Folio */
#include "audio.h"
#include "audiodemo.h"

#define MAXVOICES 12
#define NUMVOICES 10
#define NUMER 4
#define DENOM 3
#define STARTFREQ 0x2000
#define NUMSECS 1
#define SAMPPRIORITY 50
#define MIXPRIORITY 100
#define TOGETHER
#define AMPLITUDEINC  0x1000
#define MAXAMPLITUDE  0x7800
#define FREQINC  0x1000
#define MINFREQ  0x0800
#define MAXFREQ  0xF800

#if 0
#define INSNAME "varmono16.dsp"
#else
#define INSNAME "sampler.dsp"
#endif

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );
int32 TestSaw (void );
int32 TestPoly( void );
int32 TestSteady( void );

int32 SleepSeconds( int32 Secs );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}
	

Item MixerTmp = -1;
Item MixerIns = -1;
Item LeftGains[MAXVOICES];
Item RightGains[MAXVOICES];
int32 SetupMixer( void );
int32 TermMixer( void );
int32 gNumVoices;

static char *ChannelNames[] =
{
	"Input0",
	"Input1",
	"Input2",
	"Input3",
	"Input4",
	"Input5",
	"Input6",
	"Input7",
	"Input8",
	"Input9",
	"Input10",
	"Input11"
};

int main(int argc, char *argv[])
{
	int32 Result;
	uint32 butn;
	int32 doit;
	Item NewDir;
	
	PRT(("Usage: %s\n", argv[0]));
	
	gNumVoices = (argc > 1) ? atoi( argv[1] ) : NUMVOICES ;
	if(gNumVoices > MAXVOICES) gNumVoices = MAXVOICES;
	
	
/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	Result = InitJoypad();
	CHECKRESULT(Result, "InitJoypad");
	
/* Allow user to select test. */
	doit = TRUE;
	do
	{
		PRT(("---------------------------------------------------\n"));
		PRT(("Use Joypad to select audio test...\n"));
		PRT(("   A = Sawtooth on left and right channel.\n"));
		PRT(("   B = %d voice polyphony.\n", gNumVoices));
		PRT(("   C = Steady sinewave.\n"));
		PRT(("   START = quit.\n"));
		PRT(("---------------------------------------------------\n"));
	
		WaitJoypad(-1, &butn);
		switch (butn)
		{
		case ControlA:
			Result = TestSaw();
			CHECKRESULT(Result, "TestSaw");
			break;
			
		case ControlB:
			Result = TestPoly();
			CHECKRESULT(Result, "TestPoly");
			break;
			
		case ControlC:
			Result = TestSteady();
			CHECKRESULT(Result, "TestSteady");
			break;
			
		case ControlStart:
			doit = FALSE;
			break;
		}
	} while (doit);
	
	
cleanup:
	Result = TermJoypad();
	CHECKRESULT(Result, "TermJoypad");
	CloseAudioFolio();
	PRT(("Audio Test Complete\n"));
	return (Result != 0);
}

/****************************************************************************/
int32 TestSaw (void )
{
	Item OutputIns = 0;
	Item SawIns = 0;
	Item FreqKnob = 0;
	Item LoudKnob = 0;
	int32 Result, VoiceID;
	uint32 butn;
	
/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("directout.dsp",  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	
/* Load Sawtooth instrument */
	SawIns = LoadInstrument("sawtooth.dsp",  0, 100);
	CHECKRESULT(SawIns,"LoadInstrument");
	
/* Attach the Frequency knob. */
	FreqKnob = GrabKnob( SawIns, "Frequency" );
	CHECKRESULT(FreqKnob,"GrabKnob");
/* Attach the Amplitude knob. */
	LoudKnob = GrabKnob( SawIns, "Amplitude" );
	CHECKRESULT(LoudKnob,"GrabKnob");
	
	TweakKnob(LoudKnob, 0x2000);
	
/* Connect Sampler to DirectOut */
	VoiceID = StartInstrument(SawIns, NULL);
	StartInstrument( OutputIns, NULL );
	
	TweakRawKnob(FreqKnob, 0x200);
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	PRT(("Left Channel\n"));
	PRT(("Hit joypad to continue.\n"));
	WaitJoypad(-1, &butn);
	
/*	SleepSeconds(4); */
	Result = DisconnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	
	TweakRawKnob(FreqKnob, 0x600);
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");
	PRT(("Right Channel\n"));
	PRT(("Hit joypad to continue.\n"));
	WaitJoypad(-1, &butn);
/*	SleepSeconds(4); */
	Result = DisconnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	TweakKnob(LoudKnob, 0);
	
	StopInstrument(SawIns, NULL);
	
cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	ReleaseKnob( FreqKnob);
	UnloadInstrument( SawIns );
	UnloadInstrument( OutputIns );
	PRT(("Sawtooth Test Complete.\n"));
	return Result;
}

/****************************************************************************/
int32 TestPoly( void )
{
	Item SamplerTmp;
	Item SamplerIns[MAXVOICES];
	Item SampleItem = 0;
	int32 i, Result = -1, result;
	int32 Freq, VoiceID;
	uint32 butn;
	TagArg SamplerTags[3];
	
	
	SamplerTags[0].ta_Tag = AF_TAG_AMPLITUDE;
	SamplerTags[0].ta_Arg = (void *) 0x7000;
	SamplerTags[1].ta_Tag = AF_TAG_RATE;
	SamplerTags[1].ta_Arg = (void *) 0;
	SamplerTags[2].ta_Tag = TAG_END;    

	PRT(("Play %d channels of sine wave in ascending frequency.\n", gNumVoices));
	
	PRT(("Please wait a few seconds for samples and instruments to load.\n"));
TraceAudio(0);
	Result = SetupMixer();
	CHECKRESULT(Result, "SetupMixer");
	
/* Set each channel to be panned left to right. */
	for (i=0; i<gNumVoices; i++)
	{
		TweakKnob ( LeftGains[i], ((MAXAMPLITUDE/gNumVoices)*(i))/gNumVoices );
		TweakKnob ( RightGains[i], ((MAXAMPLITUDE/gNumVoices)*(gNumVoices-i))/gNumVoices );
	}

/* Load Sampler instrument definition/template */
	PRT(("   use %s\n", INSNAME));
	SamplerTmp = LoadInsTemplate(INSNAME, 0);
	CHECKRESULT(SamplerTmp,"LoadInsTemplate");
		
/* Make Sampler instruments based on template. */
	for (i=0; i<gNumVoices; i++)
	{
		SamplerIns[i] = AllocInstrument(SamplerTmp, SAMPPRIORITY);
		CHECKRESULT(SamplerIns[i],"AllocInstrument");
	}
	
/* Load digital audio Sample from disk. */
	SampleItem = LoadSample("sinewave.aiff");
	CHECKRESULT(SampleItem,"LoadSample");
TraceAudio(0);

/* Look at sample information. */
/*	DebugSample(SampleItem); */

/* Attach the sample to the instrument. */
	for (i=0; i<gNumVoices; i++)
	{
		Result = AttachSample( SamplerIns[i], SampleItem, 0 );
		CHECKRESULT(Result,"AttachSample");
PRT(("Connect %d to %s\n", i, ChannelNames[i]));
		Result = ConnectInstruments (SamplerIns[i], "Output", MixerIns,
			ChannelNames[i]);
		CHECKRESULT(Result,"ConnectInstruments");
	}

	Freq = STARTFREQ;
	
PRT(("Freq = "));
	for (i=0; i<gNumVoices; i++)
	{
		PRT((" 0x%x", Freq));
		SamplerTags[1].ta_Arg = (void *) Freq; /* Freq */
		Freq = (Freq * NUMER) / DENOM;
		VoiceID = StartInstrument( SamplerIns[i], SamplerTags );
		result = SleepSeconds( NUMSECS );
#ifdef TOGETHER
	}
	PRT(("\nTicks = %d\n", DSPGetTicks()));
	
	PRT(("Hit any button to continue.\n"));
	WaitJoypad(-1, &butn);
	
	for (i=0; i<gNumVoices; i++)
	{
#endif
		ReleaseInstrument( SamplerIns[i],  NULL );
#ifdef TOGETHER
		result = SleepSeconds( NUMSECS );
#endif
	}
	
cleanup:
	
/* The Audio Folio is immune to passing NULL values as Items. */
	for(i=0; i<gNumVoices; i++)
	{
		FreeInstrument( SamplerIns[i] );
	}
	UnloadInsTemplate( SamplerTmp );
	UnloadSample( SampleItem );
	TermMixer();
	PRT(("Polyphony Test Complete.\n"));
	return((int) Result);
}

static char *LeftNames[] = 
{
	"LeftGain0",
	"LeftGain1",
	"LeftGain2",
	"LeftGain3",
	"LeftGain4",
	"LeftGain5",
	"LeftGain6",
	"LeftGain7",
	"LeftGain8",
	"LeftGain9",
	"LeftGain10",
	"LeftGain11"
};

static char *RightNames[] = 
{
	"RightGain0",
	"RightGain1",
	"RightGain2",
	"RightGain3",
	"RightGain4",
	"RightGain5",
	"RightGain6",
	"RightGain7",
	"RightGain8",
	"RightGain9",
	"RightGain10",
	"RightGain11"
};

/*********************************************************************/
int32 SetupMixer( )
{
	int32 Result=0,  i;
	
	MixerTmp = LoadInsTemplate("mixer12x2.dsp",  0);
	CHECKRESULT(MixerTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	MixerIns = AllocInstrument(MixerTmp, MIXPRIORITY);
	CHECKRESULT(MixerIns,"AllocInstrument");
	
/* Attach the Left and Right gain knobs. */
	for (i=0; i<MAXVOICES; i++)
	{
		LeftGains[i] = GrabKnob( MixerIns, LeftNames[i] );
		CHECKRESULT(LeftGains[i],"GrabKnob");
		TweakKnob ( LeftGains[i], 0 );
	
		RightGains[i] = GrabKnob( MixerIns, RightNames[i] );
		CHECKRESULT(RightGains[i],"GrabKnob");
		TweakKnob ( RightGains[i], 0 );
	}

	
/* Mixer must be started */
	Result = StartInstrument( MixerIns, NULL );
	return Result;
	
cleanup:
	TermMixer();
	return Result;
}

/*********************************************************************/
int32 TermMixer()
{
	int32 i;
	for (i=0; i<MAXVOICES; i++)
	{
		ReleaseKnob(LeftGains[i]);
		ReleaseKnob(RightGains[i]);
	}
	FreeInstrument( MixerIns );
	UnloadInsTemplate( MixerTmp );
	return 0;
}

/*********************************************************************/
int32 SleepSeconds( int32 Secs )
{
	frac16 Rate;
	Rate = GetAudioRate() >> 16;
	return SleepAudioTicks ( Secs * Rate );
}

/****************************************************************************/
/*********** Steady Sine Wave until joy pad hit. ****************************/
/****************************************************************************/
int32 TestSteady( void )
{
	Item SamplerIns;
	Item SampleItem;
	Item FreqKnob;
	int32 Result = -1;
	int32 Freq, VoiceID;
	int32 Pan = 1, doit;
	int32 Amplitude = MAXAMPLITUDE/2;
	int32 i;
	int32 OldMixerChan;
	
	uint32 butn;
	TagArg SamplerTags[] =
	{
		{ AF_TAG_AMPLITUDE, (int32 *) 0x7000},
		{ AF_TAG_RATE, 0},
        { 0, 0 }
    };
    PRT(("Steady sine tone.\n"));
	PRT(("Please wait a few seconds for samples and instruments to load.\n"));
		
	Result = SetupMixer();
	CHECKRESULT(Result, "SetupMixer");

	for (i=0; i<gNumVoices; i++)
	{
		TweakKnob ( LeftGains[i], 0 );
		TweakKnob ( RightGains[i], 0 );
	}

/* Load Sampler instrument definition/template */
	SamplerIns = LoadInstrument(INSNAME,  0, SAMPPRIORITY);
	CHECKRESULT(SamplerIns,"LoadInstrument");
	
/* Load digital audio Sample from disk. */
	SampleItem = LoadSample("sinewave.aiff");
	CHECKRESULT(SampleItem,"LoadSample");
	
/* Look at sample information. */
/*	DebugSample(SampleItem); */

	Result = AttachSample(SamplerIns, SampleItem, 0);
	CHECKRESULT(Result,"AttachSample");
	
/* Connect Sampler to Mixer */
	Result = ConnectInstruments (SamplerIns, "Output",
		MixerIns, ChannelNames[0]);
	CHECKRESULT(Result,"ConnectInstruments");
	OldMixerChan = 0;

/* Set Mixer Levels */
	Pan = 1;
	TweakKnob(LeftGains[0], Amplitude);
	TweakKnob(RightGains[0], Amplitude);
	
	FreqKnob = GrabKnob(SamplerIns, "Frequency");
	CHECKRESULT(FreqKnob,"GrabKnob");
	
	Freq = 0x4800;
	SamplerTags[1].ta_Arg = (void *) Freq; /* Freq */
	VoiceID = StartInstrument( SamplerIns, SamplerTags );

/* Process JoyPad */
	PRT(("Use Joypad:\n"));
	PRT(("  JOYLEFT, JOYRIGHT - control left/right panning.\n"));
	PRT(("  JOYUP, JOYDOWN - control loudness.\n"));
	PRT(("  FIREA - raises frequency.\n"));
	PRT(("  FIREB - lowers frequency.\n"));

	doit = TRUE;
	while (doit)
	{
		WaitJoypad(-1, &butn);
		
		if (butn & ControlStart)
		{
			doit = FALSE;
		}
		
		if (butn & ControlA)
		{
			Freq += FREQINC;
			if (Freq > MAXFREQ) Freq = MAXFREQ;
			PRT(("Freq = 0x%x\n", Freq));
			TweakKnob(FreqKnob, Freq);
		}
		
		if (butn & ControlB)
		{
			Freq -= FREQINC;
			if (Freq < MINFREQ) Freq = MINFREQ;
			PRT(("Freq = 0x%x\n", Freq));
			TweakKnob(FreqKnob, Freq);
		}
		
		if (butn & ControlC)
		{
			Result = DisconnectInstruments (SamplerIns, "Output",
			MixerIns, ChannelNames[OldMixerChan]);
			CHECKRESULT(Result,"ConnectInstruments");
			OldMixerChan = (OldMixerChan+1) & 0x7;
			Result = ConnectInstruments (SamplerIns, "Output",
			MixerIns, ChannelNames[OldMixerChan]);
			CHECKRESULT(Result,"ConnectInstruments");
			PRT(("MixerChan = %d\n", OldMixerChan));
		}
		
		if (butn & ControlUp)
		{
			Amplitude += AMPLITUDEINC;
			if (Amplitude > MAXDSPAMPLITUDE) Amplitude = MAXDSPAMPLITUDE;
			PRT(("Amplitude = 0x%x\n", Amplitude));
		}
		if (butn & ControlDown)
		{
			Amplitude -= AMPLITUDEINC;
			if (Amplitude < 0) Amplitude = 0;
			PRT(("Amplitude = 0x%x\n", Amplitude));
		}
		
		if (butn & ControlLeft)
		{
			if (Pan > 0)
			{
				Pan -= 1;
			}
		}
				
		if (butn & ControlRight)
		{
			if (Pan < 2)
			{
				Pan += 1;
			}
		}
		
		if ((butn & (ControlRight|ControlLeft|ControlUp|ControlDown)) && !(butn & JOYFIREB))
		{
			switch(Pan)
			{
			case 0:
				TweakKnob(LeftGains[OldMixerChan], Amplitude);
				TweakKnob(RightGains[OldMixerChan], 0);
				PRT(("Left only.\n"));
				break;
			case 1:
				TweakKnob(LeftGains[OldMixerChan], Amplitude);
				TweakKnob(RightGains[OldMixerChan], Amplitude);
				PRT(("Left and Right.\n"));
				break;
			case 2:
				TweakKnob(LeftGains[OldMixerChan], 0);
				TweakKnob(RightGains[OldMixerChan], Amplitude);
				PRT(("Right only.\n"));
				break;
			default:
				ERR(("Illegal Pan = 0x%x\n", Pan));
				break;
			}
		}
	}

	ReleaseInstrument( SamplerIns,  NULL );
	
cleanup:
	
	UnloadInstrument( SamplerIns );
	UnloadSample( SampleItem );
	TermMixer();
	PRT(("Steady Sine Test Complete.\n"));
	return((int) Result);
}



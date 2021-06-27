
/*******************************************************************
**
** Horizontal faders to control sound.
**
** By: Phil Burk
** 
**
**	History:
**	6/14/93		rdg		added display for instrument current and max tick count
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote, /dsp, and /aiff removed
**	 930602 	PLB 	Round up when calculating fdr_increment.
********************************************************************/


#include "types.h"

#include "kernel.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "operamath.h"
#include "filefunctions.h"
#include "graphics.h"
#include "audio.h"
#include "stdio.h"
#include "event.h"

#include "audiodemo.h"

#define DEMOMODE

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}
	
#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto cleanup; \
	}

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MAX_FADERS (12)
Fader MyFaders[MAX_FADERS];
FaderBlock MyFaderBlock;

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer( void );

#define MAX_KNOBS MAX_FADERS
Item TestKnob[MAX_KNOBS];
char *Name[MAX_KNOBS];
	
Item gTestIns = 0;
	
/********************************************************************/
int32 RunFaders ( char *InsName )
{
	int32 doit;
	int32 Result;
	int32 i;
	uint32 joy;
	int32 IfNoteOn;
	int32 EnableA;
	int32 IfControlBOn;
	int32 OverheadTicks;
	int32 CurrentTicks;
	int32 MaximumTicks;

	doit = TRUE;
	IfNoteOn = TRUE;
	EnableA = TRUE;
	IfControlBOn = FALSE;
	
	Result = InitJoypad();
	CHECKRESULT( Result, InitJoypad );
	
	/* figure out how many ticks are already being used */
	OverheadTicks = 0;
	for(i=0; i<20; i++)
	{
		SleepAudioTicks(1);
		OverheadTicks += DSPGetTicks();
	}

	OverheadTicks /= 20;
	
/* Start the instrument we want to test */
	Result = StartInstrument( gTestIns, NULL );
	
	MaximumTicks = 0;
	while (doit)
	{
		Result = ReadJoypad( &joy );
		CHECKRESULT(Result,"ReadJoypad");
		if (joy & ControlStart)
		{
			doit = FALSE;
		}
		DriveFaders ( &MyFaderBlock, joy );

/* Toggle with ControlA */
		if (joy & ControlA)
		{
			if(EnableA)
			{
			
				if(IfNoteOn)
				{
					Result = ReleaseInstrument( gTestIns, NULL );
					IfNoteOn = FALSE;
				}
				else
				{
				
					Result = StartInstrument( gTestIns, NULL );
					IfNoteOn = TRUE;
				}
				CHECKRESULT(Result, "Start/ReleaseInstrument");
				EnableA = FALSE;
			}
		}
		else
		{
			EnableA = TRUE;
		}
		
/* Play with ControlB */
		if (joy & ControlB)
		{
			if(!IfControlBOn)
			{
				Result = StartInstrument( gTestIns, NULL );
				IfControlBOn = TRUE;
				IfNoteOn = TRUE;
			}
		}
		else
		{
			if(IfControlBOn)
			{
				Result = ReleaseInstrument( gTestIns, NULL );
				IfControlBOn = FALSE;
				IfNoteOn = FALSE;
			}
		}

		CHECKRESULT(Result, "Start/ReleaseInstrument");
		
		MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 5 );
		DrawText8( &GCon[0], CURBITMAPITEM, "3DO Instrument Tester" );
		MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 20 );
		DrawText8( &GCon[0], CURBITMAPITEM, InsName );
		MoveTo( &GCon[0], 220, TOP_VISIBLE_EDGE + 5 );

		DrawText8( &GCon[0], CURBITMAPITEM, "CT:" );

		CurrentTicks = DSPGetTicks() - OverheadTicks;

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 5 );
		DrawNumber( CurrentTicks );

		MoveTo( &GCon[0], 220, TOP_VISIBLE_EDGE + 20 );
		DrawText8( &GCon[0], CURBITMAPITEM, "MT:" );

		if(CurrentTicks > MaximumTicks)
			MaximumTicks = CurrentTicks;

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 20 );
		DrawNumber(MaximumTicks);
		
		Result = SwitchScreens();
		CHECKRESULT(Result,"SwitchScreens");
	}
cleanup:
	return Result;
}

/********************************************************************/
int32 DrawFaderScreen( void )
{
	int32 i;
	
	ClearScreen();
	for (i=0; i<MyFaderBlock.fdbl_NumFaders; i++)
	{
		DrawFader ( &MyFaders[i] );
	}
	return 0;
}

/********************************************************************/
int32 InitFaders( int32 NumFaders, int32 (*CustomFunc)() )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	int32 Result, i;
	Fader *fdr;
	
	InitFaderBlock ( &MyFaderBlock, &MyFaders[0], NumFaders, CustomFunc );

/* Change YMIN in faders.h %Q */
	for (i=0; i<NumFaders; i++)
	{
		fdr = &MyFaders[i];
		fdr->fdr_YMin = FADER_YMIN + ( i * FADER_SPACING ) + 15;
		fdr->fdr_YMax = fdr->fdr_YMin + FADER_HEIGHT;
	}
	
	DrawFaderScreen();
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");

	DrawFaderScreen();
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");
	
	Result = DisplayScreen( ScreenItems[0], 0 );
	if ( Result < 0 )
	{
		printf( "DisplayScreen() failed, error=%d\n", Result );
		goto cleanup;
	}

cleanup:
	return Result;
}

/********************************************************************/
int32 TermDemo( void )
{
	return TermGraphics();
}

/********************************************************************/

int32 CustomFaderFunc( int32 kn, int32 FaderValue )
{
	DBUG(("%8d => %s\n", FaderValue, Name[kn]));
	return TweakRawKnob(TestKnob[kn], FaderValue);
}

/********************************************************************/
int32 SetupKnobFader( Item Knob, Fader *fdr )
{
	int32 Result;
	
	TagArg Tags[4];

/* Get attributes of knob. */
	Tags[0].ta_Tag = AF_TAG_MIN;
	Tags[1].ta_Tag = AF_TAG_DEFAULT;
	Tags[2].ta_Tag = AF_TAG_MAX;
	Tags[3].ta_Tag = TAG_END;
	
	Result = GetAudioItemInfo ( Knob, Tags);
	CHECKRESULT(Result, "GetKnobInfo");
/* Now Pull Values from TagList */

	fdr->fdr_VMin  = (int32) Tags[0].ta_Arg;
PRT(("fdr->fdr_VMin = %d\n", fdr->fdr_VMin));
	fdr->fdr_Value = (int32) Tags[1].ta_Arg;
	fdr->fdr_VMax  = (int32) Tags[2].ta_Arg;
	fdr->fdr_Increment  = ((fdr->fdr_VMax - fdr->fdr_VMin) + 99) / 100;
	if (fdr->fdr_Increment < 1 ) fdr->fdr_Increment = 1;
	TweakRawKnob( Knob, fdr->fdr_Value );
	
cleanup:
	return Result;
}

int main(int argc, char *argv[])
{
	Item NoiseIns = 0;
	char *InsName;
	int32 i;
	int32 NumKnobs;
	int32 Result;
	int32 useNoise = FALSE;
	Item Att, SampleItem;

	NumKnobs = 0;
	SampleItem = 0;
	Att = 0;

	Result = InitGraphics( 1 );
	CHECKRESULT(Result, "InitGraphics");

	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	if (SetupMixer()) return -1;
	
/* Load instrument definition/template */
	if (argc < 2)
	{
		PRT(("You forgot to specify an instrument!\n"));
		PRT(("Usage:   ta_faders <filename.dsp>\n"));
		goto cleanup;
	}
	InsName = argv[1];
	gTestIns = LoadInstrument( InsName, 0, 100);
	CHECKRESULT(gTestIns,"LoadInstrument");
	
	
	if( argc > 2)
	{
		SampleItem = LoadSample( argv[2] );
		CHECKRESULT(SampleItem,"LoadSample");
PRT(("Try to attach sample!"));
		Att = AttachSample( gTestIns, SampleItem, NULL );
		CHECKRESULT(Att,"AttachSample");
	}
	
	NoiseIns = LoadInstrument("noise.dsp", 0, 100);
	CHECKRESULT(NoiseIns,"LoadInstrument noise");
	
	PRT(("Connect Instruments, Noise -> Input\n"));
	Result = ConnectInstruments (NoiseIns, "Output", gTestIns, "Input");
	if (Result < 0)
	{
		PRT(("%s has no Input\n", argv[1]));
	}
	else
	{
		useNoise = TRUE;
	}
	
	
/* Connect to Mixer */
	PRT(("Connect Instruments, test -> Mixer\n"));
	Result = ConnectInstruments (gTestIns, "Output", MixerIns, "Input0");
/*	CHECKRESULT(Result,"ConnectInstruments"); */
	
	NumKnobs = GetNumKnobs(gTestIns);
	PRT(("%d knobs.\n", NumKnobs));
	if( NumKnobs > MAX_FADERS )
	{
		PRT(("Too many knobs for this program to handle = %d\n", NumKnobs));
		NumKnobs = MAX_FADERS;
	}
	
	Result = InitFaders( NumKnobs , CustomFaderFunc);
	if ( Result < 0 )
	{
		printf( "InitFaders() failed, error=%d\n", Result );
		goto cleanup;
	}
	
/* Attach all available knobs */
	for (i=0; i<NumKnobs; i++)
	{
		Name[i] = GetKnobName( gTestIns, i);
		if (Name[i] != NULL)
		{
/* Attach knob so we can tweak what's there. */
			TestKnob[i] = GrabKnob (gTestIns, Name[i] );
			CHECKRESULT(TestKnob[i],"GrabKnob");
			SetupKnobFader( TestKnob[i], &MyFaders[i] );
			MyFaders[i].fdr_Name = Name[i];
#ifdef DEMOMODE
			if (strcmp(Name[i], "Amplitude") == 0)
			{
				TweakRawKnob( TestKnob[i], 0 );
				MyFaders[i].fdr_Value = 0;
			}
#endif
		}
	}
	
/* If the testIns has an input then start the noise */
	if(useNoise)
	  Result = StartInstrument(NoiseIns, NULL);
	
	DrawFaderScreen();
	
	RunFaders( InsName );
	
	StopInstrument(gTestIns, NULL);
	StopInstrument(NoiseIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	for (i=0; i<NumKnobs; i++)
	{
		ReleaseKnob(TestKnob[i]);
	}
	UnloadInstrument( gTestIns );
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	UnloadInstrument( NoiseIns );
	if(SampleItem) UnloadSample(SampleItem);
	
	CloseAudioFolio();
	TermDemo();
	PRT(("Finished %s\n", argv[0] ));
	
	return( (int)Result );
}

/*********************************************************************/
int32 SetupMixer( )
{
	int32 Result=0, VoiceID;
	
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
	VoiceID = StartInstrument( MixerIns, NULL );
	return Result;
	
cleanup:
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	return Result;
}


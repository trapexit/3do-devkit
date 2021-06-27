
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: effectshandler.c,v 1.18 1995/01/08 04:20:45 ceckhaus Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/effectshandler
|||	effectshandler - Utility code for mixers and sound effects.
|||
|||	  Description
|||
|||	    Utility code for basic management of mixers and sound effects.
|||
|||	    Typical usage scenario:
|||
|||	      During program initialization:
|||	        Open the audio folio.
|||	        Call ehNewMixerInfo() to load a mixer and initialize a
|||	          mixerInfo struct.
|||	        Call ehLoadSoundEffect() to load a sample as a sound effect and
|||	          initialize a sampleInfo struct.
|||	        Optionally call ehSetChannelLevels() for each sound effect.
|||
|||	      As needed during execution:
|||	        Call ehSetChannelLevels() for a given sound effect to adjust
|||	          its player's output levels.
|||	        Issue calls which control the sound effect's player instrument
|||	          ( the SampleInfo's si_Player struct member ).
|||
|||	      During program termination:
|||	        Call ehDisposeSampleInfo() for each sound effect.
|||	        Call ehDisposeMixerInfo() for the mixer.
|||	        Close the audio folio.
|||
|||	    Depending on program requirements, you may want to load one or more
|||	    sound effects and assemble their player apparati but not immediately
|||	    connect them to a mixer.  This is accomplished by calling
|||	    ehNewSampleInfo() and ehSetupSamplePlayer().  When you want to
|||	    connect the player to the mixer, call ehConnectSamplePlayer().
|||
|||	  Caveats
|||
|||	    This does not purport to be a complete sound management facility.
|||	    It is intended to handle mono sound effects, although if a stereo
|||	    sample is encountered, it connects the left output to the mixer rather
|||	    than returning an error (a diagnostic message is printed to the debug
|||	    terminal, however).
|||
|||	  Associated Files
|||
|||	    effectshandler.c effectshandler.h
|||
|||	  Location
|||
|||	    Examples/ExampleLib
|||
**/

#include "types.h"
#include "mem.h"
#include "string.h"
#include "audio.h"
#include "score.h"
#include "debug3do.h"
#include "effectshandler.h"

/* Set to one to enable DebugSample() after LoadSample. */
#define INTENSE_DEBUG	0

/*** Effects-handling definitions ***/

char *gChannelName[] =
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

/*** Function prototypes for helper functions ***/

Item	_ehSetupMixer( const char *mixerName );
void	_ehDisposeSample( Item aSample, Item aSamplePlayer );
Err		_ehAcquireGainKnob( Item mixer, int32 channel, TOutput leftRight, Item *knob );
void _ehGetGainKnobName( char *knobName, TOutput leftRight, int32 channel );
void _ehReleaseKnob( Item *knob );
void _ehSplitVolume( int32 volume, int32 balance, int32 *leftLevel, int32 *rightLevel );


/*** Effects-handling helper functions: ***/

void _ehGetGainKnobName( char *knobName, TOutput leftRight, int32 channel )
{
	if ( leftRight == kLeftOutput )
		sprintf(knobName, "LeftGain%i", channel);
	else
		sprintf(knobName, "RightGain%i", channel);
}

Item _ehSetupMixer( const char *mixerName )
/*
	Initialize the mixer and return its Item.
	
	If the instrument can't be loaded or started, it's
	unloaded and an error code is returned.
*/
{
	Item aMixer;
	Err status;

	aMixer = LoadInstrument( (char *) mixerName, 0, 100);
	if ( aMixer < 0)
	{
		status = aMixer;
		EHERR( status, "load mixer" );
		goto CLEANUP;
	}

/* Mixer must be started */
	status = StartInstrument( aMixer, NULL );
	if ( status < 0 )
	{
		EHERR( status, "start mixer" );
		goto CLEANUP;
	}

	return aMixer;

CLEANUP:
	if ( aMixer >= 0 )
		UnloadInstrument(aMixer);

	return (Item) status;
}

void _ehDisposeSample( Item sample, Item samplePlayer )
{
	UnloadInstrument( samplePlayer );
	UnloadSample( sample );
}

void _ehReleaseKnob( Item *knob )
{
	if (*knob >= 0)
	{
		ReleaseKnob( *knob );
		*knob = -1;
	}
}

Err _ehAcquireGainKnob( Item mixer, int32 channel, TOutput leftRight, Item *knob )
{
	Err	status;
	char	aKnobName[11]; /* long enough for "RightGainXX" */
	Item	aKnob = -1;

	_ehGetGainKnobName( aKnobName, leftRight, channel);
	aKnob = GrabKnob( mixer, aKnobName );
	if ( aKnob < 0 )
	{
		status = aKnob;
		EHERR( status, "grab gain knob" );
		goto CLEANUP;
	}

	*knob = aKnob;
	status = TweakKnob( aKnob, MAXDSPAMPLITUDE );
	if ( status < 0 )
	{
		EHERR( status, "tweak knob" );
		goto CLEANUP;
	}

	return 0;

CLEANUP:
	if (aKnob >= 0)
		ReleaseKnob(aKnob);

	return status;
}

void _ehSplitVolume( int32 volume, int32 balance, int32 *leftLevel, int32 *rightLevel )
/*
	Distribute the volume to the left and right outputs according
	to the specified balance. The balance value ranges from 0 (pure left)
	to kMaxBalance (pure right).
*/
{
	*leftLevel = (kMaxBalance - balance) * volume / kMaxBalance;
	*rightLevel = balance * volume / kMaxBalance;
}

/*** Effects-handling API functions: ***/

/**
|||	ehDisposeMixerInfo - Disposes of resources allocated in ehNewMixerInfo
|||
|||	  Synopsis
|||
|||	    void ehDisposeMixerInfo( pTMixerInfo pMixerInfo )
|||
|||	  Description
|||
|||	    This function disposes or releases the resources referenced in a MixerInfo, and
|||		then the MixerInfo itself.
|||
|||	  Arguments
|||
|||	    pMixerInfo	pointer to the MixerInfo to dispose
|||
|||
|||	  Return Value
|||
|||	    [none]
|||
|||
|||	  Implementation
|||
|||	    Call this function when you're finished with a MixerInfo.
|||
|||
|||	  Caveats
|||
|||		The MixerInfo should have been initialized via a prior call to ehNewMixerInfo.
|||		After calling this function, the application should no longer use the passed-in pointer.
|||
|||	  See Also
|||
|||	    ehNewMixerInfo()
|||
**/
void ehDisposeMixerInfo( pTMixerInfo pMixerInfo )
{
	if ( pMixerInfo != NULL )
	{
		if (pMixerInfo->mi_Mixer >= 0)
			UnloadInstrument( pMixerInfo->mi_Mixer );

		FreeMem(pMixerInfo, sizeof(TMixerInfo));
	}

}

/**
|||	ehNewMixerInfo - Allocates a MixerInfo and initializes a mixer.
|||
|||	  Synopsis
|||
|||	    Err ehNewMixerInfo( pTMixerInfo *pNewMixerInfo, int32 channelsUsed, const char *mixerName )
|||
|||	  Description
|||
|||	    This function creates a MixerInfo struct and initializes a mixer for playing
|||		subsequently-loaded samples.
|||
|||	  Arguments
|||
|||	    pNewMixerInfo	A pointer to the variable referencing the newly allocated
|||	                    MixerInfo struct. If the allocation fails, the variable is
|||	                    set to NULL.
|||
|||	    channelsUsed	number of mixer inputs the application intends to use
|||
|||		mixerName		name of the mixer to initialize
|||
|||
|||	  Return Value
|||
|||	    Zero if all operations were successful, otherwise an error code (a negative
|||	    value).
|||
|||
|||	  Implementation
|||
|||	    Call this function to prepare to play sound effects through
|||		a mixer.
|||
|||
|||	  Caveats
|||
|||		Remember to call ehDisposeMixerInfo when you're through with the
|||		MixerInfo.
|||
|||
|||	    effectsHandler.h
|||
|||	  See Also
|||
|||	    ehDisposeMixerInfo()
|||
**/
Err ehNewMixerInfo( pTMixerInfo *pNewMixerInfo, int32 channelsUsed, const char *mixerName )
{
	Err status = -1;

	pTMixerInfo pMixerInfo = (pTMixerInfo) AllocMem ( sizeof(TMixerInfo), MEMTYPE_ANY | MEMTYPE_FILL | 0x00 );
	if ( pMixerInfo == NULL )
	{
		status = EHNOMEM_ERR;
		EHERR( status, "allocate mixerInfo" );
		goto CLEANUP;
	}

	if ( (status = pMixerInfo->mi_Mixer = _ehSetupMixer(mixerName)) < 0 )
		goto CLEANUP;

	pMixerInfo->mi_ChannelsUsed = channelsUsed;
	
	status = 0;
	
CLEANUP:
	if ( (status < 0) && pMixerInfo )
		ehDisposeMixerInfo(pMixerInfo);

	*pNewMixerInfo = pMixerInfo;
	return status;
}

/**
|||	ehNewSampleInfo - Create a SampleInfo and initialize it with safe values.
|||
|||	  Synopsis
|||
|||	    pTSampleInfo ehNewSampleInfo( void )
|||
|||	  Description
|||
|||	    This function allocates memory for a SampleInfo struct and initializes it
|||		so that it can be associated with a sound effect, and later safely disposed.
|||
|||	  Arguments
|||
|||	    [none]
|||
|||
|||	  Return Value
|||
|||	    A pointer to the newly allocated SampleInfo struct.  If the allocation fails,
|||		NULL is returned.
|||
|||
|||	  Implementation
|||
|||	    Call this function if you want to allocate a SampleInfo, but don't need to
|||		immediately set up its player or connect the player to the output instrument (mixer).
|||
|||
|||	  Caveats
|||
|||		If you want to fully initialize a player for a sample and connect it to an output
|||		instrument, use the convenience routine ehLoadSoundEffect.
|||
|||		Remember to call ehDisposeSampleInfo when you're through with the
|||		SampleInfo.
|||
|||		Remember to call ehNewMixerInfo so you can play sound effects through
|||		the associated mixer.
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehDisposeMixerInfo()
|||
**/
pTSampleInfo ehNewSampleInfo( void )
{
	pTSampleInfo pSampleInfo = (pTSampleInfo) AllocMem ( sizeof(TSampleInfo), MEMTYPE_ANY | MEMTYPE_FILL | 0x00 );
	if ( pSampleInfo != NULL )
	{
		pSampleInfo->si_Sample = -1;
		pSampleInfo->si_Player = -1;
		pSampleInfo->si_Attachment = -1;
		pSampleInfo->si_Channel = -1;
		pSampleInfo->si_LeftGainKnob = -1;
		pSampleInfo->si_RightGainKnob = -1;
	}

	return pSampleInfo;
}

/**
|||	ehDisposeSampleInfo - Dispose of resources allocated by ehNewSampleInfo.
|||
|||	  Synopsis
|||
|||	    void ehDisposeSampleInfo( pTSampleInfo pSampleInfo )
|||
|||	  Description
|||
|||	    This function disposes or release resources referenced in a SampleInfo struct,
|||		and then disposed the SampleInfo itself.
|||
|||	  Arguments
|||
|||	    pSampleInfo		pointer to the SampleInfo to be disposed.
|||
|||
|||	  Return Value
|||
|||	    [none]
|||
|||
|||	  Implementation
|||
|||	    Call this function when you're finished with a SampleInfo.
|||
|||
|||	  Caveats
|||
|||		The SampleInfo must be allocated by a call to ehNewSampleInfo (or
|||		the convenience function ehLoadSoundEffect).
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehDisposeMixerInfo(),
|||		ehNewSampleInfo(), ehLoadSoundEffect()
|||
**/
void ehDisposeSampleInfo( pTSampleInfo pSampleInfo )
{
	if ( pSampleInfo != NULL )
	{
		_ehDisposeSample( pSampleInfo->si_Sample, pSampleInfo->si_Player );
		_ehReleaseKnob( &pSampleInfo->si_LeftGainKnob );
		_ehReleaseKnob( &pSampleInfo->si_RightGainKnob );
		FreeMem( pSampleInfo, sizeof(TSampleInfo) );
	}
}

/**
|||	ehSetupSamplePlayer - Prepares all resources required to play a sample.
|||
|||	  Synopsis
|||
|||	    Err ehSetupSamplePlayer( pTSampleInfo pSampleInfo, const char *sampleName )
|||
|||	  Description
|||
|||	    This function prepares all resources required to play a sample, but does
|||		not connect the sample player to the output instrument (mixer).  The
|||		SampleInfo struct is filled in with references to the resources.
|||
|||		The sequence of the steps taken is:
|||
|||			Load the Sample.
|||			Select the appropriate SamplePlayer
|||			Load the SamplePlayer
|||			Attach the Sample to the SamplePlayer
|||
|||
|||	  Arguments
|||
|||	    pSampleInfo		pointer to information about the sample.
|||
|||
|||	  Return Value
|||
|||	    Returns 0 if all playback resources were successfully prepared, or
|||		a negative number if an error occurred (for example, the sample was
|||		not found).
|||
|||
|||	  Implementation
|||
|||	    Call this function if you want to prepare a player for a sample, but
|||		don't need to immediately connect it to an output instrument (mixer).
|||
|||
|||	  Caveats
|||
|||		If you want to fully initialize a player for a sample and connect it to an output
|||		instrument, use the convenience routine ehLoadSoundEffect.
|||
|||		The SampleInfo must be allocated by a call to ehNewSampleInfo (or
|||		the convenience function ehLoadSoundEffect).
|||
|||
|||	  See Also
|||
|||	    ehNewSampleInfo(), ehLoadSoundEffect()
|||
**/
Err ehSetupSamplePlayer( pTSampleInfo pSampleInfo, const char *sampleName )
{
	Item	aSample;
	char	*samplePlayerName;
	Item	aSamplePlayer = -1;
	Item	aAttachment;
	Err		status = -1;

/* Load digital audio Sample from disk. */
	aSample = LoadSample( (char *) sampleName );
	if ( aSample < 0 )
	{
		status = aSample;
		EHERR( status, "load sample" );
		goto CLEANUP;
	}

#if INTENSE_DEBUG
	/* Look at sample information. */
	DebugSample(aSample);
#endif

/* Load Sample player */
	samplePlayerName = SelectSamplePlayer( aSample , TRUE );
	if ( samplePlayerName == NULL )
		samplePlayerName = SelectSamplePlayer( aSample , FALSE );
	if (samplePlayerName == NULL)
	{
		status = EHNOPLAYER_ERR;
		EHERR( status, "select sample player" );
		goto CLEANUP;
	}

	PRT(("Use instrument: %s\n", samplePlayerName));
	aSamplePlayer = LoadInstrument(samplePlayerName,  0, 100);
	if (aSamplePlayer < 0)
	{
		status = aSamplePlayer;
		EHERR( status, "load sample player" );
		goto CLEANUP;
	}

/* Attach the sample to its player */
	aAttachment = AttachSample(aSamplePlayer, aSample, NULL);
	if (aAttachment < 0)
	{
		status = aAttachment;
		EHERR( status, "attach sample" );
		goto CLEANUP;
	}

	pSampleInfo->si_Sample = aSample;
	pSampleInfo->si_Player = aSamplePlayer;
	pSampleInfo->si_Attachment = aAttachment;

	return 0;

CLEANUP:
/* The Audio Folio is immune to passing NULL values as Items. */
	_ehDisposeSample( aSample, aSamplePlayer );

	return status;
}

/**
|||	ehDisconnectSamplePlayer - Disconnect a sample player from an output instrument (mixer).
|||
|||	  Synopsis
|||
|||	    Err ehDisconnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo )
|||
|||	  Description
|||
|||	    This function disconnects the sample player instrument referenced in the SampleInfo from the output
|||		instrument (mixer) referenced in the MixerInfo.  Another player instrument can then be connected
|||	    to the mixer in the same channel.
|||
|||	  Arguments
|||
|||	    pMixerInfo		pointer to information about the output instrument (mixer).
|||
|||		pSampleInfo		pointer to information about the sample.
|||
|||	  Return Value
|||
|||	    Returns 0 if the sample player instrument was successfully disconnected, or
|||		a negative number if an error occurred.
|||
|||
|||	  Implementation
|||
|||	    Call this function when you want to disconnect a sample player from an output instrument.
|||
|||
|||	  Caveats
|||
|||		The SampleInfo should be connected to the output instrument by a call to
|||	    ehConnectSamplePlayer (or the convenience function ehLoadSoundEffect).
|||
|||		The MixerInfo must be allocated by a call to ehNewMixerInfo.
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehNewSampleInfo(), ehLoadSoundEffect(),
|||		ehSetupSamplePlayer(), ehConnectSamplePlayer
|||
**/
Err ehDisconnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo )
{
	Err		status = -1;
	int32	numOutputs = 0;
	Item	mixer = pMixerInfo->mi_Mixer;
	int32	channel;

	if ( pSampleInfo->si_LeftGainKnob >= 0 )
	{
		ReleaseKnob(pSampleInfo->si_LeftGainKnob);
		pSampleInfo->si_LeftGainKnob = -1;
	}

	if ( pSampleInfo->si_RightGainKnob >= 0 )
	{
		ReleaseKnob(pSampleInfo->si_RightGainKnob);
		pSampleInfo->si_LeftGainKnob = -1;
	}

	{
		TagArg getInfoTags[] =
		{
			{ AF_TAG_CHANNELS, 0 },
			{ TAG_END, 0}
		};

		status = GetAudioItemInfo( pSampleInfo->si_Sample, getInfoTags );
		if ( status < 0 )
		{
			EHERR( status, "get info for sample" );
			goto CLEANUP;
		}

		numOutputs = (int32) getInfoTags[0].ta_Arg;
	}

	channel = pSampleInfo->si_Channel;

	switch (numOutputs)
	{
		case 1:
			DisconnectInstruments(pSampleInfo->si_Player, "Output", mixer, gChannelName[channel]);
			break;

		case 2:
			DisconnectInstruments(pSampleInfo->si_Player, "LeftOutput", mixer, gChannelName[channel]);
			break;

		default:
			break;
	}
	
	status = 0;

CLEANUP:
	return status;
}

/**
|||	ehConnectSamplePlayer - Connect a sample player to an output instrument (mixer).
|||
|||	  Synopsis
|||
|||	    Err ehConnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo, int32 channel )
|||
|||	  Description
|||
|||	    This function connects the sample player instrument referenced in the SampleInfo to the output
|||		instrument (mixer) referenced in the MixerInfo.  The player instrument is then ready to be started.
|||
|||		If the sample is stereo, only the left player output is connected to the mixer
|||		channel.
|||
|||	  Arguments
|||
|||	    pMixerInfo		pointer to information about the output instrument (mixer).
|||
|||		pSampleInfo		pointer to information about the sample.
|||
|||		channel			number of the mixer channel to use for playing the sample.
|||
|||	  Return Value
|||
|||	    Returns 0 if the sample player instrument was successfully connected, or
|||		a negative number if an error occurred (for example, the sample was
|||		not found).
|||
|||
|||	  Implementation
|||
|||	    Call this function when you want to connect a sample player to an output instrument.
|||
|||
|||	  Caveats
|||
|||		If you want to fully initialize a player for a sample and connect it to an output
|||		instrument, use the convenience routine ehLoadSoundEffect.
|||
|||		The SampleInfo must be allocated by a call to ehNewSampleInfo (or
|||		the convenience function ehLoadSoundEffect).
|||
|||		The MixerInfo must be allocated by a call to ehNewMixerInfo.
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehNewSampleInfo(), ehLoadSoundEffect(),
|||		ehSetupSamplePlayer()
|||
**/
Err ehConnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo, int32 channel )
{
	Err		status = -1;
	int32 numOutputs = 0;
	Item mixer = pMixerInfo->mi_Mixer;

	{
		TagArg getInfoTags[] =
		{
			{ AF_TAG_CHANNELS, 0 },
			{ TAG_END, 0}
		};

		status = GetAudioItemInfo( pSampleInfo->si_Sample, getInfoTags );
		if ( status < 0 )
		{
			EHERR( status, "get info for sample" );
			goto CLEANUP;
		}

		numOutputs = (int32) getInfoTags[0].ta_Arg;
	}

	switch (numOutputs)
	{
		case 1:
			PRT(("Connecting Output to mixer\n"));
			status = ConnectInstruments(pSampleInfo->si_Player, "Output", mixer, gChannelName[channel]);
			break;

		case 2:
			PRT(("Stereo sample encountered: Connecting Left Output to mixer\n\tIgnoring right channel\n"));
			status= ConnectInstruments(pSampleInfo->si_Player, "LeftOutput", mixer, gChannelName[channel]);
			break;

		default:
			status = EHNUMOUTPUTS_ERR;
			break;
	}

	if ( status < 0 )
	{
		EHERR( status, "connect sample player to mixer" );
		goto CLEANUP;
	}

	status = _ehAcquireGainKnob( mixer, channel, kLeftOutput, &pSampleInfo->si_LeftGainKnob );
	if (status < 0)
		goto CLEANUP;

	status = _ehAcquireGainKnob( mixer, channel, kRightOutput, &pSampleInfo->si_RightGainKnob );
	if (status < 0)
		goto CLEANUP;

	pSampleInfo->si_Channel = channel;

	return 0;

CLEANUP:
	if ( pSampleInfo->si_LeftGainKnob >= 0 )
	{
		ReleaseKnob(pSampleInfo->si_LeftGainKnob);
		pSampleInfo->si_LeftGainKnob = -1;
	}

	switch (numOutputs)
	{
		case 1:
			DisconnectInstruments(pSampleInfo->si_Player, "Output", mixer, gChannelName[channel]);
			break;

		case 2:
			DisconnectInstruments(pSampleInfo->si_Player, "LeftOutput", mixer, gChannelName[channel]);
			break;

		default:
			break;
	}

	return status;
}

/**
|||	ehSetChannelLevels - Set the volume levels for mixer knobs.
|||
|||	  Synopsis
|||
|||	    void ehSetChannelLevels(pTMixerInfo pMixerInfo, Item leftKnob, Item rightKnob, int32 volume, int32 balance)
|||
|||	  Description
|||
|||	    This function determines and sets the knob values for the mixer referenced by a MixerInfo
|||		based on the desired volume and balance values, and the number of players connected to the
|||		mixer.
|||
|||
|||	  Arguments
|||
|||	    pMixerInfo		pointer to information about the output instrument (mixer).
|||
|||		leftKnob		item for the left gain knob of the mixer.
|||
|||		leftKnob		item for the right gain knob of the mixer.
|||
|||		volume			loudness to distribute over the left and right mixer outputs.
|||						The legal 3DO range of 0 .. MAXDSPAMPLITUDE should be observed.
|||
|||		balance			value which specifies how much of the volume is distributed to the left and right
|||						mixer outputs.  It should be in the range 0 .. kMaxBalance.
|||
|||	  Return Value
|||
|||	    [none]
|||
|||
|||	  Implementation
|||
|||	    Call this function to adjust the volume of the left and right mixer outputs.
|||
|||
|||	  Caveats
|||
|||		The passed-in knob items are most conveniently obtained from a SampleInfo allocated via
|||		ehNewSampleInfo (or the convenience function ehLoadSoundEffect).
|||
|||		The MixerInfo must be allocated by a call to ehNewMixerInfo.
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehNewSampleInfo(), ehLoadSoundEffect(),
|||		ehSetupSamplePlayer()
|||
**/
void ehSetChannelLevels(pTMixerInfo pMixerInfo, Item leftKnob, Item rightKnob, int32 volume, int32 balance)
{
	int32 leftLevel;
	int32 rightLevel;

	volume /= pMixerInfo->mi_ChannelsUsed;  /* this channel's contribution to overall volume */

	_ehSplitVolume( volume, balance, &leftLevel, &rightLevel );

	TweakKnob(leftKnob, leftLevel);
	TweakKnob(rightKnob, rightLevel);
}

/**
|||	ehLoadSoundEffect - Load and prepare a sound effect for playback
|||
|||	  Synopsis
|||
|||	    Err ehLoadSoundEffect( pTSampleInfo *pNewSampleInfo, pTMixerInfo pMixerInfo, const char *sampleName, int32 channel )
|||
|||	  Description
|||
|||	    This function loads and prepares a sample for playback as a sound effect.
|||
|||
|||	  Arguments
|||
|||	    pNewSampleInfo  Pointer to the allocated SampleInfo structure, or NULL if an error
|||		                occurred (for example, if the sample was not found)
|||
|||	    pMixerInfo		pointer to information about the output instrument (mixer).
|||
|||		sampleName		filename of the sample to load and prepare.
|||
|||		channel			Mixer output channel through which the sound effect is to be played
|||
|||	  Return Value
|||
|||	    Returns 0 if the sound effect was successfully prepared for playback, or
|||		a negative number if an error occurred.
|||
|||
|||	  Implementation
|||
|||	    Call this convenience function to load and prepare to play a sound effect.
|||
|||
|||	  See Also
|||
|||	    ehNewMixerInfo(), ehNewSampleInfo(), ehNewSampleInfo(),
|||		ehSetupSamplePlayer(), ehConnectSamplePlayer(), ehDisposeSampleInfo()
|||
**/
Err ehLoadSoundEffect( pTSampleInfo *pNewSampleInfo, pTMixerInfo pMixerInfo, const char *sampleName, int32 channel )
{
	Err status = -1;
	pTSampleInfo pSampleInfo = NULL;

	pSampleInfo = ehNewSampleInfo(); /* Cleans up on failure */
	if ( pSampleInfo )
	{
		status = ehSetupSamplePlayer(pSampleInfo, sampleName);
		if ( status >= 0 )
			status = ehConnectSamplePlayer( pMixerInfo, pSampleInfo, channel );
	}
	else
	{
		status = EHNOMEM_ERR;
		EHERR( status, "allocate sampleInfo" );
	}
	
	if ( status < 0 )
	{
		ehDisposeSampleInfo( pSampleInfo );
		pSampleInfo = NULL;
	}
		
	if ( pNewSampleInfo )
		*pNewSampleInfo = pSampleInfo;
		
	return status;
}

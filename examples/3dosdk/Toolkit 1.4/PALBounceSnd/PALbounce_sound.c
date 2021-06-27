/*
	File:		PALbounce_sound.c

	Contains:	3DO BounceSound demo

	Written by:	Darren Gibbs

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	  8/5/93	JAY		changing dsp/ to $audio/dsp/ for DragonTail3
		 <2>	  4/5/93	JAY		removing /remote/ from filename (i.e. making path relative to
									initial working directory)
		 <1>	  4/3/93	JAY		first checked in
				  4/3/93	JAY		xxx put comment here xxx
				 3/31/93	RDG		Added TermBounceSound and did last check before 
									release submission
				 3/30/93	RDG		Took out redundant code in DoObjectCollision- since
									I'm not using the orientation data for the object
									collisions, there's no need to check for it...
				 3/25/93	RDG	 	Update for Cardinal	
				 3/12/93	RDG	 	new	

	To Do:
*/

#include "PALbounce.h"
#include "PALbounce_sound.h"

extern long	gGlobeXPos;
extern long	gGlobeYPos;
extern long	gCubeXPos;
extern long	gCubeYPos;
extern long	gTvXPos;
extern long	gTvYPos;
extern long	gBallXPos;
extern long	gBallYPos;


Item Mixer = 0;
Item Samplers[NUMSAMPLERS];
Item Samples[NUMSAMPLERS];
Item LeftGains[NUMCHANNELS];
Item RightGains[NUMCHANNELS];


/* Define Tags for StartInstrument */
TagArg SamplerTags[] =
	{
		{ AF_TAG_PITCH, 0},
        { 0, 0 }
    };

static char *LeftNames[] = 
{
	"LeftGain0",
	"LeftGain1",
	"LeftGain2",
	"LeftGain3",
	"LeftGain4",
	"LeftGain5",
	"LeftGain6",
	"LeftGain7"
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
	"RightGain7"
};

static char *ChannelNames[] =
{
	"Input0",
	"Input1",
	"Input2",
	"Input3",
	"Input4",
	"Input5",
	"Input6",
	"Input7"
};

int32 InitBounceSound(void)
{
	int32 	Result = 0;
	int32	i;
	Item	Attachment;
	Item	TempItem = 0;		/* For item error checking macro*/

/* Load 8x2 mixer */
	Mixer = LoadInstrument("$audio/dsp/mixer8x2.dsp", 0, 50);
	 CHECKITEM(Mixer,"_InitBounceSound","Couldn't load 8x2 mixer");
	
/* set initial gain and panning. */
		LeftGains[BALL_TV] = GrabKnob( Mixer, LeftNames[0] );
		TweakKnob ( LeftGains[0], BALL_TV_GAIN/2 );
	
		RightGains[BALL_TV] = GrabKnob( Mixer, RightNames[0] );
		TweakKnob ( RightGains[0], BALL_TV_GAIN/2 );

		LeftGains[BALL_GLOBE] = GrabKnob( Mixer, LeftNames[1] );
		TweakKnob ( LeftGains[1], BALL_GLOBE_GAIN/2 );
	
		RightGains[BALL_GLOBE] = GrabKnob( Mixer, RightNames[1] );
		TweakKnob ( RightGains[1], BALL_GLOBE_GAIN/2 );

		LeftGains[TV_CUBE] = GrabKnob( Mixer, LeftNames[2] );
		TweakKnob ( LeftGains[2], TV_CUBE_GAIN/2 );
	
		RightGains[TV_CUBE] = GrabKnob( Mixer, RightNames[2] );
		TweakKnob ( RightGains[2], TV_CUBE_GAIN/2 );

		LeftGains[CUBE_GLOBE] = GrabKnob( Mixer, LeftNames[3] );
		TweakKnob ( LeftGains[3], CUBE_GLOBE_GAIN/2 );
	
		RightGains[CUBE_GLOBE] = GrabKnob( Mixer, RightNames[3] );
		TweakKnob ( RightGains[3], CUBE_GLOBE_GAIN/2 );

		LeftGains[BALL_FLOOR] = GrabKnob( Mixer, LeftNames[4] );
		TweakKnob ( LeftGains[4], BALL_FLOOR_GAIN/2 );
	
		RightGains[BALL_FLOOR] = GrabKnob( Mixer, RightNames[4] );
		TweakKnob ( RightGains[4], BALL_FLOOR_GAIN/2 );

		LeftGains[TV_FLOOR] = GrabKnob( Mixer, LeftNames[5] );
		TweakKnob ( LeftGains[5], TV_FLOOR_GAIN/2 );
	
		RightGains[TV_FLOOR] = GrabKnob( Mixer, RightNames[5] );
		TweakKnob ( RightGains[5], TV_FLOOR_GAIN/2 );

		LeftGains[CUBE_FLOOR] = GrabKnob( Mixer, LeftNames[6] );
		TweakKnob ( LeftGains[6], CUBE_FLOOR_GAIN/2 );
	
		RightGains[CUBE_FLOOR] = GrabKnob( Mixer, RightNames[6] );
		TweakKnob ( RightGains[6], CUBE_FLOOR_GAIN/2 );

		LeftGains[GLOBE_FLOOR] = GrabKnob( Mixer, LeftNames[7] );
		TweakKnob ( LeftGains[7], GLOBE_FLOOR_GAIN/2 );
	
		RightGains[GLOBE_FLOOR] = GrabKnob( Mixer, RightNames[7] );
		TweakKnob ( RightGains[7], GLOBE_FLOOR_GAIN/2 );

/* Mixer must be started */
	Result = StartInstrument( Mixer, NULL );

/* Load up Samples 	*/
	Samples[BALL_TV] = LoadSample(BALL_TV_SND);
	 CHECKITEM(Samples[BALL_TV],"_InitBounceSound","Couldnt load BALL_TV snd");
	Samples[BALL_GLOBE] = LoadSample(BALL_GLOBE_SND);
	 CHECKITEM(Samples[BALL_GLOBE],"_InitBounceSound","Couldnt load BALL_GLOBE snd");
	Samples[TV_CUBE] = LoadSample(TV_CUBE_SND);
	 CHECKITEM(Samples[TV_CUBE],"_InitBounceSound","Couldnt load TV_CUBE snd");
	Samples[CUBE_GLOBE] = LoadSample(CUBE_GLOBE_SND);
	 CHECKITEM(Samples[CUBE_GLOBE],"_InitBounceSound","Couldnt load CUBE_GLOBE snd");
	Samples[BALL_FLOOR] = LoadSample(BALL_FLOOR_SND);
	 CHECKITEM(Samples[BALL_FLOOR],"_InitBounceSound","Couldnt load BALL_FLOOR snd");
	Samples[TV_FLOOR] = LoadSample(TV_FLOOR_SND);
	 CHECKITEM(Samples[TV_FLOOR],"_InitBounceSound","Couldnt load TV_FLOOR snd");
	Samples[CUBE_FLOOR] = LoadSample(CUBE_FLOOR_SND);
	 CHECKITEM(Samples[CUBE_FLOOR],"_InitBounceSound","Couldnt load CUBE_FLOOR snd");
	Samples[GLOBE_FLOOR] = LoadSample(GLOBE_FLOOR_SND);
	 CHECKITEM(Samples[GLOBE_FLOOR],"_InitBounceSound","Couldnt load GLOBE_FLOOR snd");
	 

/* Make Sampler instruments based on template. */
	for (i=0; i<NUMSAMPLERS; i++)
	{
		Samplers[i] =  LoadInstrument("$audio/dsp/sampler.dsp", 0, 100);
		 CHECKITEM(Samplers[i],"_InitBounceSounds","Couldn't load a sampler");

		Attachment = AttachSample(Samplers[i], Samples[i], 0);
		 CHECKITEM(Attachment,"_InitBounceSounds","Couldn't attach a sample");

		Result = ConnectInstruments (Samplers[i], "Output", Mixer,
			ChannelNames[i]);
	 	 CHECKRESULT(Result,"_InitBounceSounds","Couldn't connect a sampler to mixer");
	}

error:
	return Result;
}

void DoObjectCollisionSound(uint32 IAFlags)
{
	int32 	Result = 0;

	if (BALL_TV_COLL & IAFlags)
	{
		PanMixerChannel(BALL_TV, BALL_TV_GAIN, gBallXPos);
		SamplerTags[0].ta_Arg = (int32 *) YPositionToPitch(gBallYPos);
		Result = StartInstrument(Samplers[BALL_TV], &SamplerTags[0]);
		ReleaseInstrument(Samplers[BALL_TV], NULL);
		goto DONE;
	}


	if (BALL_GLOBE_COLL & IAFlags)
	{
		PanMixerChannel(BALL_GLOBE, BALL_GLOBE_GAIN, gGlobeXPos);
		SamplerTags[0].ta_Arg = (int32 *) YPositionToPitch(gGlobeYPos);
		Result = StartInstrument(Samplers[BALL_GLOBE], &SamplerTags[0]);
		ReleaseInstrument(Samplers[BALL_GLOBE], NULL);
		goto DONE;
	}

	
	if (TV_CUBE_COLL & IAFlags)
	{
		PanMixerChannel(TV_CUBE, TV_CUBE_GAIN, gTvXPos);
		SamplerTags[0].ta_Arg = (int32 *) YPositionToPitch(gTvYPos);
		Result = StartInstrument(Samplers[TV_CUBE], &SamplerTags[0]);
		ReleaseInstrument(Samplers[TV_CUBE], NULL);
		goto DONE;
	}


	if (CUBE_GLOBE_COLL & IAFlags)
	{
		PanMixerChannel(CUBE_GLOBE, CUBE_GLOBE_GAIN, gCubeXPos);
		SamplerTags[0].ta_Arg = (int32 *) YPositionToPitch(gCubeYPos);
		Result = StartInstrument(Samplers[CUBE_GLOBE], &SamplerTags[0]);
		ReleaseInstrument(Samplers[CUBE_GLOBE], NULL);
		goto DONE;
	}

DONE:
	Result = 0;
}

void DoRoomCollisionSound(uint32 IAFlags)
{
	int32 	Result = 0;

	if ((BALL | FLOOR) == (IAFlags))
	{
		PanMixerChannel(BALL_FLOOR, BALL_FLOOR_GAIN, gBallXPos);
		Result = StartInstrument(Samplers[BALL_FLOOR], NULL);
		ReleaseInstrument(Samplers[BALL_FLOOR], NULL);
		goto DONE;
	}
	if ((TV | FLOOR) == (IAFlags))
	{
		PanMixerChannel(TV_FLOOR, TV_FLOOR_GAIN, gTvXPos);
		Result = StartInstrument(Samplers[TV_FLOOR], NULL);
		ReleaseInstrument(Samplers[TV_FLOOR], NULL);
		goto DONE;
	}
	if ((CUBE | FLOOR) == (IAFlags))
	{
		PanMixerChannel(CUBE_FLOOR, CUBE_FLOOR_GAIN, gCubeXPos);
		Result = StartInstrument(Samplers[CUBE_FLOOR], NULL);
		ReleaseInstrument(Samplers[CUBE_FLOOR], NULL);
		goto DONE;
	}
	if ((GLOBE | FLOOR) == (IAFlags))
	{
		PanMixerChannel(GLOBE_FLOOR, GLOBE_FLOOR_GAIN, gGlobeXPos);
		Result = StartInstrument(Samplers[GLOBE_FLOOR], NULL);
		ReleaseInstrument(Samplers[GLOBE_FLOOR], NULL);
		goto DONE;
	}

DONE:
	Result = 0;
}

void PanMixerChannel(int32 ChannelNumber, int32 MaxAmp, int32 Pan)
{
	int32 RightGain, LeftGain;
	
	/* The code below scales gain so that 
		if MaxAmp = 2000
		  LeftPan 	= 2000 left and 0 right
		  CtrPan 	= 1000 on left and right
		  RightPan 	= 2000 Right and 0 left
	*/
	RightGain = ((MaxAmp * (Pan - LEFT_WALL_POS)) / WINDOW_WIDTH);	
	LeftGain = ((MaxAmp * (WINDOW_WIDTH - (Pan - LEFT_WALL_POS))) / WINDOW_WIDTH);
	
	TweakKnob(LeftGains[ChannelNumber], LeftGain);
	TweakKnob(RightGains[ChannelNumber], RightGain);
	
}

int32 YPositionToPitch(int32 YPosition)
{
	return (((((DISPLAY_HEIGHT - YPosition) * 10) / 100) + 50));
}

void TermBounceSound ( void )
{
	int32	i;
	
	/* Make Sampler instruments based on template. */
	for (i=0; i<NUMSAMPLERS; i++)
	{
		UnloadInstrument(Samplers[i]);
	}
}

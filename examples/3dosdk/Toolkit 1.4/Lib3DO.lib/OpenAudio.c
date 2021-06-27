/*
	File:		OpenAudio.c

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	  9/6/93	JAY		change 2nd argument in the LoadInstrument to 0 for DragonTail6.
									Removed conditional compiles and search directories for finding
									dsp files
		 <2>	  8/5/93	JAY		putting in place the final system directory scheme for the
									Dragon Tail release. Affects how dsp, aiff files are referenced.
									Getting rid of the multiple trials of pathnames (almost).
									Replacing with alias $audio which should work forever in any
									environment

	To Do:
*/

#include "Portfolio.h"
#include "filefunctions.h"
#include "Init3DO.h"

#if DEBUG
#define DIAGNOSTIC(s)		kprintf("Error: %s\n", s)
#define DIAGSTRING(s,t)		kprintf("Fyi: %s %s\n", s ,t )
#else
#define DIAGNOSTIC(s)
#define DIAGSTRING(s,t)
#endif

/*=========================================================
	GLOBAL VARIABLES ACCESSED BY OTHER LIBS

	NOTE: DO NOT TAMPER WITH THESE NAMES
	UNLESS YOU ALSO CHANGE THEIR REFERENCES
	IN PARSE3DO.LIB AND UTILS3DO.LIB.
--------------------------------------------------------*/
	
	Item TheMixer = -1;

/*========================================================*/


bool OpenAudio(void)
	{
#define DEFAULT_MIXER_SETTING 0x2000
	char LeftName[32];
	char RightName[32];
	Item left, right;
	char cChannel;
	int i;
	char name[32];
	
	strcpy( LeftName, "LeftGain0" );
	strcpy( RightName, "RightGain0" );

	if (OpenAudioFolio())
		{
		DIAGNOSTIC("Cannot open audio folio");
		return FALSE;
		}
	
	strcpy(name,"$audio/dsp/mixer4x2.dsp");
	TheMixer = LoadInstrument(name, 0, 0);

	if ((TheMixer ) < 0)
		{
		DIAGNOSTIC("Cannot load mixer instrument");
		return FALSE;
		}
		
	for (cChannel = '0'; cChannel < '4'; cChannel++)
		{
		LeftName[8] = cChannel;
		RightName[9] = cChannel;
		left = GrabKnob(TheMixer, LeftName);
		right = GrabKnob(TheMixer, RightName);
		if (right < 0 || left < 0)
			{
			DIAGNOSTIC("Cannot set mixer channel");
			return FALSE;
			}
		TweakKnob(right, DEFAULT_MIXER_SETTING);
		TweakKnob(left, DEFAULT_MIXER_SETTING);
		ReleaseKnob(right);
		ReleaseKnob(left);
		}
	
	(void)StartInstrument(TheMixer, NULL);

	return TRUE;
	}

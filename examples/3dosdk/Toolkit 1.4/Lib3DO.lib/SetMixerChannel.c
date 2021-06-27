#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*==============================================================
	EXTERNAL GLOBAL VARIABLES
	These variables are exported from Init3DO.lib.c.
---------------------------------------------------------------*/

	extern Item TheMixer;
/*=============================================================*/

bool SetMixerChannel(int nChannel, int32 nLeftLevel, int32 nRightLevel)
	{
	static int nLastChannel = -1;
	static Item iLastLeft = -1;
	static Item iLastRight = -1;
	
	static char LeftName[] = "LeftGain0";
	static char RightName[] = "RightGain0";

	if (nChannel != nLastChannel)
		{
		ReleaseKnob(iLastLeft);
		ReleaseKnob(iLastRight);
		nLastChannel = nChannel;
		
		LeftName[8] = nChannel + '0';
		RightName[9] = nChannel + '0';
		iLastLeft = GrabKnob(TheMixer, LeftName);
		iLastRight = GrabKnob(TheMixer, RightName);
		if (iLastLeft < 0 || iLastRight < 0)
			{
			DIAGNOSTIC("Cannot attach balance knobs");
			return FALSE;
			}
		}
#ifndef CardinalChanges		
	TweakKnob(iLastLeft, nLeftLevel);
	TweakKnob(iLastRight, nRightLevel);
#else
	PutKnob(iLastLeft, nLeftLevel);
	PutKnob(iLastRight, nRightLevel);
#endif
	
	return TRUE;
	}

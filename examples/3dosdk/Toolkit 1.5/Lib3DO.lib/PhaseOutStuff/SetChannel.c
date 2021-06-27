/*****************************************************************************
 *	File:			SetChannel.c 
 *
 *	Contains:		Connect an instrument to a channel of the global mixer.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This routine is not thread-safe.
 ****************************************************************************/
#include "audio.h"
#include "Utils3DO.h"


extern Item TheMixer;

Boolean SetChannel(Item iInstrument, int nChannel)
{
	char ChannelName[] = "Input0";
	
	ChannelName[5] = nChannel + '0';
	if (ConnectInstruments(iInstrument, "Output", TheMixer, ChannelName) < 0)
		return FALSE;
	else
		return TRUE;
}

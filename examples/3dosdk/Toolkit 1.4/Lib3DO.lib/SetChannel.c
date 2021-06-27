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

bool SetChannel(Item iInstrument, int nChannel)
	{
	char ChannelName[] = "Input0";
	
	ChannelName[5] = nChannel + '0';
	if (ConnectInstruments(iInstrument, "Output", TheMixer, ChannelName) < 0)
		return FALSE;
	else
		return TRUE;
	}

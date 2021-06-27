#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

bool SetMixer(int nChannel, int32 nVolume, int32 nBalance)
	{
	int32 nLeftLevel, nRightLevel;
	int i;
	
	nVolume >>= 2;	/* Split master volume over four channels */
	
	if (nBalance < 0x4000)
		{
		nLeftLevel = nVolume;
		nRightLevel = (nBalance * nVolume) >> 14;
		}
	else
		{
		nRightLevel = nVolume;
		nLeftLevel = ((0x8000 - nBalance) * nVolume) >> 14;
		}

	if (nChannel < 0)
		{
		i = 0;
		nChannel = 3;
		}
	else
		{
		i = nChannel;
		}
		
	for (; i <= nChannel; i++)
		{
		if (!SetMixerChannel(i, nLeftLevel, nRightLevel))
			return FALSE;
		}
		
	return TRUE;
	}

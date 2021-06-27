/*
	File:		CSound.h

	Contains:	Sound class (header).

	Written by:	Paul A. Bauersfeld
				Jon A. Weiner

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

	<1>	 10/28/93	pab		New today.

	To Do:
*/
#ifndef _CSOUND_H_
#define _CSOUND_H_

#include "soundfile.h"

#define NUMBLOCKS 		16
#define BLOCKSIZE 		2048
#define BUFSIZE 		(NUMBLOCKS * BLOCKSIZE) 

class CSound 
{
	public:
		CSound (char *pSndFileName, long vol = 0x1000);
		~CSound (void);
		
		long GetVolume(void);
		void Loop(long nTimes);
		void Play(void);
		void SetVolume(long vol);
		void Stop(void);
		void Rewind(void);
		
	private:
		SoundFilePlayer  *fSndFile;
		long fVolume;
};

#endif
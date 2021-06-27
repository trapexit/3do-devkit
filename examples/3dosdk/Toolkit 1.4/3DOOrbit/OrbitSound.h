/*
	File:		OrbitSound.h

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland,lynn ackler,philippe cassereau

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 3/17/93	JAY		making changes for Cardinal

	To Do:
*/

/*********************************************************************************************
 *	File:			OrbitSound.h
 *
 *	Contains:		definitions for Orbit sound support code
 *
 *	Copyright © 1992 The 3DO Company
 *
 *	All rights reserved. This material constitutes confidential and proprietary 
 *	information of the 3DO Company and shall not be used by any Person or for any 
 *	purpose except as expressly authorized in writing by the 3DO Company.
 *
 *	History:
 *	-------
 *	12/30/92		jb		Add symbols for the file names.
 *	12/29/92		jb		New today.
 *********************************************************************************************/

#ifndef __OrbitSOUND_H__
#define __OrbitSOUND_H__

#include "types.h"

#define Use_Announce

bool	InitOrbitSound( void );
void	StopAllOrbitSound( void );

void	PlayStop( long numSegmentsRemoved );
void	PlayAnnounce(void);
#endif	/* __OrbitSOUND_H__ */

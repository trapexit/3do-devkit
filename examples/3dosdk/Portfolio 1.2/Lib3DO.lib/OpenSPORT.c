/*
	File:		OpenSPORT.c

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 7/28/93	JAY		creating extern for SPORTDevice that use to be in graphics.h
									before Dragon Tail release.  Will most likely be removed for
									final release

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
	
	Item SPORTIO[2] = {-1};

/*========================================================*/

Item SPORTDevice;

bool OpenSPORT( void )
	{
	int i;
	bool retvalue;
	TagArg targs[2];
	
/*??? SPORTDevice is opened as a global in gfx.lib as part of*/
/*??? OpenGraphicsFolio().  If this changes, the following code */
/*??? will be re-enabled.*/
	if ((SPORTDevice = OpenItem( 
			FindNamedItem(MKNODEID(KERNELNODE, DEVICENODE), "SPORT"), 0 )) < 0)
		{
		DIAGNOSTIC( "SPORT open failed" );
		goto DONE;
		}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *)SPORTDevice;
	targs[1].ta_Tag = TAG_END;

	for ( i = 1;  i >= 0; i-- )
		if ( (SPORTIO[i] = CreateItem (MKNODEID (KERNELNODE,
				IOREQNODE), targs)) < 0 )
			{
			DIAGNOSTIC( "Can't create SPORT I/O" );
			goto DONE;
			}

	retvalue = TRUE;
	
DONE:
	return( retvalue );	
	}

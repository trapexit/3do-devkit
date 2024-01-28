/*****************************************************************************
 *	File:			OpenSPORT.c
 *
 *	Contains:		Routine for opening the SPORT device.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	This function is now obsolete.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Debug3DO.h"
#include "Init3DO.h"

Boolean
OpenSPORT (void)
{
  return TRUE;
}

#if 0 // obsolete code...

Item SPORTIO[2] = {-1, -1};
Item SPORTDevice;

Boolean OpenSPORT( void )
{
	int i;
	bool retvalue = FALSE;
	TagArg targs[2];
	
	if ((SPORTDevice = OpenItem( 
			FindNamedItem(MKNODEID(KERNELNODE, DEVICENODE), "SPORT"), 0 )) < 0)
		{
		DIAGNOSE(( "SPORT open failed" ));
		goto DONE;
		}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *)SPORTDevice;
	targs[1].ta_Tag = TAG_END;

	for ( i = 1;  i >= 0; i-- )
		if ( (SPORTIO[i] = CreateItem (MKNODEID (KERNELNODE,
				IOREQNODE), targs)) < 0 )
			{
			DIAGNOSE(( "Can't create SPORT I/O" ));
			goto DONE;
			}

	retvalue = TRUE;
	
DONE:

	return( retvalue );	
}

#endif // obsolete

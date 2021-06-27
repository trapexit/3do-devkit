
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: getvideoinfo.c,v 1.3 1994/11/23 18:29:39 vertex Exp $
**
******************************************************************************/

#include "graphics.h"
#include "debug3do.h"

#include "getvideoinfo.h"

int32 GetDisplayType( void )
/* Get the display type for the video being used. */
{
	int32		displayType = -1;
	int32		errorCode;

	/*  Determine field frequency. */
	errorCode = QueryGraphics(QUERYGRAF_TAG_DEFAULTDISPLAYTYPE, (void *) &displayType);
	if (errorCode < 0)
		{
		DIAGNOSE_SYSERR( errorCode, ("Can't query default display type\n") );
		}

	return displayType;
}

int32 GetScreenWidth( int32 displayType )
/* Get the screen width corresponding to the specified display type */
{
	int32 width = -1;

	switch ( displayType )
	{
		case DI_TYPE_NTSC:
			width = NTSC_SCREEN_WIDTH;
			break;

		case DI_TYPE_PAL1:
			width = PAL1_SCREEN_WIDTH;
			break;

		case DI_TYPE_PAL2:
			width = PAL2_SCREEN_WIDTH;
			break;

		default:
			PRT( ("GetScreenWidth: Unknown display type: %i\n", displayType) );
			break;
	}

	return width;

}

int32 GetScreenHeight( int32 displayType )
/* Get the screen height corresponding to the specified display type */
{
	int32 height = -1;

	switch ( displayType )
	{
		case DI_TYPE_NTSC:
			height = NTSC_SCREEN_HEIGHT;
			break;

		case DI_TYPE_PAL1:
			height = PAL1_SCREEN_HEIGHT;
			break;

		case DI_TYPE_PAL2:
			height = PAL2_SCREEN_HEIGHT;
			break;

		default:
			PRT( ("GetScreenHeight: Unknown display type: %i\n", displayType) );
			break;
	}

	return height;

}

/*******************************************************************************************
 *	File:			TestAM.c
 *
 *	Contains:		Asset Manager Test Program
 *
 *	Usage:			TestDS <-c | -a | -s> <streamFile> ...
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/20/93		jb		New today
 *
 *******************************************************************************************/

#define	PROGRAM_VERSION_STRING		"1.0"

#include "Portfolio.h"
#include "Init3DO.h"
#include "Utils3DO.h"

#include "AssetMgr.h"

/*********************************************************************************************
 * Library and system globals
 *********************************************************************************************/
ScreenContext	gScreenContext;
ubyte*			gBackgroundPtr		= NULL;
long 			gScreenSelect		= 0;
Item			VBLIOReq;			// IO Parameter for timer in WaitVBL

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean	StartUp( void );


/*===========================================================================================
 ============================================================================================
									Utility Routines
 ============================================================================================
 ===========================================================================================*/


/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean		StartUp( void )
	{
	gScreenContext.sc_nScreens = 2;
	
	VBLIOReq = GetVBLIOReq();
	
	if ( ! OpenGraphics( &gScreenContext, 2 ) )
		return false;

	if ( ! OpenMacLink() );

	if ( ! OpenSPORT() )
		return false;

	if ( ! OpenAudio() )
		return false;

	if ( (OpenMathFolio() != 0) )
		return false;

	return true;
	}


/*===========================================================================================
 ============================================================================================
									Main Program Logic
 ============================================================================================
 ===========================================================================================*/


/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int main (int argc, char **argv)
	{
	int32			status;
	AssetFileDesc	afd;
	char*			pData;
	long			rectNum;
	long			strCount;
	long			stringIndex;

	/* Initialize the library code */
	if ( ! StartUp() )
		{
		printf( "StartUp() initialization failed!\n" );
		exit(0);
		}

	/* Load the asset file into memory */
	status = LoadAssetFile( argv[1], &afd );
	if ( status != 0 )
		{
		printf( "LoadAssetFile() " );
		goto ERROREXIT;		
		}

	printf( "\n" );

	/* Dump out all the 'rect' resources */
	for ( rectNum = 128; rectNum <= 130; rectNum++ )
		{
		status = GetAsset( &afd, 'rect', rectNum, (void**) &pData );
		if ( status != 0 )
			{
			printf( "GetAsset() #1 " );
			goto ERROREXIT;		
			}

		printf( "rect %ld: %d, %d\n", rectNum, *((short *) pData), *(((short *) pData) + 1) );
		}

	/* Dump all the STR# resources */
	status = GetAsset( &afd, 'STR#', 128, (void**) &pData );
	if ( status != 0 )
		{
		printf( "GetAsset() #2 " );
		goto ERROREXIT;		
		}

	strCount = (long) *((short *) pData);
	pData += sizeof(short);
	while ( strCount--  > 0)
		{
		for ( stringIndex = 0; stringIndex < *pData; stringIndex++ )
			printf( "%c", pData[ stringIndex + 1 ] );	// + 1 to account for length byte

		printf( "\n" );
		pData += *pData + 1;
		}

ERROREXIT:
	if ( status != 0 )
		{
		printf( "error %ld: ", status );
		PrintfSysErr( status );
		}

	UnloadAssetFile( &afd );
	exit(0);
	}

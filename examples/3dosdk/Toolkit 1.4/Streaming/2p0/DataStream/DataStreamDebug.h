/*******************************************************************************************
 *	File:			DataStreamDebug.h
 *
 *	Contains:		definitions for DataStreamDebug.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/15/93		jb		Switch to "%lx" formatting for dsResult
 *	6/2/93		jb		Add CHECK_DS_RESULT macro and make it sensitive to the make file
 *						compile flag DEBUG. If DEBUG=0, then no code is generated.
 *	5/21/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __DATASTREAMDEBUG_H__
#define __DATASTREAMDEBUG_H__

#ifndef __DATASTREAM_H__
#include "DataStream.h"
#endif

#if DEBUG
	#ifndef _STDLIB_H
	#include "stdlib.h"			/* for exit() */
	#endif

	#ifndef _STDIO_H
	#include "stdio.h"			/* for printf() */
	#endif

	#define CHECK_DS_RESULT( name, dsResult ) 					\
		if ( ((int32) dsResult) < 0 )							\
			{ 													\
			printf( "Failure in %s: $%lx\n", name, ((int32) dsResult) );	\
			PrintfDSError( ((int32) dsResult) ); 				\
			exit( 0 );											\
			}
#else
	#define CHECK_DS_RESULT( name, dsResult )
#endif


/*****************************/
/* Public routine prototypes */
/*****************************/
void	PrintfDSError( int32 dsResult );

#endif	/* __DATASTREAMDEBUG_H__ */


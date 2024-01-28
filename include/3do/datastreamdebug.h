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
 *	1/20/94		rdg		make C++ compatible
 *	6/15/93		jb		Switch to "%lx" formatting for dsResult
 *	6/2/93		jb		Add CHECK_DS_RESULT macro and make it sensitive to the make file
 *						compile flag DEBUG. If DEBUG=0, then no code is generated.
 *	5/21/93		jb		New today.
 *
 *******************************************************************************************/
#pragma include_only_once

#include "extern_c.h"

#include "datastream.h"

#if DEBUG

#include "stdlib.h"		/* for exit() */
#include "stdio.h"		/* for printf() */

#define CHECK_DS_RESULT( name, dsResult )                               \
  if ( ((int32) dsResult) < 0 )                                         \
    {                                                                   \
      printf( "Failure in %s: $%lx\n", name, ((int32) dsResult) );	\
      PrintfDSError( ((int32) dsResult) ); 				\
      exit( 0 );                                                        \
    }
#else
#define CHECK_DS_RESULT( name, dsResult )
#endif


/*****************************/
/* Public routine prototypes */
/*****************************/

EXTERN_C_BEGIN

void PrintfDSError(int32 dsResult);

EXTERN_C_END

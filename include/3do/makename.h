#pragma include_only_once

/*******************************************************************************************
 *	File:			MakeName.h
 *
 *	Contains:		definitions for MakeName.c
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/

/*****************************/
/* Public routine prototypes */
/*****************************/

#include "extern_c.h"

EXTERN_C_BEGIN

char* MakeName(char* outputNameBuf, int32 maxNameLen, char* baseNameString, int32 uniqueID);

EXTERN_C_END

#pragma once

/*******************************************************************************************
 *	File:			MakeName.h
 *
 *	Contains:		definitions for MakeName.c
 *
 *	Copyright (c) 1992-93 The 3DO Company. All Rights Reserved.
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
#include "types.h"


EXTERN_C_BEGIN

char* MakeName(char* outputNameBuf, s32 maxNameLen, char* baseNameString, s32 uniqueID);

EXTERN_C_END

#pragma include_only_once

/*******************************************************************************************
 *	File:			SemHelper.h
 *
 *	Contains:		definitions for SemHelper.c
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/

#include "extern_c.h"

#include "semaphore.h"
#include "makename.h"

/********************************************************/
/* Macros to make semaphore operations consistent with	*/
/* "new, dispose, wait, and signal" paradigm			*/
/********************************************************/
#define	DisposeSemaphore( semaphore )  DeleteItem( semaphore )
#define	WaitSemaphore( semaphore )     LockSemaphore( semaphore, 1 )
#define	SignalSemaphore( semaphore )   UnlockSemaphore( semaphore )

/*****************************/
/* Public routine prototypes */
/*****************************/

EXTERN_C_BEGIN

Item NewSemaphore(char* baseNameString, int32 uniqueID);

EXTERN_C_END

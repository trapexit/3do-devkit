/*******************************************************************************************
 *	File:			SemHelper.h
 *
 *	Contains:		definitions for SemHelper.c
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/

#ifndef __SEMHELPER_H__
#define __SEMHELPER_H__

#ifndef _SEMAPHORE_H
#include "semaphore.h"
#endif

#ifndef __MAKENAME_H__
#include "MakeName.h"
#endif

/********************************************************/
/* Macros to make semaphore operations consistent with	*/
/* "new, dispose, wait, and signal" paradigm			*/
/********************************************************/
#define	DisposeSemaphore( semaphore )	DeleteItem( semaphore )
#define	WaitSemaphore( semaphore )		LockSemaphore( semaphore, 1 )
#define	SignalSemaphore( semaphore )	UnlockSemaphore( semaphore )

/*****************************/
/* Public routine prototypes */
/*****************************/
Item	NewSemaphore( char* baseNameString, int32 uniqueID );

#endif	/* __SEMHELPER_H__ */

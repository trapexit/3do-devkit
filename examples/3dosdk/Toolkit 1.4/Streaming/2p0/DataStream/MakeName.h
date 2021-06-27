/*******************************************************************************************
 *	File:			MakeName.h
 *
 *	Contains:		definitions for MakeName.c
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/
#ifndef __MAKENAME_H__
#define __MAKENAME_H__

/*****************************/
/* Public routine prototypes */
/*****************************/

char*	MakeName( char* outputNameBuf, int32 maxNameLen, char* baseNameString, int32 uniqueID );

#endif	/* __MAKENAME_H__ */

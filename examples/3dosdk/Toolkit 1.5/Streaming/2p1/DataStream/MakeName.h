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
#ifndef __MAKENAME_H__
#define __MAKENAME_H__

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  char *MakeName (char *outputNameBuf, int32 maxNameLen, char *baseNameString,
                  int32 uniqueID);

#ifdef __cplusplus
}
#endif

#endif /* __MAKENAME_H__ */

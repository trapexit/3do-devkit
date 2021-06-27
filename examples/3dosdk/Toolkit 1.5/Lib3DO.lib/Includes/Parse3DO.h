/*****************************************************************************
 *	File:		Parse3DO.h
 *
 *	Contains:	Datatypes and prototypes for handling Cel, Image, Anim, and
 *				Sound data stored in standard 3DO file formats.
 *
 *	Written by: Neil Cormia
 *
 *	Copyright:	(c) 1993-1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *	07/11/94  Ian	Standarized protective wrapper macro name to PARSE3DO_H.
 *					Standardized comment blocks.
 *	08/06/93  JML 	Changed parameters to LoadAnim and ParseAnim.
 *	07/29/93  ian 	Added UnloadImage(), UnloadCel(), UnloadAnim().
 *					Changed LoadCel() and LoadAnim() to OldLoadCel()
 *					and OldLoadAnim(), then added new versions of them.
 *					Added ParseCel() and ParseAnim().
 *	01/28/93  dsm 	Dynamically allocate frame entries in ANIM structure.
 *	01/03/93  dsm 	Magneto5 Update
 *	01/02/93  pgb 	upgrade ifdefs to Rel_Magneto4. Also declared
 *					FreeSoundEffects(), and FindChunk(), GetChunk() as externals...
 *	12/23/92  pgb 	tidier LoadSoundFX, LoadSoundEffect, FreeSoundEffect routines
 *	12/10/92  JML 	Changed LoadImage to handle VDLs better.
 *	12/09/92  JML 	first checked in
 *
 *	Implementation notes:
 *
 *	This header is all but obsolete.  It includes the current proper headers
 *	(CelUtils, AnimUtils, etc).  It exists for compatibility for old code.
 ****************************************************************************/

#pragma include_only_once

#ifndef PARSE3DO_H
#define PARSE3DO_H

#include "Init3DO.h"
#include "AnimUtils.h"
#include "CelUtils.h"

/*----------------------------------------------------------------------------
 * prototypes
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus 
  extern "C" {
#endif

int32	GetFileSize(char *fn);
int		ReadFile(char *filename, int32 size, int32 *buffer, int32 offset);

CCB *	OldLoadCel (char *name, int32 **buffer);
int32	OldLoadAnim (char *name, int32 **buffer, ANIM *pAnim);
void 	OldFreeAnimFrames(ANIM *pAnim);

#ifdef __cplusplus
  }
#endif

#endif

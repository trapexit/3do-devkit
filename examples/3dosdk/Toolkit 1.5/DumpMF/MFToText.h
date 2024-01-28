/*******************************************************************************************
 *	File:			MFToText.h
 *
 *	Contains:		Interface to module which converts MIDIFiles into
 *text.
 *
 *	Usage:			Err MIDIFileToText( FILE* fp );
 *
 *
 *
 *	Written by:		Software Attic
 *					 with a few clever ideas courtesy of Phil
 *Burk
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	06/02/94	rdg		Port back to MPW after debugging in
 *ThinkC 05/23/94	rdg		NewToday
 *
 *******************************************************************************************/

#ifndef __MFTOTEXT_H__
#define __MFTOTEXT_H__

#include "ParseMF.h"
#include "types.h"

typedef enum MFTTErrorCodes
{
  MFTTErrNullFilePtr = -1000 /* The file ptr passed in was NULL */
};

/* Prototypes */
Err MIDIFileToText (FILE *fp);

#endif /* __MFTOTEXT_H__ */

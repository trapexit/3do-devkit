/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1
 *with
 **							squareroot-delta-exactencoding and
 *write an AIFC file.
 **							Throw out all chunks but SSND FORM
 *COMM MARK INST FVER
 **
 **	This File:		SquashSnd.h
 **
 ** Contains:		Project wide typedefs, structs, and macros
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and
 *proprietary *	information of the 3DO Company and shall not be used by any
 *Person or for any *	purpose except as expressly authorized in writing by
 *the 3DO Company.
 **
 **
 **  History:
 **  -------
 **	 6/17/93	RDG		version 1.0
 **	 6/15/93	RDG		version .12
 **	 6/15/93	RDG		modify to use MacFileIO routines
 **	 3/10/93	RDG	    Version .11
 **						  -new  support for stereo
 *samples
 **						  -fix  sets the creator and type of the
 *new AIFC file *	 3/10/93	RDG		altered the prototype
 *of CompressSSND to pass the CommChunk *	 2/11/93	RDG
 *New Today
 **
 **************************************************************************************/

#ifndef __SQUASHSND_H__
#define __SQUASHSND_H__

#include "MacFileIO.h"
#include "iff_tools.h"
#include "twotoone.h"
#include <CType.h>
#include <Files.h>
#include <StdLib.h>
#include <Strings.h>
#include <stdio.h>
#include <string.h>

/* Macro to simplify error checking. */
#define CHECKRESULT(val, routine, string)                                     \
  if (val != 0)                                                               \
    {                                                                         \
      Result = val;                                                           \
      printf ("\n\nFATAL ERROR!!!\n ");                                       \
      printf ("------> in %s: %s\n", routine, string);                        \
      goto error;                                                             \
    }

long gVerbose = false;       /* flag for verbose mode */
ExtCommonChunk theCommChunk; /* com chunk passed to twotoone.c */

#endif /* __SQUASHSND_H__ */

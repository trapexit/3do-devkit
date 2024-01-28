/**************************************************************************************
 **	Project:		AIFF_2to1 - Read AIFF and AIFC files, compress 2:1
 *with squareroot-delta-exact
 **							encoding and write an AIFC file.
 *Copy all other chunks *
 *and encode samples at Markers as exacts for looping etc.
 **
 **	This File:		twotoone.h
 **
 ** Contains:		stuff
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
 **	 6/15/93	RDG		rorganized
 **	 3/10/93	RDG	    Version .11
 **						  -new  support for stereo
 *samples
 **						  -fix  sets the creator and type of the
 *new AIFC file *	 3/10/93	RDG		altered CompressSSND
 *prototype to pass CommChunk
 **	 3/10/93	RDG		newly factored out of twotoone.c
 **
 **************************************************************************************/
#ifndef __TWOTOONE_H__
#define __TWOTOONE_H__

#include "SquashSnd.h"
#include <math.h>

/*********************************************************************
 ** 		Public ANSI function prototypes
 *********************************************************************/
long CompressSSND (iff_control_ptr in_ptr, iff_control_ptr out_ptr,
                   ExtCommonChunk theCommChunk);

#endif /* __TWOTOONE_H__ */

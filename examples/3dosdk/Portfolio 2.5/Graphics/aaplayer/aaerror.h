#ifndef __AAERROR_H
#define __AAERROR_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: aaerror.h,v 1.2 1994/10/18 19:45:38 gyld Exp $
**
******************************************************************************/


#include "operror.h"

#define ER_AA				MakeErrId('A','a')

#define AALOADIMAGE_ERR		MAKETERR ( ER_AA, ER_SEVERE, ER_C_NSTND, 1 ) 
#define AALOADANIM_ERR		MAKETERR ( ER_AA, ER_SEVERE, ER_C_NSTND, 2 ) 
#define AANUMFRAMES_ERR		MAKETERR ( ER_AA, ER_SEVERE, ER_C_NSTND, 3 ) 
#define AAGRAPHICS_ERR		MAKETERR ( ER_AA, ER_SEVERE, ER_C_NSTND, 4 ) 
#define AAALLOC_ERR			MAKETERR ( ER_AA, ER_SEVERE, ER_C_NSTND, 5 ) 


			/*
			   The error text to print out with PrintfSysErr()
			*/
static char *AAErrors[] = {
	"no error",
	
	/* AALOADIMAGE_ERR */
	"Error in loading background image",
	
	/* AALOADANIM_ERR */
	"Error in loading anim or cel",
	
	/* AANUMFRAMES_ERR */
	"Loaded anim contains no frames",
	
	/* AAGRAPHICS_ERR */
	"Unable to open graphics",
	
	/* AAALLOC_ERR */
	"Unable to allocate appropriate memory",
		
};

#define AAMAX_ERROR_LEN 40


			/*
			   Tag args for the error text item
			*/
static TagArg AAErrorTags[] = {
	TAG_ITEM_NAME,		(void *) "aaplayer",
	ERRTEXT_TAG_OBJID,	(void *) ((ER_TASK << ERR_IDSIZE ) | ER_AA ),
	ERRTEXT_TAG_MAXERR,	(void *) (sizeof (AAErrors)/sizeof (char *)),
	ERRTEXT_TAG_TABLE,	(void *) AAErrors,
	ERRTEXT_TAG_MAXSTR,	(void *) AAMAX_ERROR_LEN,
	TAG_END,			0
};


#endif /* _AAERROR_H */

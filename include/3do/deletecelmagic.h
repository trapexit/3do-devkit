#ifndef __DELETECELMAGIC_H
#define __DELETECELMAGIC_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: deletecelmagic.h,v 1.3 1994/10/05 17:30:04 vertex Exp $
**
**  Lib3DO header to support the 'magic' DeleteCel system.
**
**  See the notes in CelUtils:DeleteCelMagic.c if you need to use this stuff.
**
**  This header file is generally not intended for application use.  It
**  supports the internals of the DeleteCel() system in Lib3DO.  A few rare
**  applications may need to implement their own support for DeleteCel() if
**  they have some special types of cels they create internally and for some
**  reason they can't use CreateCel() as a starting point.  (For example, if
**  you designed a special cel load process using the Join subscriber in
**  datastreaming, you might want to include DeleteCel() support in it.)
**
******************************************************************************/


#include "graphics.h"

enum {
	DELETECELMAGIC_CCB_ONLY		= 0x0de11CCB,	/* <- 'dellCCB' (sorta). cute, huh? */
	DELETECELMAGIC_DATA_ONLY,
	DELETECELMAGIC_CCB_AND_DATA,
	DELETECELMAGIC_CALLBACK
};

#ifdef __cplusplus
  extern "C" {
#endif

CCB *	AllocMagicCel_(uint32 extraBytes, uint32 freeMagic, void *freeData, void *creatorData);
void	ModifyMagicCel_(CCB *cel, uint32 freeMagic, void *freeData, void *creatorData);
void	FreeMagicCel_(CCB *cel);

#ifdef __cplusplus
  }
#endif


#endif /* __DELETECELMAGIC_H */

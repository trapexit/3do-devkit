#ifndef _MMU_H
#define _MMU_H

/*****

$Id: mmu.h,v 1.3 1992/10/24 01:41:07 dale Exp $

$Log: mmu.h,v $
 * Revision 1.3  1992/10/24  01:41:07  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1992, New Technologies Group, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#define MMU_SYSTEM	(1<<8)
#define MMU_BIGENDIAN	(1<<7)
#define MMU_LATEABRT	(1<<6)
#define MMU_32BITDATA	(1<<5)
#define MMU_32BITPRGM	(1<<4)
#define MMU_WRITEBUFFER	(1<<3)
#define MMU_CACHEON	(1<<2)
#define MMU_ADDRFLTON	(1<<1)
#define MMU_MMUON	(1<<0)

#define MMU_STANDARD	(MMU_BIGENDIAN|MMU_LATEABRT|MMU_32BITDATA \
			|MMU_32BITPRGM|MMU_SYSTEM)

#endif /* _MMU_H */

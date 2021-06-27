/* $Id: */

#ifndef __ST_H
#define __ST_H


/****************************************************************************/


/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/*****************************************************************************/


typedef struct STParms
{
    Item    stp_ScreenGroup;
    List   *stp_MemoryLists;
} STParms;


/*****************************************************************************/


#endif /* __ST_H */

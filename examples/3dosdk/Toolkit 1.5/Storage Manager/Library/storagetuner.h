#ifndef __STORAGETUNER_H
#define __STORAGETUNER_H

#pragma force_top_level
#pragma include_only_once


/****************************************************************************/


/* Copyright (C) 1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __LIST_H
#include "list.h"
#endif


/*****************************************************************************/


/* Error codes, !!! should be real system error definitions */
#define STORAGETUNER_ERR_BADTAG         -1
#define STORAGETUNER_ERR_BADSCREENCOUNT -2
#define STORAGETUNER_ERR_NOMEM          -3
#define STORAGETUNER_ERR_CANTLOADCELS   -4
#define STORAGETUNER_ERR_CANTLOADFONT   -5


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************/


Err StorageTunerRequest(Item screenGroup, List *memoryLists,
                        TagArg *args);


/*****************************************************************************/


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __STORAGETUNER_H */

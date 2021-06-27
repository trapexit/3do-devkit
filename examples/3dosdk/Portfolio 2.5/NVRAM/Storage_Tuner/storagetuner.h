#ifndef __STORAGETUNER_H
#define __STORAGETUNER_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: storagetuner.h,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/


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

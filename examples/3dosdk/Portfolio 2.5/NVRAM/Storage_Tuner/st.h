#ifndef __ST_H
#define __ST_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: st.h,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/

#ifndef __TYPES_H
#include "types.h"
#endif

/*****************************************************************************/

typedef struct STParms
{
  Item stp_ScreenGroup;
  List *stp_MemoryLists;
} STParms;

/*****************************************************************************/

#endif /* __ST_H */

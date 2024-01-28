#ifndef __TAGS_H
#define __TAGS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tags.h,v 1.2 1994/09/21 19:11:20 peabody Exp $
**
**  Tag management definitions
**
******************************************************************************/


#include "types.h"



#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


void DumpTagList(const TagArg *tagList, const char *desc);
TagArg *FindTagArg(const TagArg *tagList, uint32 tag);
TagData GetTagArg(const TagArg *tagList, uint32 tag, TagData defaultValue);
TagArg *NextTagArg(const TagArg **tagList);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif	/* __TAGS_H */

#pragma once
/* TagArgs - use to pass a list of arguments to functions */
#include "types_ints.h"

typedef	void *TagData;

typedef struct TagArg
{
  uint32  ta_Tag;
  TagData ta_Arg;
} TagArg, *TagArgP;

#define TAG_END	 0
#define TAG_JUMP 254
#define TAG_NOP	 255

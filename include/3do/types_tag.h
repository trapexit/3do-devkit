#ifndef TYPES_TAG_H_INCLUDED
#define TYPES_TAG_H_INCLUDED

/* TagArgs - use to pass a list of arguments to functions */
typedef	void *TagData;

typedef struct TagArg
{
  uint32  ta_Tag;
  TagData ta_Arg;
} TagArg, *TagArgP;

#define TAG_END	 0
#define TAG_JUMP 254
#define TAG_NOP	 255

#endif

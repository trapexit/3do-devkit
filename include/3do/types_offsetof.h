#pragma include_only_once

#include "types_size_t.h"

#define Offset(struct_type,field) ((size_t)&(((struct_type)NULL)->field))

#ifndef offsetof
#define offsetof(type,memb)	  ((size_t)&(((type *)NULL)->memb))
#endif

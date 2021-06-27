#ifndef TDO_TYPES_H_INCLUDED
#define TDO_TYPES_H_INCLUDED

#ifdef __cplusplus
#define	volatile
#endif /* __cplusplus */

#include "types_size_t.h"
#include "types_boolean.h"
#include "types_null.h"
#include "types_ints.h"
#include "types_tag.h"

typedef int32 Item;
typedef	int32 Err;

typedef void *CodeHandle;

/* Offset of a field in a structure */
#define Offset(struct_type,field) ((uint32)&(((struct_type)NULL)->field))

#ifndef offsetof
#define offsetof(type,memb)	  ((size_t)&(((type *)NULL)->memb))
#endif

#define KERNELSWI 0x10000

#endif

#pragma include_only_once

typedef unsigned char boolean;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef __cplusplus
typedef boolean bool;

#ifndef true
#define true TRUE
#endif

#ifndef false
#define false FALSE
#endif
#endif

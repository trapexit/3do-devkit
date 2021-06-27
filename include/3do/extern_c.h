#ifndef EXTERN_C_INCLUDED
#define EXTERN_C_INCLUDED

#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

#endif /* EXTERN_C_INCLUDED */

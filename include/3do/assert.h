#pragma force_top_level
#pragma once
/* Runtime and compile-time assertion interfaces. */

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(cond, msg) \
  typedef char static_assertion_##msg[(cond) ? 1 : -1]
#endif

#ifdef __cplusplus
extern "C" void __assert(const char *, const char *, int);
#else
extern void __assert(const char *, const char *, int);
#endif

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#ifdef __STDC__
#define assert(e) ((e) ? (void)0 : __assert(#e, __FILE__, __LINE__))
#else
#define assert(e) ((e) ? (void)0 : __assert("e", __FILE__, __LINE__))
#endif
#endif

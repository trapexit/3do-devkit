#pragma force_top_level
#pragma include_only_once

/*
 * C++ Library file new.h
 * (After Stroustrup, "The C++ Programming Language", section 3.2.6)
 * Copyright (C) Advanced RISC Machines Limited, 1993. All rights reserved.
 * Copyright (C) Codemist Limited, 1994.
 */

/*
 * RCS $Revision: 1.3 $
 * Checkin $Date: 1998/01/07 14:47:59 $
 * Revising $Author: sdouglas $
 */

#ifndef __STD_NEW
#ifndef __new_h
#define __new_h

#include <stddef.h>               /* for size_t */

typedef void (*new_handler)(void);

extern new_handler set_new_handler(new_handler);

#ifndef RWSTD_NO_NEW_DECL
inline void *operator new(size_t, void* ptr) { return ptr; }
inline void *operator new[](size_t, void* ptr) { return ptr; }
#endif

#endif
#endif

/* End of new.h */

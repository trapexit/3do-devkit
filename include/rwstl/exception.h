/*
 * C++ Library file exception
 * Copyright (C) Advanced RISC Machines Limited, 1997. All rights reserved.
 */

/*
 * RCS $Revision: 1.2 $
 * Checkin $Date: 1997/12/12 14:26:23 $
 * Revising $Author: swallis $
 */


#pragma force_top_level
#pragma include_only_once

#ifndef __exception_h
#define __exception_h

/* Standard library things */

#ifndef __RWSTD_EXCEPTION_SEEN

typedef void (*unexpected_handler)();
extern unexpected_handler set_unexpected(unexpected_handler f);
extern void unexpected(void);
typedef void (*terminate_handler)();
extern terminate_handler set_terminate(terminate_handler f);
extern void terminate();
extern bool uncaught_exception();

#endif /* #ifndef __RWSTD_EXCEPTION_SEEN */

#endif /* ifndef __exception_h */

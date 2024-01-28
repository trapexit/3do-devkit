#ifndef __TIME_H
#define __TIME_H

/*****

$Id: time.h,v 1.5 1994/01/21 02:30:46 limes Exp $

$Log: time.h,v $
 * Revision 1.5  1994/01/21  02:30:46  limes
 * recover from rcs bobble
 *
 * Revision 1.6  1994/01/20  02:14:30  sdas
 * C++ compatibility - updated
 *
 * Revision 1.5  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.4  1992/10/24  01:46:09  dale
 * rcsa
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

struct timeval {
        int32    tv_sec;         /* seconds */
        int32    tv_usec;        /* and microseconds */
};

#endif /* __TIME_H */

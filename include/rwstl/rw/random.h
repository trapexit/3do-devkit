/***************************************************************************
 *
 * random.h - Header for the Standard Library random generator
 *
 * $Id: random.h@#/main/sl1main/sl121main/0  02/01/97 21:37:01  smithey (SL121RA_UNIX)
 *
 ***************************************************************************
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 ***************************************************************************
 *
 * (c) Copyright 1994-1997 Rogue Wave Software, Inc.
 * ALL RIGHTS RESERVED
 *
 * The software and information contained herein are proprietary to, and
 * comprise valuable trade secrets of, Rogue Wave Software, Inc., which
 * intends to preserve as trade secrets such software and information.
 * This software is furnished pursuant to a written license agreement and
 * may be used, copied, transmitted, and stored only in accordance with
 * the terms of such license and with the inclusion of the above copyright
 * notice.  This software and information or any other copies thereof may
 * not be provided or otherwise made available to any other person.
 *
 * Notwithstanding any other lease or license that may pertain to, or
 * accompany the delivery of, this computer software and information, the
 * rights of the Government regarding its use, reproduction and disclosure
 * are as set forth in Section 52.227-19 of the FARS Computer
 * Software-Restricted Rights clause.
 * 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause at DFARS 252.227-7013.
 * Contractor/Manufacturer is Rogue Wave Software, Inc.,
 * P.O. Box 2328, Corvallis, Oregon 97339.
 *
 * This computer software and information is distributed with "restricted
 * rights."  Use, duplication or disclosure is subject to restrictions as
 * set forth in NASA FAR SUP 18-52.227-79 (April 1985) "Commercial
 * Computer Software-Restricted Rights (April 1985)."  If the Clause at
 * 18-52.227-74 "Rights in Data General" is specified in the contract,
 * then the "Alternate III" clause applies.
 *
 **************************************************************************/

#include <stdcomp.h>
#include "rw/stddefs.h"   
#ifdef RWSTD_MULTI_THREAD
#include "rw/stdmutex.h"
#endif

#ifndef RWSTD_NO_NAMESPACE
namespace __rwstd {
#endif

class RWSTDExport  __random_generator
{
  protected:

    enum { LENGTH = 55 };

    unsigned long table[LENGTH];
    size_t        index1;
    size_t        index2;
#ifdef RWSTD_MULTI_THREAD
    RWSTDMutex    mutex;
#endif

    void seed (unsigned long j);

  public:

    unsigned long operator() (unsigned long limit)
    {
#ifdef RWSTD_MULTI_THREAD
        RWSTDGuard guard(mutex);
#endif
        index1 = (index1 + 1) % LENGTH;
        index2 = (index2 + 1) % LENGTH;
        table[index1] = table[index1] - table[index2];
        return table[index1] % limit;
    }

    __random_generator (unsigned long j) { this->seed(j); }
   
};

#ifndef RWSTD_NO_NAMESPACE
}
#endif


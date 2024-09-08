/***************************************************************************
 *
 * iterator.cc - Non-inline definitions for the Standard Library iterators
 *
 * $Id: iterator.cc@#/main/sl1main/0  01/22/97 10:01:04  hinke (SL121RA_UNIX)
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

#ifndef _RWSTD_NO_NAMESPACE
namespace std {
#endif

template <class InputIterator, class Distance>
void __distance (InputIterator first, InputIterator last, Distance& n, 
                 input_iterator_tag)
{
    while (first != last) { ++first; ++n; }
}

template <class ForwardIterator, class Distance>
void __distance (ForwardIterator first, ForwardIterator last, Distance& n, 
                 forward_iterator_tag)
{
    while (first != last) { ++first; ++n; }
}

template <class BidirectionalIterator, class Distance>
void __distance (BidirectionalIterator first, BidirectionalIterator last, 
                 Distance& n, bidirectional_iterator_tag)
{
    while (first != last) { ++first; ++n; }
}


#ifdef _RWSTD_NO_BASE_CLASS_MATCH
//
// We include assert() to test for possible problem in advance().
// Furthermore, we FORCE assert() to always expand.
//
#ifdef  NDEBUG
#define __RW_NDEBUG
#undef  NDEBUG
#endif
#ifndef _RWSTD_NO_NEW_HEADER
#include <cassert>
#else
#include <assert.h>
#endif

#endif /*_RWSTD_NO_BASE_CLASS_MATCH*/


template <class InputIterator, class Distance>
void __advance (InputIterator& i, Distance n, input_iterator_tag)
{
#ifdef _RWSTD_NO_BASE_CLASS_MATCH
    //
    // All uses of advance() end up calling this template, even
    // when advance() is being invoked on a bidirectional or random
    // iterator.  We need to check that n is non-negative, or else
    // this algorithm will fail horribly.  We MUST document the
    // restriction that advance() only be called with non-negative
    // Distance.  There don't appear to be any _EXPLICIT uses of advance()
    // with a negative Distance argument in the STL library itself.
    //
    // This assert() is ALWAYS on -- see how it's included'd above.
    //
    assert(n >= 0);
#endif /*_RWSTD_NO_BASE_CLASS_MATCH*/
    while (n--) ++i;
}

//
// Don't forget to turn off expansion of assert() if that's what the
// user expects.
//
#ifdef  __RW_NDEBUG
#define NDEBUG
#undef  __RW_NDEBUG
#endif

template <class ForwardIterator, class Distance>
void __advance (ForwardIterator& i, Distance n, forward_iterator_tag)
{
    while (n--) ++i;
}

template <class BidirectionalIterator, class Distance>
void __advance (BidirectionalIterator& i, Distance n, 
                bidirectional_iterator_tag)
{
    if (n >= 1)
        while (n--) ++i;
    else
        while (n++) --i;
}



#ifndef _RWSTD_NO_NAMESPACE
}
#endif

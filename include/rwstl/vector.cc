/***************************************************************************
 *
 * vector.cc - Non-inline definitions for the Standard Library vector class
 *
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

#ifndef RWSTD_NO_NAMESPACE
namespace std {
#endif


//
// This requires that T have a default constructor.
//

template <class T, class Allocator>
void vector<T,Allocator>::resize (size_type new_size)
{
    T value;
    if (new_size > size())
        insert(end(), new_size - size(), value);
    else if (new_size < size())
        erase(begin() + new_size, end());
}

template <class T, class Allocator>
void vector<T,Allocator>::resize (size_type new_size, T value)
{
    if (new_size > size())
        insert(end(), new_size - size(), value);
    else if (new_size < size())
        erase(begin() + new_size, end());
}

template <class T, class Allocator>
vector<T,Allocator>& vector<T,Allocator>::operator= (const vector<T,Allocator>& x)
{
    if (&x == this) return *this;
    if (x.size() > capacity())
    {
        destroy(start, finish);
        RWSTD_VECTOR_VALUE_ALLOCATOR.deallocate(start);
        start = RWSTD_VECTOR_VALUE_ALLOCATOR.allocate(x.end() - x.begin(),(pointer)0);
        end_of_storage = uninitialized_copy(x.begin(), x.end(), start);
    }
    else if (size() >= x.size())
    {
        iterator i = copy(x.begin(), x.end(), begin());
        destroy(i, finish);
    }
    else
    {
        copy(x.begin(), x.begin() + size(), begin());
        uninitialized_copy(x.begin() + size(), x.end(), begin() + size());
    }
    finish = begin() + x.size();
    return *this;
}

template <class T, class Allocator>
void vector<T,Allocator>::insert_aux (iterator position, const T& x)
{
    if (finish != end_of_storage)
    {
        RWSTD_VECTOR_VALUE_ALLOCATOR.construct(finish, *(finish - 1));
        copy_backward(position, finish - 1, finish);
        *position = x;
        ++finish;
    }
    else
    {
        //
        // We always allocate enough space for a number of additional
        // elements in the vector, unless the size of each element is
        // very large.
        //
        size_type len = size() + buffer_size;
        iterator tmp = RWSTD_VECTOR_VALUE_ALLOCATOR.allocate(len,start);
        uninitialized_copy(begin(), position, tmp);
        RWSTD_VECTOR_VALUE_ALLOCATOR.construct((tmp + (position - begin())), x);
        uninitialized_copy(position, end(), tmp + (position - begin()) + 1); 
        destroy(begin(), end());
        RWSTD_VECTOR_VALUE_ALLOCATOR.deallocate(begin());
        end_of_storage = tmp + len;
        finish = tmp + size() + 1;
        start = tmp;
    }
}

template <class T, class Allocator>
void vector<T,Allocator>::insert (iterator position, size_type n, const T& x)
{
    if (n == 0) return;
    if (end_of_storage - finish >= n)
    {
        if (end() - position > n)
        {
            uninitialized_copy(end() - n, end(), end());
            copy_backward(position, end() - n, end());
            fill(position, position + n, x);
        }
        else
        {
            uninitialized_copy(position, end(), position + n);
            fill(position, end(), x);
            uninitialized_fill_n(end(), n - (end() - position), x);
        }
        finish += n;
    }
    else
    {
        size_type len = size() + max(size(), n);
        iterator tmp = RWSTD_VECTOR_VALUE_ALLOCATOR.allocate(len,start);
        uninitialized_copy(begin(), position, tmp);
        uninitialized_fill_n(tmp + (position - begin()), n, x);
        uninitialized_copy(position, end(), tmp + (position - begin() + n));
        destroy(begin(), end());
        RWSTD_VECTOR_VALUE_ALLOCATOR.deallocate(begin());
        end_of_storage = tmp + len;
        finish = tmp + size() + n;
        start = tmp;
    }
}

#ifndef RWSTD_NO_MEMBER_TEMPLATES
template<class T, class Allocator>
template<class InputIterator>
void vector<T,Allocator>::insert (iterator position, InputIterator first, 
                        InputIterator last)
#else
template<class T, class Allocator>
void vector<T,Allocator>::insert (iterator position, const_iterator first,
                        const_iterator last)
#endif
{
    if (first == last) return;
    size_type n;
    __initialize(n, size_type(0));
    distance(first, last, n);
    if (end_of_storage - finish >= n)
    {
        if (end() - position > n)
        {
            uninitialized_copy(end() - n, end(), end());
            copy_backward(position, end() - n, end());
            copy(first, last, position);
        }
        else
        {
            uninitialized_copy(position, end(), position + n);
            copy(first, first + (end() - position), position);
            uninitialized_copy(first + (end() - position), last, end());
        }
        finish += n;
    }
    else
    {
        size_type len = size() + max(size(), n);
        iterator tmp = RWSTD_VECTOR_VALUE_ALLOCATOR.allocate(len,start);
        uninitialized_copy(begin(), position, tmp);
        uninitialized_copy(first, last, tmp + (position - begin()));
        uninitialized_copy(position, end(), tmp + (position - begin() + n));
        destroy(begin(), end());
        RWSTD_VECTOR_VALUE_ALLOCATOR.deallocate(begin());
        end_of_storage = tmp + len;
        finish = tmp + size() + n;
        start = tmp;
    }
}

#ifndef RWSTD_NO_NAMESPACE
}
#endif


/***************************************************************************
 *
 * list.cc - Non-nline list definitions for the Standard Library
 *
 * $Id: list.cc@#/main/sl1main/sl121main/0  02/11/97 15:02:39  sree (SL121RA_UNIX)
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

#define RWSTD_LIST_SORT_COUNTERS 64

#ifndef RWSTD_NO_NAMESPACE
namespace std {
#endif

template <class T, class Allocator>
void list<T,Allocator>::deallocate_buffers ()
{
    while (buffer_list)
    {
        buffer_pointer tmp = buffer_list;
        buffer_list = (buffer_pointer)(buffer_list->next_buffer);
#ifdef RWSTD_ALLOCATOR
        the_allocator.deallocate(tmp->buffer);
        the_allocator.deallocate(tmp);
#else
        list_node_allocator.deallocate(tmp->buffer);
        buffer_allocator.deallocate(tmp);
#endif
    }
    free_list = 0;
    next_avail = 0;
    last = 0;
}

//
// This requires that T have a default constructor.
//

template <class T, class Allocator>
void list<T,Allocator>::resize (size_type new_size)
{
    T value;
    if (new_size > size())
        insert(end(), new_size - size(), value);
    else if (new_size < size())
    {
        iterator tmp = begin();
        advance(tmp, (difference_type) new_size);
        erase(tmp, end());
    }
}

template <class T, class Allocator>
void list<T,Allocator>::resize (size_type new_size, T value)
{
    if (new_size > size())
        insert(end(), new_size - size(), value);
    else if (new_size < size())
    {
        iterator tmp = begin();
        advance(tmp, (difference_type) new_size);
        erase(tmp, end());
    }
}

#ifndef RWSTD_NO_MEMBER_TEMPLATES
template <class T, class Allocator>   
template<class InputIterator>
void list<T,Allocator>::insert (iterator position, InputIterator first,
                      InputIterator locallast)
{
    while (first != locallast) insert(position, *first++);
}
#else
template <class T, class Allocator>   
void list<T, Allocator>::insert (iterator position, const_iterator first,
                      const_iterator locallast)
{
    while (first != locallast) insert(position, *first++);
}
template <class T, class Allocator>
void list<T, Allocator>::insert (iterator position, const T* first, const T* locallast)
{
    while (first != locallast) insert(position, *first++);
}
#endif

template <class T, class Allocator>
void list<T, Allocator>::insert (iterator position, size_type n, const T& x)
{
    while (n--) insert(position, x);
}

template <class T, class Allocator>
typename list<T, Allocator>::iterator 
list<T, Allocator>::erase (iterator first, iterator locallast)
{
    iterator tmp = end();
    while (first != locallast) 
    {
      tmp = erase(first++);
    }
    return tmp;
}

template <class T, class Allocator>
list<T, Allocator>& list<T, Allocator>::operator= (const list<T, Allocator>& x)
{
    if (this != &x)
    {
        iterator first1 = begin();
        iterator last1 = end();
        const_iterator first2 = x.begin();
        const_iterator last2 = x.end();
        while (first1 != last1 && first2 != last2) *first1++ = *first2++;
        if (first2 == last2)
            erase(first1, last1);
        else
            insert(last1, first2, last2);
    }
    return *this;
}

template <class T, class Allocator>
void list<T, Allocator>::remove (const T& value)
{
    iterator _first = begin();
    iterator _last = end();
    while (_first != _last)
    {
        iterator next = _first;
        ++next;
        if (*_first == value) erase(_first);
        _first = next;
    }
}

template <class T, class Allocator>
void list<T, Allocator>::unique ()
{
    iterator _first = begin();
    iterator _last = end();
    if (_first == _last) return;
    iterator next = _first;
    while (++next != _last)
    {
        if (*_first == *next)
            erase(next);
        else
            _first = next;
        next = _first;
    }
}

template <class T, class Allocator>
void list<T, Allocator>::merge (list<T, Allocator>& x)
{
    iterator first1 = begin();
    iterator last1 = end();
    iterator first2 = x.begin();
    iterator last2 = x.end();
    while (first1 != last1 && first2 != last2)
    {
        if (*first2 < *first1)
        {
            iterator next = first2;
            transfer(first1, first2, ++next, x);
            first2 = next;
        }
        else
            first1++;
    }
    if (first2 != last2) 
        transfer(last1, first2, last2, x);
}

template <class T, class Allocator>
void list<T, Allocator>::reverse ()
{
    if (size() < 2) return;
    for (iterator first = ++begin(); first != end();)
    {
        iterator old = first++;
        transfer(begin(), old, first, *this);
    }
}    


template <class T, class Allocator>
void list<T, Allocator>::sort ()
{
    if (size() < 2) return;

    list<T, Allocator> carry(get_allocator());
    list<T, Allocator> counter[RWSTD_LIST_SORT_COUNTERS];
    for (int i = 0; i < RWSTD_LIST_SORT_COUNTERS; i++)
      counter[i].set_allocator(get_allocator());

    int fill = 0;
    while (!empty())
    {
        carry.splice(carry.begin(), *this, begin());

        int i = 0;
        while (i < fill && !counter[i].empty())
        {
            counter[i].merge(carry);
            carry.swap(counter[i++]);
        }
        carry.swap(counter[i]);         
        if (i == fill) ++fill;
    } 
    while (fill--) 
    {  
      merge(counter[fill]);
    }
}

#ifndef RWSTD_NO_MEMBER_TEMPLATES
template<class T, class Allocator>
template<class Predicate> void list<T, Allocator>::remove_if (Predicate pred)
{
    iterator first = begin();
    iterator last = end();
    while (first != last)
    {
        iterator next = first;
        ++next;
        if (pred(*first)) erase(first);
        first = next;
    }
}

template<class T, class Allocator>
template<class BinaryPredicate> void list<T, Allocator>::unique (BinaryPredicate pred)
{
    iterator first = begin();
    iterator last = end();
    if (first == last) return;
    iterator next = first;
    while (++next != last)
    {
        if (pred(*first, *next))
            erase(next);
        else
            first = next;
        next = first;
    }
}

template<class T, class Allocator>
template<class Compare> void list<T, Allocator>::merge (list<T, Allocator>& x, Compare comp)
{
    iterator first1 = begin();
    iterator last1  = end();
    iterator first2 = x.begin();
    iterator last2  = x.end();
    while (first1 != last1 && first2 != last2)
    {
        if (comp(*first2, *first1))
        {
            iterator next = first2;
            transfer(first1, first2, ++next, x);
            first2 = next;
        }
        else
            first1++;
    }
    if (first2 != last2) 
        transfer(last1, first2, last2, x);
}



template<class T, class Allocator>
template<class Compare> void list<T, Allocator>::sort (Compare comp)
{
    if (size() < 2) return;

    list<T, Allocator> carry(get_allocator());
    list<T, Allocator> counter[ RWSTD_LIST_SORT_COUNTERS];
    for (int i = 0; i < RWSTD_LIST_SORT_COUNTERS; i++)
      counter[i].set_allocator(get_allocator());
    int fill = 0;
    while (!empty())
    {
        carry.splice(carry.begin(), *this, begin());
        int i = 0;
        while (i < fill && !counter[i].empty())
        {
            counter[i].merge(carry, comp);
            carry.swap(counter[i++]);
        }
        carry.swap(counter[i]);         
        if (i == fill) ++fill;
    } 
    while (fill--) merge(counter[fill]);
}
#endif /*RWSTD_NO_MEMBER_TEMPLATES*/

#undef RWSTD_LIST_SORT_COUNTERS

#ifndef RWSTD_NO_NAMESPACE
}
#endif

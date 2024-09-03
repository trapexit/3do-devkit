
/***************************************************************************
 *
 * tree.cc - Non-nline tree definitions for the Standard Library
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

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::deallocate_buffers ()
  {
    while (buffer_list)
    {
      buffer_pointer tmp = buffer_list;
      buffer_list        = (buffer_pointer)(buffer_list->next_buffer);
      RWSTD_TREE_NODE_ALLOCATOR.deallocate(tmp->buffer);
      RWSTD_TREE_BUFFER_ALLOCATOR.deallocate(tmp);
    }
  }


  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>& 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::
  operator= (const rb_tree<Key, Value, KeyOfValue, Compare, Allocator>& x)
  {
    if (!(this == &x))
    {
      //
      // Can't be done as in list because Key may be a constant type.
      //
      erase(begin(), end());
      root() = __copy(x.root(), header);
      if (isNil(root()))
      {
        leftmost()  = header; rightmost() = header;
      }
      else
      {
        leftmost()  = minimum(root()); rightmost() = maximum(root());
      }
      node_count = x.node_count;
    }
    return *this;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::
  __insert (link_type x, link_type y, const Value& v)
  {
    ++node_count;
    link_type z = get_node(v);

    if (y == header || !isNil(x) || key_compare(KeyOfValue()(v), key(y)))
    {
      left(y) = z;  // Also makes leftmost() = z when y == header.
      if (y == header)
      {
        root() = z; rightmost() = z;
      }
      else if (y == leftmost())
        leftmost() = z;   // Maintain leftmost() pointing to minimum node.
    }
    else
    {
      right(y) = z;
      if (y == rightmost())
        rightmost() = z;  // Maintain rightmost() pointing to maximum node.
    }
    parent(z) = y;
    x = z;  // Recolor and rebalance the tree.
    while (x != root() && color(parent(x)) == rb_red) 
      if (parent(x) == left(parent(parent(x))))
      {
        y = right(parent(parent(x)));
        if (!isNil(y) && color(y) == rb_red)
        {
          color(parent(x))         = rb_black;
          color(y)                 = rb_black;
          color(parent(parent(x))) = rb_red;
          x                        = parent(parent(x));
        }
        else
        {
          if (x == right(parent(x)))
          {
            x = parent(x); 
            rotate_left(x);
          }
          color(parent(x)) = rb_black;
          color(parent(parent(x))) = rb_red;
          rotate_right(parent(parent(x)));
        }
      }
      else
      {
        y = left(parent(parent(x)));
        if (!isNil(y) && color(y) == rb_red)
        {
          color(parent(x))         = rb_black;
          color(y)                 = rb_black;
          color(parent(parent(x))) = rb_red;
          x                        = parent(parent(x));
        }
        else
        {
          if (x == left(parent(x)))
          {
            x = parent(x); 
            rotate_right(x);
          }
          color(parent(x))         = rb_black;
          color(parent(parent(x))) = rb_red;
          rotate_left(parent(parent(x)));
        }
      }
    color(root()) = rb_black;
    return iterator(z);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::pair_iterator_bool
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::insert (const Value& v)
  {
    link_type y = header;
    link_type x = root();
    bool RWSTD_COMP   = true;
    while (!isNil(x))
    {
      y    = x;
      RWSTD_COMP = key_compare(KeyOfValue()(v), key(x));
      x    = RWSTD_COMP ? left(x) : right(x);
    }
    if (insert_always)
    {
      pair_iterator_bool tmp(__insert(x, y, v), true); return tmp;
    }
    iterator j = iterator(y);   
    if (RWSTD_COMP)
    {
      if (j == begin())  
      {
        pair_iterator_bool tmp(__insert(x, y, v), true); return tmp;
      }
      else
        --j;
    }
    if (key_compare(key(j.node), KeyOfValue()(v)))
    {
      pair_iterator_bool tmp(__insert(x, y, v), true); return tmp;
    }
    pair_iterator_bool tmp(j, false);
    return tmp;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::insert (iterator position,
                                                               const Value& v)
  {
    if (position == iterator(begin()))
    {
      if (size() > 0 && key_compare(KeyOfValue()(v), key(position.node)))
        return __insert(position.node, position.node, v);
      //
      // First argument just needs to be non-NIL.
      //
      else
        return insert(v).first;
    }
    else if (position == iterator(end()))
    {
      if (key_compare(key(rightmost()), KeyOfValue()(v)))
        return __insert(__nil(), rightmost(), v);
      else
        return insert(v).first;
    }
    else
    {
      iterator before = --position;
      if (key_compare(key(before.node), KeyOfValue()(v))
          && key_compare(KeyOfValue()(v), key(position.node)))
      {
        if (isNil(right(before.node)))
          return __insert(__nil(), before.node, v); 
        else
          return __insert(position.node, position.node, v);
        //
        // First argument just needs to be non-NIL.
        //
      }
      else
        return insert(v).first;
    }
  }

#ifndef RWSTD_NO_MEMBER_TEMPLATES
  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  template<class Iterator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::insert (Iterator _first, 
                                                                    Iterator _last)
  {
    while (_first != _last) insert(*_first++);
  }
#else
  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::insert (const_iterator _first, 
                                                                    const_iterator _last)
  {
    while (_first != _last) insert(*_first++);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::insert (const Value* _first, 
                                                                    const Value* _last)
  {
    while (_first != _last) insert(*_first++);
  }
#endif
         
  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::erase (iterator position)
  {
    iterator tmp = position; // Retain 'next' position for return
    if (tmp != end())
      tmp++;

    link_type z;
    __initialize(z, link_type(position.node));
    link_type y = z;
    link_type x;
    bool __deleted = false;
    if (isNil(left(y)))
    {
      if (isNil(right(y)))
      {
        x = parent(y);
        __erase_leaf(y);
        __deleted = true;
      }
      else
        x = right(y); 
    }
    else
    {
      if (isNil(right(y))) 
        x = left(y);
      else
      {
        y = right(y);
        while (!isNil(left(y))) y = left(y);
        x = right(y);
      }
    }
    if (!__deleted && y != z)
    {
      //
      // Relink y in place of z.
      //
      parent(left(z)) = y; 
      left(y) = left(z);
      if (y != right(z))
      {
        if (!isNil(x))
          parent(x)        = parent(y); 
        left(parent(y))  = x;         // Y must be a left child.
        right(y)         = right(z);
        parent(right(z)) = y;
      }
      else if (!isNil(x))
        parent(x) = y;  

      if (root() == z)
        root() = y;
      else if (left(parent(z)) == z)
        left(parent(z)) = y;
      else 
        right(parent(z)) = y;

      parent(y) = parent(z);
      if (isNil(x))
        x = y;       // Don't want to start balancing with nil
#ifndef RWSTD_NO_NAMESPACE
      std::swap(color(y), color(z));
#else
      ::swap(color(y), color(z));
#endif
      y = z;         // y points to node to be actually deleted.
    }
    else if (!__deleted)
    {
      //
      // y == z
      //
      parent(x) = parent(y);   
      if (root() == z)
        root() = x;
      else 
      {
        if (left(parent(z)) == z)
          left(parent(z)) = x;
        else
          right(parent(z)) = x;
      }
      if (leftmost() == z) 
      {
        if (isNil(right(z)))     // left(z) must be nil also.
          leftmost() = parent(z);
        else
          leftmost() = minimum(x);
      }
      if (rightmost() == z)  
      {
        if (isNil(left(z)))     // right(z) must be nil also.
          rightmost() = parent(z);
        else
          rightmost() = maximum(x);
      }
    }
    if (x != header && color(y) != rb_red)
    { 
      while (x != root() && color(x) == rb_black)
      {
        if (x == left(parent(x)))
        {
          link_type w = right(parent(x));
          if (isNil(w))
            x = parent(x);
          else
          {
            if (color(w) == rb_red)
            {
              color(w)         = rb_black;
              color(parent(x)) = rb_red;
              rotate_left(parent(x));
              w = right(parent(x));
            }
            if ((isNil(left(w)) || color(left(w)) == rb_black) && 
                (isNil(right(w)) || color(right(w)) == rb_black))
            {
              color(w) = rb_red;
              x = parent(x);
            }
            else
            {
              if (!isNil(right(w)) && color(right(w)) == rb_black)
              {
                if (!isNil(left(w)))
                  color(left(w)) = rb_black;
                color(w)       = rb_red;
                rotate_right(w);
                w = right(parent(x));
              }
              color(w) = color(parent(x));
              color(parent(x)) = rb_black;
              if (!isNil(right(w)))
                color(right(w))  = rb_black;
              rotate_left(parent(x));
              break;
            }
          }
        }
        else
        {
          //
          // Same as then clause with "right" and "left" exchanged.
          //
          link_type w = left(parent(x));
          if (isNil(w))
            x = parent(x);
          else
          {
            if (color(w) == rb_red)
            {
              color(w)         = rb_black;
              color(parent(x)) = rb_red;
              rotate_right(parent(x));
              w = left(parent(x));
            }
            if ((isNil(right(w)) || color(right(w)) == rb_black) &&
                (isNil(left(w)) || color(left(w)) == rb_black))
            {
              color(w) = rb_red; x = parent(x);
            }
            else
            {
              if (!isNil(left(w)) && color(left(w)) == rb_black)
              {
                if (!isNil(right(w)))
                  color(right(w)) = rb_black;
                color(w)        = rb_red;
                rotate_left(w);
                w = left(parent(x));
              }
              color(w) = color(parent(x));
              color(parent(x)) = rb_black;
              if (!isNil(left(w)))
                color(left(w))   = rb_black;
              rotate_right(parent(x));
              break;
            }
          }
        }
      }
      color(x) = rb_black;
    }
    put_node(y);
    --node_count;
    return tmp;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::size_type 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::erase (const Key& x)
  {
    pair_iterator_iterator p = equal_range(x);
    size_type n;
    __initialize(n, size_type(0));
    distance(p.first, p.second, n);
    erase(p.first, p.second);
    return n;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::link_type 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::__copy (link_type x, link_type p)
  {
    //
    // Structural copy.
    // 
    link_type r = x;
    while (!isNil(x))
    {
      link_type y = get_node(value(x));
      if (r == x) r = y;  // Save for return value.
      left(p)   = y;
      parent(y) = p;
      color(y)  = color(x);
      right(y)  = __copy(right(x), y);
      p         = y;
      x         = left(x);
    }
    left(p) = __nil();
    return r;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::__erase (link_type x)
  {
    //
    // Erase without rebalancing.
    //
    while (!isNil(x))
    {
      __erase(right(x));
      link_type y = left(x);
      put_node(x);
      x = y;
    }
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::erase (iterator _first, 
                                                              iterator locallast)
  {
    iterator tmp = end();
    if (_first == begin() && locallast == end() && node_count != 0)
    {
      __erase(root());
      leftmost()  = header;
      root()      = __nil();
      rightmost() = header;
      node_count  = 0;
      tmp = end();
    } else
    {
      while (_first != locallast) 
        tmp =  erase(_first++);
    }
    return tmp;
  }

  template <class Key, class Value, class KeyOfValue, class Compare, class Allocator>
  void rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::erase (const Key* _first, 
                                                                   const Key* _last)
  {
    while (_first != _last) erase(*_first++);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::find (const Key& k)
  {
    link_type y = header;  // Last node that is not less than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (!key_compare(key(x), k))
        y = x, x = left(x);
      else
        x = right(x);
    }
    iterator j = iterator(y);
    return (j == end() || key_compare(k, key(j.node))) ? end() : j;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::const_iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::find (const Key& k) const
  {
    link_type y = header;  // Last node that is not less than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (!key_compare(key(x), k))
        y = x, x = left(x);
      else
        x = right(x);
    }
    const_iterator j = const_iterator(y);   
    return (j == end() || key_compare(k, key(j.node))) ? end() : j;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::size_type 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::count (const Key& k) const
  {
    pair<const_iterator, const_iterator> p = equal_range(k);
    size_type n;
    __initialize(n, size_type(0));
    distance(p.first, p.second, n);
    return n;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::lower_bound (const Key& k)
  {
    link_type y = header;  // Last node that is not less than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (!key_compare(key(x), k))
        y = x, x = left(x);
      else
        x = right(x);
    }

    return iterator(y);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::const_iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::lower_bound (const Key& k) const
  {
    link_type y = header;  // Last node that is not less than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (!key_compare(key(x), k))
        y = x, x = left(x);
      else
        x = right(x);
    }

    return const_iterator(y);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::upper_bound (const Key& k)
  {
    link_type y = header;  // Last node that is greater than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (key_compare(k, key(x)))
        y = x, x = left(x);
      else
        x = right(x);
    }

    return iterator(y);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::const_iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::upper_bound (const Key& k) const
  {
    link_type y = header;  // Last node that is greater than k.
    link_type x = root();  // Current node.

    while (!isNil(x))
    {
      if (key_compare(k, key(x)))
        y = x, x = left(x);
      else
        x = right(x);
    }

    return const_iterator(y);
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::pair_iterator_iterator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::equal_range (const Key& k)
  {
    pair_iterator_iterator tmp(lower_bound(k), upper_bound(k));
    return tmp;
  }

  template <class Key, class Value, class KeyOfValue, 
  class Compare, class Allocator>
  typename rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::pair_citerator_citerator 
  rb_tree<Key, Value, KeyOfValue, Compare, Allocator>::equal_range (const Key& k) const
  {
    pair_citerator_citerator tmp(lower_bound(k), upper_bound(k));
    return tmp;
  }


#ifndef RWSTD_NO_NAMESPACE
}
#endif

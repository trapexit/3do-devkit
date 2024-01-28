/*******************************************************************************************
 *	File:			TypeID.h
 *
 *	Contains:		Type identifier for a file or chunk.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/26/94		crm		Got rid of TInternalTypeID
 *	8/21/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __TYPEID__
#define __TYPEID__

class TTypeID
{
public:
  static const unsigned long WILDCARD;
  // value matches any other type identifier

  TTypeID ();
  TTypeID (unsigned long);
  TTypeID (char, char, char, char);
  TTypeID (const TTypeID &);
  ~TTypeID ();

  operator unsigned long () const;
  // Cast this type identifier to an unsigned long integer

  TTypeID &operator= (const TTypeID &);
  // Assign this type identifier from another

  Boolean operator== (const TTypeID &) const;
  Boolean operator!= (const TTypeID &) const;
  // Test this type identifier for equality

  ostream &operator>>= (ostream &) const;
  // Flatten this type identifier to the specified stream

  istream &operator<<= (istream &);
  // Unflatten this type identifier from the specified stream

  friend ostream &operator<< (ostream &, const TTypeID &);
  // Print this type identifier to the specified character stream

private:
  union
  {
    unsigned long typeID;
    unsigned char typeIDChars[sizeof (unsigned long)];
  };
};

#endif

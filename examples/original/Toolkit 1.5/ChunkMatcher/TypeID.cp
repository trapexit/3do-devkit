/*******************************************************************************************
 *	File:			TypeID.cp
 *
 *	Contains:		Type identifier for a file or chunk.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/3/94		crm		Text file stream operator <<
 *substitutes blanks for null bytes in type identifier. 9/26/94 crm
 *Got rid of TInternalTypeID 8/21/94		crm		New today
 *
 *******************************************************************************************/

#include "TypeID.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

const unsigned long TTypeID::WILDCARD = 0x2A2A2A2A; // '****'

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/***********
 * TTypeID
 ***********
 * Null constructor
 */

TTypeID::TTypeID () { this->typeID = 0; }

/***********
 * TTypeID
 ***********
 * Constructor
 */

TTypeID::TTypeID (unsigned long typeSignature)

{
  this->typeID = typeSignature;
}

/***********
 * TTypeID
 ***********
 * Constructor
 */

TTypeID::TTypeID (char c0, char c1, char c2, char c3)

{
  this->typeIDChars[0] = c0;
  this->typeIDChars[1] = c1;
  this->typeIDChars[2] = c2;
  this->typeIDChars[3] = c3;
}

/***********
 * TTypeID
 ***********
 * Copying constructor
 */

TTypeID::TTypeID (const TTypeID &copy) { this->typeID = copy.typeID; }

/************
 * ~TTypeID
 ************
 * Destructor
 */

TTypeID::~TTypeID () {}

/**************************
 * TTypeID
 **************************
 * operator unsigned long
 **************************
 * Cast this type identifier to an unsigned
 * long quantity.
 */

TTypeID::operator unsigned long () const { return this->typeID; }

/**************
 * TTypeID
 **************
 * operator =
 **************
 * Take the value of this object from that of
 * the specified object.  Result is a reference
 * to this object.
 */

TTypeID &
TTypeID::operator= (const TTypeID &assign)

{
  if (this != &assign)
    this->typeID = assign.typeID;

  return *this;
}

/***************
 * TTypeID
 ***************
 * operator ==
 ***************
 * Test for equality.
 */

Boolean
TTypeID::operator== (const TTypeID &operand) const

{
  if (this->typeID == operand.typeID)
    return true;

  if (this->typeID == WILDCARD || operand.typeID == WILDCARD)
    return true;

  return false;
}

/***************
 * TTypeID
 ***************
 * operator !=
 ***************
 * Negated test for equality.
 */

Boolean
TTypeID::operator!= (const TTypeID &operand) const

{
  if (this->typeID == WILDCARD || operand.typeID == WILDCARD)
    return false;

  if (this->typeID != operand.typeID)
    return true;

  return false;
}

/***************
 * operator <<
 ***************
 * Print the state of the specified chunk type to the
 * specified character stream.
 */

ostream &
operator<< (ostream &stream, const TTypeID &that)

{
  char c; // next character in type identifier

  // Print null bytes in identifier as blanks

  for (int i = 0; i < sizeof (unsigned long); ++i)
    {
      c = that.typeIDChars[i];
      stream << (c == '\0' ? ' ' : c);
    }

  return stream;
}

/****************
 * TTypeID
 ****************
 * operator <<=
 ****************
 * Flatten this type identifier to the specified stream.
 */

ostream &
TTypeID::operator>>= (ostream &stream) const

{
  stream.write (this->typeIDChars, sizeof (unsigned long));

  return stream;
}

/****************
 * TTypeID
 ****************
 * operator >>=
 ****************
 * Unflatten this type identifier from the specified stream.
 */

istream &
TTypeID::operator<<= (istream &stream)

{
  stream.read (this->typeIDChars, sizeof (unsigned long));

  return stream;
}

/*******************************************************************************************
 *	File:			Chunk.cp
 *
 *	Contains:		Base class for different kinds of chunks.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/26/94		crm		Got rid of TSize data type
 *	6/16/94		crm		New today
 *
 *******************************************************************************************/

#include "Chunk.h"
#include "ChunkFileFormat.h"
#include "ChunkTypes.h"
#include "TypeID.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/**********
 * TChunk
 **********
 * Null constructor
 * Creates an empty chunk
 */

TChunk::TChunk () { this->rawChunk = NULL; }

TChunk::TChunk (ChunkHeader *rawChunkPtr) { this->rawChunk = rawChunkPtr; }

/**********
 * TChunk
 **********
 * Copying constructor
 */

TChunk::TChunk (const TChunk &that) { this->Copy (that); }

/**********
 * TChunk
 **********
 * Constructor
 * Called from derived classes
 */

TChunk::TChunk (uint32 rawType, uint32 rawSize)

{
  // Allocate storage for the raw chunk

  this->rawChunk = (ChunkHeader *)new char[rawSize];

  // Clear the bytes of the raw chunk and set the
  // header fields

  if (this->rawChunk != NULL)
    {
      memset (this->rawChunk, 0, (size_t)rawSize);
      this->rawChunk->type = rawType;
      this->rawChunk->size = rawSize;
    }
}

/***********
 * ~TChunk
 ***********
 * Destructor
 */

TChunk::~TChunk ()

{
  delete[](char *) this->rawChunk;
  this->rawChunk = NULL;
}

/**************
 * TChunk
 **************
 * operator =
 **************
 * Take the value of this object from that of
 * the specified object.  Result is a reference
 * to this object.
 */

TChunk &
TChunk::operator= (const TChunk &that)

{
  if (this != &that)
    // Copy from the right-hand side
    this->Copy (that);

  return *this;
}

/***********
 * TChunk
 ***********
 * GetSize
 ***********
 * Result is the total size of the header and
 * data parts of this chunk.
 */

uint32
TChunk::GetSize () const

{
  if (this->rawChunk == NULL)
    return 0;
  else
    return this->rawChunk->size;
}

/***********
 * TChunk
 ***********
 * GetType
 ***********
 * Result is the type signature of this chunk.
 */

TTypeID
TChunk::GetType () const

{
  if (this->rawChunk == NULL)
    return TTypeID (0);
  else
    return TTypeID (this->rawChunk->type);
}

/**************
 * TChunk
 **************
 * GetSubType
 **************
 * Result is the sub-type signature of this chunk.
 */

TTypeID
TChunk::GetSubType () const

{
  // Only stream chunks have sub-type information

  if (this->IsStreamChunk ())
    return TTypeID (((StreamChunkHeader *)this->rawChunk)->subChunkType);
  else
    return TTypeID (0);
}

/***************
 * TChunk
 ***************
 * GetDataSize
 ***************
 * Result is the size of the data part
 * of this chunk.
 */

uint32
TChunk::GetDataSize () const

{
  if (this->rawChunk == NULL)
    return 0;
  else
    return this->rawChunk->size - sizeof (ChunkHeader);
}

/***************
 * operator <<
 ***************
 * Print the state of the specified chunk to the
 * specified character stream.
 */

ostream &
operator<< (ostream &stream, const TChunk &chunk)

{
  stream << chunk.GetType () << "\t" << chunk.GetSubType () << "\t"
         << chunk.GetSize () << "\n";

  return stream;
}

/****************
 * TChunk
 ****************
 * operator >>=
 ****************
 * Flatten this chunk to the specified stream.
 */

ostream &
TChunk::operator>>= (ostream &stream) const

{
  if (this->rawChunk != NULL)
    stream.write ((char *)this->rawChunk, (int)this->rawChunk->size);

  return stream;
}

/****************
 * TChunk
 ****************
 * operator <<=
 ****************
 * Unflatten this chunk from the specified stream.
 */

istream &
TChunk::operator<<= (istream &stream)

{
  uint32 rawType;
  uint32 rawSize;

  stream.read ((char *)&rawType, sizeof (uint32));
  stream.read ((char *)&rawSize, sizeof (uint32));

  this->rawChunk = (ChunkHeader *)new char[rawSize];

  if (this->rawChunk != NULL)
    {
      this->rawChunk->type = rawType;
      this->rawChunk->size = rawSize;
      stream.read ((char *)this->rawChunk + sizeof (ChunkHeader),
                   (int)this->GetDataSize ());
    }

  return stream;
}

/********************************/
/*  PROTECTED MEMBER FUNCTIONS  */
/********************************/

/*****************
 * TChunk
 *****************
 * IsStreamChunk
 *****************
 * Determine whether this chunk is in stream format.
 */

Boolean
TChunk::IsStreamChunk () const

{
  uint32 rawType;

  if (this->rawChunk == NULL)
    return false;
  else
    {
      rawType = this->rawChunk->type;
      return (rawType == CHUNK_SANM || rawType == CHUNK_SCEL
              || rawType == CHUNK_SNDS || rawType == CHUNK_FILM
              || rawType == CHUNK_JOIN || rawType == CHUNK_CONTROL
              || rawType == CHUNK_SAAA);
    }
}

/***************
 * TChunk
 ***************
 * GetRawChunk
 ***************
 * Return a pointer to the encapsulated
 * raw chunk representation.
 */

void *
TChunk::GetRawChunk () const

{
  return this->rawChunk;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/**********
 * TChunk
 **********
 * Copy
 **********
 * Copy the specified chunk object into
 * this chunk object.
 */

void
TChunk::Copy (const TChunk &that)

{
  uint32 size;

  size = that.GetSize ();

  if (that.rawChunk == NULL)
    this->rawChunk = NULL;
  else
    {
      delete[](char *) this->rawChunk;
      this->rawChunk = (ChunkHeader *)new char[size];
      if (this->rawChunk != NULL)
        memcpy (this->rawChunk, that.rawChunk, (size_t)size);
    }
}

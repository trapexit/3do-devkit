/*******************************************************************************************
 *	File:			ChunkSequence.cp
 *
 *	Contains:		Describes a sequence of chunk type identifiers.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#include "Chunk.h"
#include "ChunkFileFormat.h"
#include "ChunkQueue.h"
#include "ChunkSequence.h"
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

/******************
 * TChunkSequence
 ******************
 * Null constructor
 */

TChunkSequence::TChunkSequence () { this->length = 0; }

/******************
 * TChunkSequence
 ******************
 * Copying constructor
 */

TChunkSequence::TChunkSequence (const TChunkSequence &that)

{
  this->CopyFrom (that);
}

/*******************
 * ~TChunkSequence
 *******************
 * Destructor
 */

TChunkSequence::~TChunkSequence () {}

/******************
 * TChunkSequence
 ******************
 * operator =
 ******************
 * Take the value of this object from that of
 * the specified object.  Result is a reference
 * to this object.
 */

TChunkSequence &
TChunkSequence::operator= (const TChunkSequence &that)

{
  if (this != &that)
    {
      // Copy from the right-hand side

      this->CopyFrom (that);
    }

  return *this;
}

/******************
 * TChunkSequence
 ******************
 * Match
 ******************
 * Match the chunks on the front of the specified
 * queue to this chunk type sequence.
 * Result is an enum indicating the extent of the
 * match.
 */

TChunkSequence::TMatch
TChunkSequence::Match (const TChunkQueue &chunkList) const

{
  int matchCount;     // count of type identifiers that match so far
  TMatch matchResult; // result of the attempt to match
  TChunk *chunk;      // next chunk in the list
  TChunkQueueIterator iterator (&chunkList); // iterator over chunk list
  TTypeID chunkType;
  TTypeID chunkSubType;

  matchResult = NONE;
  chunk = iterator.GetNext ();

  // Visit each chunk in the list

  for (matchCount = 0; chunk != NULL;)
    {

      // Exit loop if chunk doesn't match sequence

      chunkType = chunk->GetType ();
      chunkSubType = chunk->GetSubType ();

      if (chunkType != this->typeID[matchCount]
          || chunkSubType != this->subTypeID[matchCount])
        {
          matchResult = NONE;
          matchCount = 0;
          break;
        }

      // Chunk matched; advance to next chunk in list

      chunk = iterator.GetNext ();

      // Exit loop if sequence is matched exactly

      if (++matchCount >= this->length)
        {
          matchResult = EXACT;
          break;
        }

      // Chunk matched and sequence is not yet exhausted, so
      // keep going

      matchResult = PARTIAL;
    }

  return matchResult;
}

/******************
 * TChunkSequence
 ******************
 * Append
 ******************
 * Append the specified type identifiers to this
 * sequence.
 */

void
TChunkSequence::Append (TTypeID nextTypeID, TTypeID nextSubTypeID)

{
  if (this->length < MAX_TYPEIDS)
    {
      this->typeID[length] = nextTypeID;
      this->subTypeID[length] = nextSubTypeID;
      ++this->length;
    }
}

/******************
 * TChunkSequence
 ******************
 * GetLength
 ******************
 * Result is the length of this sequence in chunks.
 */

int
TChunkSequence::GetLength () const

{
  return this->length;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/******************
 * TChunkSequence
 ******************
 * CopyFrom
 ******************
 * Copy this object from the specified object.
 */

void
TChunkSequence::CopyFrom (const TChunkSequence &that)

{
  for (int i = 0; i < that.length; ++i)
    {
      this->typeID[i] = that.typeID[i];
      this->subTypeID[i] = that.subTypeID[i];
    }
  this->length = that.length;
}

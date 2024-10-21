/*******************************************************************************************
 *	File:			ChunkSequenceHandler.cp
 *
 *	Contains:		Base class for objects that handle chunk sequence
 *extraction or analysis.
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
#include "ChunkFile.h"
#include "ChunkFileFormat.h"
#include "ChunkQueue.h"
#include "ChunkSequence.h"
#include "ChunkSequenceHandler.h"
#include "MultiPartFileName.h"
#include "TypeID.h"
#include <files.h>
#include <fstream.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*********************/
/*  PRIVATE GLOBALS  */
/*********************/

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/*************************
 * TChunkSequenceHandler
 *************************
 * Null constructor
 */

TChunkSequenceHandler::TChunkSequenceHandler () { this->sequenceCount = 0; }

/**************************
 * ~TChunkSequenceHandler
 **************************
 * Destructor
 */

TChunkSequenceHandler::~TChunkSequenceHandler () {}

Boolean
TChunkSequenceHandler::HandleSequence (TChunkQueue &chunkList)

{
  TChunkSequence::TMatch match;
  int chunkCount;
  TChunk *chunk;
  Boolean handled;

  handled = false;

  // Iterate over the registered chunk sequences

  for (int i = 0; !handled && i < this->sequenceCount; ++i)
    {
      match = this->chunkSequence[i]->Match (chunkList);

      if (match == TChunkSequence::EXACT)
        {
          // Call the handler function

          (this->*handler[i]) (chunkList);

          // Scrub the handled sequence from the chunk list

          chunkCount = this->chunkSequence[i]->GetLength ();

          while (chunkCount--)
            {
              chunk = chunkList.Remove ();
              delete chunk;
            }

          // All done

          handled = true;
        }
      else if (match == TChunkSequence::PARTIAL)
        {
          // Partial match; need more chunks to see if the match
          // will be exact.

          handled = true;
        }
    }

  return handled;
}

/********************************/
/*  PROTECTED MEMBER FUNCTIONS  */
/********************************/

/*************************
 * TChunkSequenceHandler
 *************************
 * RegisterChunkSequence
 *************************
 * Register the specified handler function for the specified
 * chunk sequence.  When a list of chunks submitted to this
 * object matches the sequence, the associated handler function
 * (a member of this object) is called.
 */

void
TChunkSequenceHandler::RegisterSequence (
    const TChunkSequence *chunkSequencePtr, TChunkSequenceFunction handler)
{
  if (this->sequenceCount < MAX_SEQUENCES)
    {
      this->chunkSequence[this->sequenceCount] = chunkSequencePtr;
      this->handler[this->sequenceCount] = handler;
      ++this->sequenceCount;
    }
}

/*************************
 * TChunkSequenceHandler
 *************************
 * OutputChunkList
 *************************
 * Write the specified chunks to the output file.
 */

void
TChunkSequenceHandler::OutputChunkList (const char *fileName,
                                        const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList);
  TChunk *chunk;
  TChunkFile outputFile;

  // Open the output file

  outputFile.Open (fileName);

  // Iterate over the chunk list, writing each chunk
  // to the output file

  while ((chunk = iter.GetNext ()) != NULL)
    outputFile.Write (*chunk);

  // Close the output file

  outputFile.Close ();
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

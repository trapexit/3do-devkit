/*******************************************************************************************
 *	File:			SAnimExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from
 *a SANM stream file.
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
#include "ChunkSequenceHandler.h"
#include "ChunkTypes.h"
#include "Form3DOChunks.h"
#include "Form3DOFileFormat.h"
#include "MultiPartFileName.h"
#include "SAnimExtractor.h"
#include "StreamChunks.h"
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

/*******************
 * TSAnimExtractor
 *******************
 * Null constructor
 */

TSAnimExtractor::TSAnimExtractor (const char *outputPathName)
    : outputName (outputPathName)

{
  this->mainCCBChunk = NULL;

  // Condition the output file name object
  // File names have the form "SANMframeNNNN"

  this->outputName.SetPartName ("SANMframe");

  // Build SANM header sequence

  this->ccbSequence.Append (CHUNK_SANM, SANM_HEADER);
  this->ccbSequence.Append (CHUNK_SANM, SANM_CCB);

  // Build SANM frame sequence

  this->frameSequence.Append (CHUNK_SANM, SANM_FRAME);

  // Register handler functions for these sequences

  this->RegisterSequence (
      &this->frameSequence,
      (TChunkSequenceFunction)&TSAnimExtractor::HandleFrameSequence);
  this->RegisterSequence (
      &this->ccbSequence,
      (TChunkSequenceFunction)&TSAnimExtractor::HandleHeaderSequence);
}

/********************
 * ~TSAnimExtractor
 ********************
 * Destructor
 */

TSAnimExtractor::~TSAnimExtractor () { delete this->mainCCBChunk; }

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/************************
 * TSAnimExtractor
 ************************
 * HandleHeaderSequence
 ************************
 *
 */

void
TSAnimExtractor::HandleHeaderSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList); // iterator over chunk list
  TSAnimCCBChunk *sanimCCBChunk;

  // Ignore the header chunk
  (void)iter.GetNext ();

  // Get the SANM CCB chunk.  Cast the representation down to an object
  // of the specific type and extract the encapsulated CCB.
  // Save the CCB as a CCB chunk object.

  sanimCCBChunk = (TSAnimCCBChunk *)iter.GetNext ();

  this->mainCCBChunk = new TCCBChunk (*sanimCCBChunk->GetCCB ());
}

/***********************
 * TSAnimExtractor
 ***********************
 * HandleFrameSequence
 ***********************
 *
 */

void
TSAnimExtractor::HandleFrameSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList); // iterator over chunk list
  TSAnimFrameChunk *sanimFrame;
  TChunkQueue outputList;

  // Short-circuit if no header sequence has been seen yet

  if (this->mainCCBChunk == NULL)
    return;

  // Add the CCB chunk to the output list

  outputList.Add (this->mainCCBChunk);

  // Add the chunks embedded in the frame chunk to
  // the output list

  sanimFrame = (TSAnimFrameChunk *)iter.GetNext ();
  sanimFrame->GetEmbeddedChunks (outputList);

  // Write the output chunk list to the next output file

  this->OutputChunkList (this->outputName, outputList);

  // Advance to the next output file name

  this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
}

/*******************************************************************************************
 *	File:			SAAAnimExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from
 *a SAAA stream file.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/26/94		crm		Enabled fixed TOptions class
 *	9/16/94		crm		New today
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
#include "Options.h"
#include "SAAAnimExtractor.h"
#include "StreamChunks.h"
#include "TypeID.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

extern TOptions gOptions;

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/*********************
 * TSAAAnimExtractor
 *********************
 * Null constructor
 */

TSAAAnimExtractor::TSAAAnimExtractor (const char *outputPathName)
    : outputName (outputPathName)

{
  this->mainCCBChunk = NULL;
  this->edgeCCBChunk = NULL;
  this->edgePLUTChunk = NULL;

  // Condition the output file name object
  // File names have the form "SAAAframeNNNN"

  this->outputName.SetPartName ("SAAAframe");

  // Build SAAA header sequence

  this->ccbSequence.Append (CHUNK_SAAA, SANM_HEADER);
  this->ccbSequence.Append (CHUNK_SAAA, SANM_CCB);
  this->ccbSequence.Append (CHUNK_SAAA, SAAA_CCB);
  this->ccbSequence.Append (CHUNK_SAAA, SAAA_PLUT);

  // Build SAAA frame sequence

  this->frameSequence.Append (CHUNK_SAAA, SANM_FRAME);

  // Register handler functions for these sequences

  this->RegisterSequence (
      &this->frameSequence,
      (TChunkSequenceFunction)&TSAAAnimExtractor::HandleFrameSequence);
  this->RegisterSequence (
      &this->ccbSequence,
      (TChunkSequenceFunction)&TSAAAnimExtractor::HandleHeaderSequence);
}

/**********************
 * ~TSAAAnimExtractor
 **********************
 * Destructor
 */

TSAAAnimExtractor::~TSAAAnimExtractor ()

{
  delete this->mainCCBChunk;
  delete this->edgeCCBChunk;
  delete this->edgePLUTChunk;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/************************
 * TSAAAnimExtractor
 ************************
 * HandleHeaderSequence
 ************************
 *
 */

void
TSAAAnimExtractor::HandleHeaderSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList); // iterator over chunk list
  TSAnimCCBChunk *sanimCCBChunk;
  TSAnimPLUTChunk *sanimPLUTChunk;
  const uint32 *sanimPLUT;
  int count;

  // Ignore the header chunk
  (void)iter.GetNext ();

  // Get the SAAA CCB and edge CCB chunks.  Cast the representations down to
  // objects of the specific type and extract the encapsulated CCBs.
  // Save the CCBs as a CCB chunk objects.

  sanimCCBChunk = (TSAnimCCBChunk *)iter.GetNext ();
  this->mainCCBChunk = new TCCBChunk (*sanimCCBChunk->GetCCB ());
  sanimCCBChunk = (TSAnimCCBChunk *)iter.GetNext ();
  this->edgeCCBChunk = new TCCBChunk (*sanimCCBChunk->GetCCB ());

  // Similarly, get an object for the edge cel's PLUT

  sanimPLUTChunk = (TSAnimPLUTChunk *)iter.GetNext ();
  sanimPLUT = sanimPLUTChunk->GetPLUT (count);
  this->edgePLUTChunk = new TPLUTChunk (sanimPLUT, count);
}

/***********************
 * TSAAAnimExtractor
 ***********************
 * HandleFrameSequence
 ***********************
 *
 */

void
TSAAAnimExtractor::HandleFrameSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList); // iterator over chunk list
  TSAnimFrameChunk *sanimFrame;
  TChunkQueue outputList;
  TChunkQueue embeddedList;
  TChunk *chunk;

  // Short-circuit if header sequence is not all there

  if (this->mainCCBChunk == NULL || this->edgeCCBChunk == NULL
      || this->edgePLUTChunk == NULL)
    return;

  // Add the CCB chunk to the output list

  outputList.Add (this->mainCCBChunk);

  // Get the chunks embedded in the frame chunk

  sanimFrame = (TSAnimFrameChunk *)iter.GetNext ();
  sanimFrame->GetEmbeddedChunks (embeddedList);

  // Add the main cel's PDAT chunk

  chunk = embeddedList.Remove ();
  outputList.Add (chunk);

  // Next embedded chunk is either a PLUT for the main
  // cel or the edge cel's PDAT

  chunk = embeddedList.Remove ();
  if (chunk->GetType () == TTypeID (CHUNK_PLUT))
    {
      outputList.Add (chunk);
      chunk = embeddedList.Remove ();
    }

  if (gOptions.GetOperation () == TOptions::EXTRACT)
    {
      // Extracting the main cel and edge separately

      this->OutputChunkList (this->outputName, outputList);
      outputList.RemoveAll ();

      // Add a suffix to the name of the output file

      this->outputName.SetSuffix ("edge");
    }

  // Add the edge CCB and PLUT to the output list

  outputList.Add (this->edgeCCBChunk);
  outputList.Add (this->edgePLUTChunk);

  // Add the edge cel's PDAT chunk to the output list

  outputList.Add (chunk);

  // Write the output chunk list to the next output file

  this->OutputChunkList (this->outputName, outputList);

  // Advance to the next output file name and make sure the file
  // suffix is left empty

  this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
  this->outputName.SetSuffix (NULL);
}

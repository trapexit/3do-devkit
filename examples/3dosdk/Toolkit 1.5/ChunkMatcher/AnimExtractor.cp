/*******************************************************************************************
 *	File:			AnimExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from
 *an ANIM chunk in a form 3DO file.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright � 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/26/94		crm		Enabled fixed TOptions class
 *	9/16/94		crm		New today
 *
 *******************************************************************************************/

#include "AnimExtractor.h"
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

/******************
 * TAnimExtractor
 ******************
 * Null constructor
 */

TAnimExtractor::TAnimExtractor (const char *outputPathName)
    : outputName (outputPathName)

{
  // Condition the output file name object
  // File names have the form "ANIMframeNNNN"

  this->outputName.SetPartName ("ANIMframe");

  // Build ANIM frame sequence

  this->frameSequence.Append (CHUNK_CCB, 0);
  this->frameSequence.Append (CHUNK_PDAT, 0);

  // Build anti-aliased ANIM frame sequence

  this->aaFrameSequence.Append (CHUNK_CCB, 0);
  this->aaFrameSequence.Append (CHUNK_PDAT, 0);
  this->aaFrameSequence.Append (CHUNK_CCB, 0);
  this->aaFrameSequence.Append (CHUNK_PLUT, 0);
  this->aaFrameSequence.Append (CHUNK_PDAT, 0);

  // Register handler functions for these sequences
  // Be sure to put the longer sequence first

  this->RegisterSequence (
      &this->aaFrameSequence,
      (TChunkSequenceFunction)&TAnimExtractor::HandleAAFrameSequence);
  this->RegisterSequence (
      &this->frameSequence,
      (TChunkSequenceFunction)&TAnimExtractor::HandleFrameSequence);
}

/*******************
 * ~TAnimExtractor
 *******************
 * Destructor
 */

TAnimExtractor::~TAnimExtractor () {}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/***********************
 * TAnimExtractor
 ***********************
 * HandleFrameSequence
 ***********************
 *
 */

void
TAnimExtractor::HandleFrameSequence (const TChunkQueue &chunkList)

{
  // Write the chunk list as-is to the next output file

  this->OutputChunkList (this->outputName, chunkList);

  // Advance to the next output file name

  this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
}

/*************************
 * TAnimExtractor
 *************************
 * HandleAAFrameSequence
 *************************
 *
 */

void
TAnimExtractor::HandleAAFrameSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList);
  TChunkQueue outputList;

  if (gOptions.GetOperation () == TOptions::EXTRACT_GROUPS)
    {
      // Write the chunk list as-is to the next output file

      this->OutputChunkList (this->outputName, chunkList);
    }
  else
    {
      // Split the chunk list into the main and edge cels, and
      // write them out individually.
      // First assemble the main cel's CCB and PDAT

      outputList.Add (iter.GetNext ());
      outputList.Add (iter.GetNext ());

      this->OutputChunkList (this->outputName, outputList);
      outputList.RemoveAll ();

      // Add a suffix to the name of the output file

      this->outputName.SetSuffix ("edge");

      // Now assemble the edge cel's CCB, PLUT, and PDAT

      outputList.Add (iter.GetNext ());
      outputList.Add (iter.GetNext ());
      outputList.Add (iter.GetNext ());

      this->OutputChunkList (this->outputName, outputList);

      // Remove the suffix on the output name and advance
      // the frame number

      this->outputName.SetSuffix (NULL);
      this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
    }

  // Advance to the next output file name

  this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
}

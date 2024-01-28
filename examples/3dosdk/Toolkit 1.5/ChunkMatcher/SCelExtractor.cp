/*******************************************************************************************
 *	File:			SCelExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from
 *a SCEL stream file.
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
#include "SCelExtractor.h"
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

/******************
 * TSCelExtractor
 ******************
 * Null constructor
 */

TSCelExtractor::TSCelExtractor (const char *outputPathName)
    : outputName (outputPathName)

{
  // Condition the output file name object
  // File names have the form "SCELframeNNNN"

  this->outputName.SetPartName ("SCELframe");

  // Build SCEL header sequence

  this->headerSequence.Append (CHUNK_SCEL, SCEL_HEADER);

  // Build SCEL frame sequence

  this->frameSequence.Append (CHUNK_SCEL, SCEL_CELLIST);

  // Register handler functions for these sequences

  this->RegisterSequence (
      &this->frameSequence,
      (TChunkSequenceFunction)&TSCelExtractor::HandleFrameSequence);
  this->RegisterSequence (
      &this->headerSequence,
      (TChunkSequenceFunction)&TSCelExtractor::HandleHeaderSequence);
}

/*******************
 * ~TSCelExtractor
 *******************
 * Destructor
 */

TSCelExtractor::~TSCelExtractor () {}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/************************
 * TSCelExtractor
 ************************
 * HandleHeaderSequence
 ************************
 *
 */

void
TSCelExtractor::HandleHeaderSequence (const TChunkQueue &)

{
}

/***********************
 * TSCelExtractor
 ***********************
 * HandleFrameSequence
 ***********************
 *
 */

void
TSCelExtractor::HandleFrameSequence (const TChunkQueue &chunkList)

{
  TChunkQueueIterator iter (&chunkList); // iterator over chunk list
  TChunkQueue outputList;

  if (gOptions.GetOperation () == TOptions::EXTRACT_GROUPS)
    {
      // Write the chunk list as-is to the next output file

      this->OutputChunkList (this->outputName, chunkList);

      // Advance to the next output file name

      this->outputName.SetPartNumber (this->outputName.GetPartNumber () + 1);
    }
}

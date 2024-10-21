/*******************************************************************************************
 *	File:			Main.cp
 *
 *	Contains:		Parse through and operate on the chunks in a chunk
 *file.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/3/94		crm		Verbose output to cout is flushed on
 *every line.
 *	9/29/94		crm		Added error check for input file not
 *opening.
 *	9/28/94		crm		Added SpinCursor feedback in main loop
 *	9/26/94		crm		Enabled fixed TOptions class
 *	6/16/94		crm		New today
 *
 *******************************************************************************************/

#include "AnimExtractor.h"
#include "Chunk.h"
#include "ChunkFile.h"
#include "ChunkFileFormat.h"
#include "ChunkQueue.h"
#include "ChunkSequence.h"
#include "ChunkSequenceHandler.h"
#include "Form3DOChunks.h"
#include "Form3DOFileFormat.h"
#include "MultiPartFileName.h"
#include "Options.h"
#include "SAAAnimExtractor.h"
#include "SAnimExtractor.h"
#include "SCelExtractor.h"
#include "StreamAnalyzer.h"
#include "TypeID.h"
#include <cursorctl.h>
#include <files.h>
#include <fstream.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*************/
/*  GLOBALS  */
/*************/

TOptions gOptions;

/*********************/
/*  PRIVATE CLASSES  */
/*********************/

class TChunkMatcher
{
public:
  TChunkMatcher ();

  virtual ~TChunkMatcher ();

  void Run ();
  // Execute the program

private:
  enum
  {
    MAX_HANDLERS = 4
  };
  // number of sequence handlers

  TChunkMatcher (const TChunkMatcher &) {}
  void
  operator= (const TChunkMatcher &)
  {
  }
  // Copying and assignment are disallowed

  Boolean HandleSequence ();
  // Match and handle the sequence at the head of the chunk list

  TChunkFile inputFile;  // pointer to head of this queue
  TChunkQueue chunkList; // list of chunks read from the input file
  TChunkSequenceHandler
      *chunkSequenceHandler[MAX_HANDLERS]; // available sequence handlers
};

/*****************
 * TChunkMatcher
 *****************
 * Constructor
 * Initialize for ChunkMatcher execution
 */

TChunkMatcher::TChunkMatcher ()

{
  // Open the input chunk file

  this->inputFile.OpenReadOnly (gOptions.GetInputFileName ());

  // Null all of the pointers in the chunk-sequence-handler table

  for (int i = 0; i < MAX_HANDLERS; ++i)
    this->chunkSequenceHandler[i] = NULL;

  // Create the individual chunk sequence handlers

  if (gOptions.GetOperation () == TOptions::STREAM_INFO)
    {
      // Analysis handlers

      this->chunkSequenceHandler[0] = new TStreamAnalyzer ();
    }
  else
    {
      // Extraction handlers

      this->chunkSequenceHandler[0]
          = new TSAnimExtractor (gOptions.GetOutputDirectoryName ());
      this->chunkSequenceHandler[1]
          = new TSCelExtractor (gOptions.GetOutputDirectoryName ());
      this->chunkSequenceHandler[2]
          = new TAnimExtractor (gOptions.GetOutputDirectoryName ());
      this->chunkSequenceHandler[3]
          = new TSAAAnimExtractor (gOptions.GetOutputDirectoryName ());
    }
}

/******************
 * ~TChunkMatcher
 ******************
 * Destructor
 */

TChunkMatcher::~TChunkMatcher ()

{
  for (int i = 0; i < MAX_HANDLERS; ++i)
    delete this->chunkSequenceHandler[i];
}

/*****************
 * TChunkMatcher
 *****************
 * Run
 *****************
 * Main body of ChunkMatcher program
 */

void
TChunkMatcher::Run ()

{
  TChunk *chunk; // next chunk read from input file

  // Make sure the input file is open and ready

  if (!inputFile)
    {
      cerr << "*** Input file did not open\n";
      return;
    }

  // Read every chunk from the input file

  while ((chunk = inputFile.Read ()) != NULL)
    {

      // Keep the cursor spinning as feedback that work is being done

      SpinCursor (32);

      // Print information about the chunk if the
      // verbose option is enabled

      if (gOptions.GetVerbose ())
        {
          cout << *chunk;
          cout.flush ();
        }

      // Add the new chunk to the tail of the list

      this->chunkList.Add (chunk);

      // Try to match and handle the sequence at the head of
      // the chunk list

      if (!this->HandleSequence ())
        {
          // Didn't match at all; remove the head chunk and try again

          chunk = chunkList.Remove ();
          delete chunk;
        }
    }
}

Boolean
TChunkMatcher::HandleSequence ()

{
  Boolean handled;
  TChunkSequenceHandler *nextHandler;

  // Iterate over the chunk-sequence-handler table

  handled = false;

  for (int i = 0; i < MAX_HANDLERS; ++i, ++nextHandler)
    {
      nextHandler = this->chunkSequenceHandler[i];

      // See if the next handler can match the sequence at the
      // head of the chunk list

      if (nextHandler != NULL)
        {
          handled = nextHandler->HandleSequence (this->chunkList);
          if (handled)
            {
              // Matched partially or exactly; stop looking
              break;
            }
        }
    }

  return handled;
}

int
main (int argc, char **argv)

{
  TChunkMatcher *chunkMatcher; // program object

  if (!gOptions.Parse (argc, argv))
    {
      gOptions.PrintUsage (cerr);
      goto Return;
    }

  // Initialize

  chunkMatcher = new TChunkMatcher ();
  if (chunkMatcher == NULL)
    {
      cerr << "*** Out of memory";
      goto Return;
    }

  // Execute

  chunkMatcher->Run ();

  // Clean up

  delete chunkMatcher;

Return:
  return 0;
}

/*******************************************************************************************
 *	File:			ChunkFile.h
 *
 *	Contains:		Input/Output file of chunks.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/3/94		crm		Fixed EOF test in ReadChunk
 *	9/29/94		crm		Added operator ! to detect if file
 *opened. 7/16/94		crm		New today
 *
 *******************************************************************************************/

#include "Chunk.h"
#include "ChunkFile.h"
#include "ChunkFileFormat.h"
#include "ChunkTypes.h"
#include "TypeID.h"
#include <files.h>
#include <fstream.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <types.h>

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/**************
 * TChunkFile
 **************
 * Null constructor
 * File has "CEL" file creator and type as a default.
 */

TChunkFile::TChunkFile () : type (CEL_FILE_TYPE), creator (CEL_FILE_CREATOR)
{
  this->blocking = 0;
  this->fileName = NULL;
}

/***************
 * ~TChunkFile
 ***************
 * Destructor
 */

TChunkFile::~TChunkFile ()

{
  this->Close ();

  delete[] this->fileName;
  this->fileName = NULL;
}

/**************
 * TChunkFile
 **************
 * Open
 **************
 */

void
TChunkFile::Open (const char *name, long blockingFactor)

{
  this->InternalOpen (name, WRITE_PERMISSION, blockingFactor);
}

/****************
 * TChunkFile
 ****************
 * OpenReadOnly
 ****************
 */

void
TChunkFile::OpenReadOnly (const char *name, long blockingFactor)

{
  this->InternalOpen (name, READ_PERMISSION, blockingFactor);
}

/**************
 * TChunkFile
 **************
 * Close
 **************
 * Close this chunk file.
 */

void
TChunkFile::Close ()

{
  // Close the file

  this->stream.close ();

  // Update the file's creator and type

  this->WriteFinderType ();
}

/**************
 * TChunkFile
 **************
 * Read
 **************
 * Read the next chunk from this chunk file.
 */

TChunk *
TChunkFile::Read ()

{
  TChunk *chunk = NULL; // chunk read from file

  if (this->stream.good ())
    {

      // ******* Adjust for blocking *******

      // Allocate storage for the new chunk object

      chunk = new TChunk ();

      // Have the chunk object unflatten itself from the
      // file stream

      if (chunk != NULL)
        *chunk <<= this->stream;

      // Toss the chunk if the read failed for some reason

      if (!this->stream.good () || this->stream.eof ())
        {
          delete chunk;
          chunk = NULL;
        }
    }

  return chunk;
}

/**************
 * TChunkFile
 **************
 * Write
 **************
 * Write the next chunk to this chunk file.
 */

void
TChunkFile::Write (const TChunk &chunk)

{
  // ******* Adjust for blocking *******

  // Have the chunk object flatten itself to the
  // file stream

  chunk >>= this->stream;
}

/**************
 * TChunkFile
 **************
 * operator !
 **************
 * Result is false if the file is open for
 * I/O, and true otherwise.
 */

int
TChunkFile::operator!() const

{
  return !this->stream;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/****************
 * TChunkFile
 ****************
 * InternalOpen
 ****************
 */

void
TChunkFile::InternalOpen (const char *name, AccessPermission permission,
                          long blockingFactor)

{
  this->blocking = blockingFactor;

  // Open the file

  this->stream.open (name, (permission == READ_PERMISSION)
                               ? (ios::in | ios::nocreate)
                               : (ios::in | ios::out));

  // Save a copy of the file path name

  this->fileName = new char[strlen (name) + 1];
  if (this->fileName != NULL)
    strcpy (this->fileName, name);
}

/*******************
 * TChunkFile
 *******************
 * WriteFinderType
 *******************
 * Write the Macintosh Finder information
 * (creator and type) for this chunk file.
 */

void
TChunkFile::WriteFinderType () const

{
  FInfo finderInfo; // Finder information for the specified file
  OSErr error;

  // Read-modify-write the file's Finder information to change
  // the creator and type.

  c2pstr (this->fileName);
  error = GetFInfo ((ConstStr255Param)this->fileName, 0, &finderInfo);
  if (error != 0)
    return;

  finderInfo.fdCreator = (unsigned long)this->creator;
  finderInfo.fdType = (unsigned long)this->type;

  error = SetFInfo ((ConstStr255Param)this->fileName, 0, &finderInfo);
  p2cstr ((unsigned char *)this->fileName);
}

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
 *	9/29/94		crm		Added operator ! to detect if file
 *opened. 7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __CHUNKFILE__
#define __CHUNKFILE__

/*************/
/*  CLASSES  */
/*************/

class TChunkFile
{
public:
  TChunkFile ();
  virtual ~TChunkFile ();

  void Open (const char *name, long blockingFactor = 0);
  // Open the specified file as a chunk file

  void OpenReadOnly (const char *name, long blockingFactor = 0);
  // Open the specified file as a read-only chunk file

  void Close ();
  // Close this chunk file

  TChunk *Read ();
  // Read the next chunk from this chunk file

  void Write (const TChunk &chunk);
  // Write the next chunk to this chunk file

  int operator!() const;
  // Returns true if this file is not open for I/O

private:
  enum AccessPermission
  {
    READ_PERMISSION,
    WRITE_PERMISSION
  };

  TChunkFile (const TChunkFile &) {}
  void
  operator= (const TChunkFile &)
  {
  }

  void InternalOpen (const char *name, AccessPermission permission,
                     long blockingFactor = 0);
  // Helper function to do the work of an open

  void WriteFinderType () const;
  // Update Macintosh Finder information for this file

  long blocking;   // blocking factor for this chunk file
  char *fileName;  // path name of the chunk file
  TTypeID creator; // Macintosh file creator
  TTypeID type;    // Macintosh file type
  fstream stream;  // file stream implementation
};

#endif

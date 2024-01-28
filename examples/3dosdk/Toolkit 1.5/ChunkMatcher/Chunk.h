/*******************************************************************************************
 *	File:			Chunk.h
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
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __CHUNK__
#define __CHUNK__

/*************/
/*  CLASSES  */
/*************/

class TChunk
{
public:
  TChunk ();
  TChunk (ChunkHeader *rawChunkPtr);

  virtual ~TChunk ();

  virtual TChunk &operator= (const TChunk &);

  virtual uint32 GetSize () const;
  // Return size of chunk header and data

  virtual uint32 GetDataSize () const;
  // Return size of chunk data

  virtual TTypeID GetType () const;
  // Return type signature of this chunk

  virtual TTypeID GetSubType () const;
  // Return sub-type signature of this chunk

  virtual ostream &operator>>= (ostream &) const;
  // Flatten this chunk to the specified stream

  virtual istream &operator<<= (istream &);
  // Unflatten this chunk from the specified stream

  friend ostream &operator<< (ostream &, const TChunk &);
  // Print this chunk to the specified character stream

protected:
  TChunk (const TChunk &);
  TChunk (uint32 rawType, uint32 rawSize);

  void *GetRawChunk () const;
  // Return a pointer to the raw chunk representation

  Boolean IsStreamChunk () const;
  // Determine whether this chunk is a stream-format chunk

private:
  void Copy (const TChunk &that);
  // Copy this object from the specified object

  ChunkHeader *rawChunk; // pointer to header of chunk
};

#endif

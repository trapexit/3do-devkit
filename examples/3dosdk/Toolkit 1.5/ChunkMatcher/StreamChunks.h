/*******************************************************************************************
 *	File:			StreamChunks.h
 *
 *	Contains:		Classes for chunks found in stream files.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __STREAMCHUNKS__
#define __STREAMCHUNKS__

/*************/
/*  CLASSES  */
/*************/

class TSAnimCCBChunk : public TChunk
{
public:
  const CCB *GetCCB () const;
  // Return the CCB encapsulated in this chunk

private:
  TSAnimCCBChunk (){};
  virtual ~TSAnimCCBChunk (){};
  TSAnimCCBChunk (const TSAnimCCBChunk &) {}
};

class TSAnimFrameChunk : public TChunk
{
public:
  void GetEmbeddedChunks (TChunkQueue &chunkList) const;
  // Return a list of chunks encapsulated in this chunk

private:
  TSAnimFrameChunk (){};
  virtual ~TSAnimFrameChunk (){};
  TSAnimFrameChunk (const TSAnimFrameChunk &) {}
};

class TSAnimPLUTChunk : public TChunk
{
public:
  const uint32 *GetPLUT (int &count) const;
  // Return the PLUT encapsulated in this chunk

private:
  TSAnimPLUTChunk (){};
  virtual ~TSAnimPLUTChunk (){};
  TSAnimPLUTChunk (const TSAnimPLUTChunk &) {}
};

#endif

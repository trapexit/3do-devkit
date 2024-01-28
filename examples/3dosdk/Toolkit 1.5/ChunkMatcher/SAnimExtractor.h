/*******************************************************************************************
 *	File:			SAnimExtractor.h
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

#ifndef __SANIMEXTRACTOR__
#define __SANIMEXTRACTOR__

/*************/
/*  CLASSES  */
/*************/

class TSAnimExtractor : public TChunkSequenceHandler
{
public:
  TSAnimExtractor (const char *outputPathName);
  virtual ~TSAnimExtractor ();

private:
  TSAnimExtractor (const TSAnimExtractor &) {}
  void
  operator= (const TSAnimExtractor &)
  {
  }
  // Copying and assignment are disallowed

  void HandleHeaderSequence (const TChunkQueue &chunkList);
  // Handler for SANM header (CCB) chunk sequence

  void HandleFrameSequence (const TChunkQueue &chunkList);
  // Handler for SANM frame chunk sequence

private:
  TChunkSequence ccbSequence;    // type identifiers for CCB sequence
  TChunkSequence frameSequence;  // type identifiers for frame data sequence
  TCCBChunk *mainCCBChunk;       // CCB chunk for set of frames
  TMultiPartFileName outputName; // name of next output file
};

#endif

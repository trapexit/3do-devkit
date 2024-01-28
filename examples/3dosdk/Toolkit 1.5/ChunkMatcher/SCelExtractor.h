/*******************************************************************************************
 *	File:			SCelExtractor.h
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
 *	9/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __SCELEXTRACTOR__
#define __SCELEXTRACTOR__

/*************/
/*  CLASSES  */
/*************/

class TSCelExtractor : public TChunkSequenceHandler
{
public:
  TSCelExtractor (const char *outputPathName);
  virtual ~TSCelExtractor ();

private:
  TSCelExtractor (const TSCelExtractor &) {}
  void
  operator= (const TSCelExtractor &)
  {
  }
  // Copying and assignment are disallowed

  void HandleHeaderSequence (const TChunkQueue &chunkList);
  // Handler for SCEL header chunk sequence

  void HandleFrameSequence (const TChunkQueue &chunkList);
  // Handler for SCEL frame chunk sequence

private:
  TChunkSequence headerSequence; // type identifiers for header sequence
  TChunkSequence frameSequence;  // type identifiers for frame data sequence
  TMultiPartFileName outputName; // name of next output file
};

#endif

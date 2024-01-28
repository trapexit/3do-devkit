/*******************************************************************************************
 *	File:			StreamAnalyzer.h
 *
 *	Contains:		Object that handles chunk sequence analysis on a
 *generic stream file.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/25/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __STREAMANALYZER__
#define __STREAMANALYZER__

/*************/
/*  CLASSES  */
/*************/

class TStreamAnalyzer : public TChunkSequenceHandler
{
public:
  TStreamAnalyzer ();
  virtual ~TStreamAnalyzer ();

private:
  TStreamAnalyzer (const TStreamAnalyzer &) {}
  void
  operator= (const TStreamAnalyzer &)
  {
  }
  // Copying and assignment are disallowed

  void HandleControlSequence (const TChunkQueue &chunkList);
  // Handler for CTRL chunk in stream

  void HandleFillerSequence (const TChunkQueue &chunkList);
  // Handler for FILL chunk in stream

  void HandleDataSequence (const TChunkQueue &chunkList);
  // Handler for all other chunks in stream

  void OnEnd ();
  // Perform end-of-run processing

private:
  TChunkSequence controlSequence; // type identifiers for control sequence
  TChunkSequence fillerSequence;  // type identifiers for filler sequence
  TChunkSequence dataSequence;    // type identifiers for stream-data sequence
  TMultiPartFileName outputName;  // name of next output file

  uint32 fillerBytes;  // total bytes of filler
  uint32 controlBytes; // total bytes of control information
  uint32 dataBytes;    // total bytes of data
};

#endif

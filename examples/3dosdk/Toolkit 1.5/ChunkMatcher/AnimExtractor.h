/*******************************************************************************************
 *	File:			AnimExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from
 *an ANIM chunk in a form-3DO file.
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

#ifndef __ANIMEXTRACTOR__
#define __ANIMEXTRACTOR__

/*************/
/*  CLASSES  */
/*************/

class TAnimExtractor : public TChunkSequenceHandler
{
public:
  TAnimExtractor (const char *outputPathName);
  virtual ~TAnimExtractor ();

private:
  TAnimExtractor (const TAnimExtractor &) {}
  void
  operator= (const TAnimExtractor &)
  {
  }
  // Copying and assignment are disallowed

  void HandleFrameSequence (const TChunkQueue &chunkList);
  // Handler for ANIM frame chunk sequence

  void HandleAAFrameSequence (const TChunkQueue &chunkList);
  // Handler for ANIM anti-aliased frame chunk sequence

private:
  TChunkSequence frameSequence; // type identifiers for frame data sequence
  TChunkSequence
      aaFrameSequence; // type identifiers for anti-aliased frame data sequence
  TMultiPartFileName outputName; // name of next output file
};

#endif

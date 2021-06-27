/*******************************************************************************************
 *	File:			SAAAnimExtractor.cp
 *
 *	Contains:		Object that handles chunk sequence extraction from a SAAA stream
 *					file.
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


#ifndef __SAAANIMEXTRACTOR__
#define __SAAANIMEXTRACTOR__



/*************/
/*  CLASSES  */
/*************/


class TSAAAnimExtractor : public TChunkSequenceHandler {
public:

						TSAAAnimExtractor (const char* outputPathName);
	virtual 			~TSAAAnimExtractor ();


private:
						TSAAAnimExtractor (const TSAAAnimExtractor&) {}
			void		operator = (const TSAAAnimExtractor&) {}
							// Copying and assignment are disallowed

			void		HandleHeaderSequence (const TChunkQueue& chunkList);
							// Handler for SAAA header chunk sequence

			void		HandleFrameSequence (const TChunkQueue& chunkList);
							// Handler for SAAA frame chunk sequence


private:
	TChunkSequence		ccbSequence;	// type identifiers for CCB sequence
	TChunkSequence		frameSequence;	// type identifiers for frame data sequence
	TCCBChunk*			mainCCBChunk;	// CCB chunk for set of frames
	TCCBChunk*			edgeCCBChunk;	// edge CCB chunk for set of frames
	TPLUTChunk*			edgePLUTChunk;	// edge PLUT chunk for set of frames
	TMultiPartFileName	outputName;		// name of next output file
};



#endif


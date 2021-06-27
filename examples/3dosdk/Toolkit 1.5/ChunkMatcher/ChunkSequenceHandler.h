/*******************************************************************************************
 *	File:			ChunkSequenceHandler.h
 *
 *	Contains:		Base class for objects that handle chunk sequence extraction
 *					or analysis.
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


#ifndef __CHUNKSEQUENCEHANDLER__
#define __CHUNKSEQUENCEHANDLER__



/*************/
/*  CLASSES  */
/*************/


class TChunkSequenceHandler {
public:

						TChunkSequenceHandler ();
	virtual 			~TChunkSequenceHandler ();

	virtual	Boolean		HandleSequence (TChunkQueue& chunkList);
								// Match and handle the sequence at the head of the specified list


protected:
	typedef	void (TChunkSequenceHandler::*TChunkSequenceFunction) (const TChunkQueue& chunkList);
								// pointer to handling member function

			void		RegisterSequence (const TChunkSequence* chunkSequencePtr,
										  TChunkSequenceFunction handler);
								// Register the specified handler function for the specified chunk sequence

			void		OutputChunkList (const char* fileName, const TChunkQueue& chunkList);
								// Write the specified chunks to the output file


private:
						TChunkSequenceHandler (const TChunkSequenceHandler&) {}
			void		operator = (const TChunkSequenceHandler&) {}
								// Copying and assignment are disallowed


private:
	enum { MAX_SEQUENCES = 8 };
								// Array bound for table of sequences and handlers functions

	long					sequenceCount;					// number of sequences handled
	const TChunkSequence*	chunkSequence[MAX_SEQUENCES];	// pointers to sequences handled
	TChunkSequenceFunction	handler[MAX_SEQUENCES];			// pointers to handler functions
};



#endif


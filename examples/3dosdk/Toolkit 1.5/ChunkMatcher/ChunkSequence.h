/*******************************************************************************************
 *	File:			ChunkSequence.cp
 *
 *	Contains:		Describes a sequence of chunk type identifiers.
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


#ifndef __CHUNKSEQUENCE__
#define __CHUNKSEQUENCE__



/*************/
/*  CLASSES  */
/*************/

class TChunkSequence {
public:
	enum TMatch { NONE, PARTIAL, EXACT };
								// Chunk sequences can match exactly, partially
								// (from the beginning), or not at all

							TChunkSequence ();
							TChunkSequence (const TChunkSequence&);

	virtual 				~TChunkSequence ();

			TChunkSequence&	operator = (const TChunkSequence&);
								// Assign this object from the specified object

			void			Append (TTypeID nextTypeID, TTypeID nextSubTypeID);
								// Append the specified type identifiers to this sequence

			TMatch			Match (const TChunkQueue& chunkList) const;
								// Match the chunks on the front of the specified
								// queue to this chunk type sequence

			int				GetLength () const;
								// Return the length of this sequence in chunks


private:
	enum { MAX_TYPEIDS = 8 };
								// size of array representation of type sequence

			void			CopyFrom (const TChunkSequence&);
								// Copy this object from the specified object

	long		length;					// number of chunk type (pairs) in the sequence
	TTypeID		typeID[MAX_TYPEIDS];	// chunk type identifiers
	TTypeID		subTypeID[MAX_TYPEIDS];	// chunk sub-type identifiers
};


#endif


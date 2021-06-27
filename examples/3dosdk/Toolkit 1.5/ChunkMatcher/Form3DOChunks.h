/*******************************************************************************************
 *	File:			Form3DOChunks.cp
 *
 *	Contains:		Classes for chunks found in Form-3DO files.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __FORM3DOCHUNKS__
#define __FORM3DOCHUNKS__



/*************/
/*  CLASSES  */
/*************/



class TCCBChunk : public TChunk {
public:
				TCCBChunk (const CCB& ccb);
	virtual 	~TCCBChunk ();

private:
				TCCBChunk () {}
				TCCBChunk (const TCCBChunk&) {}
};



class TPLUTChunk : public TChunk {
public:
				TPLUTChunk (const uint32* plutPtr, int count = MAX_PLUT_ENTRIES);
	virtual 	~TPLUTChunk ();

private:
				TPLUTChunk () {}
				TPLUTChunk (const TPLUTChunk&) {}
};



#endif


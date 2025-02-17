/*******************************************************************************************
 *	File:			Form3DOChunks.cp
 *
 *	Contains:		Classes for chunks found in Form-3DO files.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright � 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/16/94		crm		New today
 *
 *******************************************************************************************/

#include "Chunk.h"
#include "ChunkFileFormat.h"
#include "ChunkTypes.h"
#include "Form3DOChunks.h"
#include "Form3DOFileFormat.h"
#include "TypeID.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/*************
 * TCCBChunk
 *************
 * Null constructor
 */

TCCBChunk::TCCBChunk (const CCB &ccb) : TChunk (CHUNK_CCB, sizeof (CCBChunk))
{
  CCBChunk *ccbRawChunk; // pointer to the raw chunk in memory

  // Initialize the CCB-chunk-specific fields of the
  // raw representation

  ccbRawChunk = (CCBChunk *)this->GetRawChunk ();
  if (ccbRawChunk != NULL)
    {
      ccbRawChunk->version = 0;
      ccbRawChunk->ccb = ccb;
    }
}

/**************
 * ~TCCBChunk
 **************
 * Destructor
 */

TCCBChunk::~TCCBChunk () {}

TPLUTChunk::TPLUTChunk (const uint32 *plutPtr, int count)
    : TChunk (CHUNK_PLUT, sizeof (PLUTChunk)
                              - (PLUT_ENTRY_SIZE * (MAX_PLUT_ENTRIES - count)))
{
  PLUTChunk *plutRawChunk; // pointer to the raw chunk in memory

  // Initialize the PLUT-chunk-specific fields of the
  // raw representation

  plutRawChunk = (PLUTChunk *)this->GetRawChunk ();
  if (plutRawChunk != NULL)
    {
      plutRawChunk->numEntries = count;
      for (int i = 0; i < count; ++i)
        {
          plutRawChunk->PLUT[i] = *plutPtr++;
        }
    }
}

/***************
 * ~TPLUTChunk
 ***************
 * Destructor
 */

TPLUTChunk::~TPLUTChunk () {}

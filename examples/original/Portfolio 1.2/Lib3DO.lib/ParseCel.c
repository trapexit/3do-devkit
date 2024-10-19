/*
        File:		ParseCel.c

        Contains:	Routine to parse a cel previously loaded into a buffer,
                                creating a valid CCB with PDAT and PLUT
   attached to it.

        Written by:	Ian

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 8/20/93	JAY		first checked in

        To Do:
*/

#include "BlockFile.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "Debug3DO.h"

/*****************************************************************************
 * CCB * ParseCel(void *inBuf, long inBufSize)
 *
 *	Parses a cel file in a buffer, returns a pointer to the CCB for the
 *cel; the CCB will contain pointers to the pixels and (optional) PLUT.
 *
 ****************************************************************************/

CCB *
ParseCel (void *inBuf, long inBufSize)
{
  ulong chunk_ID;
  long tempSize;
  char *tempBuf;
  char *pChunk;
  CCB *pCCB;
  long *pPLUT;
  CelData *pPDAT;

  /*------------------------------------------------------------------------
   * sanity-check parms.
   *----------------------------------------------------------------------*/

#if DEBUG
  if (inBuf == NULL)
    {
      DIAGNOSE (("NULL buffer pointer\n"));
      return NULL;
    }

  if (inBufSize < sizeof (CCB))
    {
      DIAGNOSE (
          ("Buffer size is less than sizeof(CCB); can't be a Cel file\n"));
      return NULL;
    }
#endif

  /*------------------------------------------------------------------------
   * loop thru chunks, remembering pointers to the important bits.
   *----------------------------------------------------------------------*/

  pCCB = NULL;
  pPDAT = NULL;
  pPLUT = NULL;
  tempBuf = (char *)inBuf;
  tempSize = inBufSize;

  while ((pChunk = GetChunk (&chunk_ID, &tempBuf, &tempSize)) != NULL)
    {
      switch (chunk_ID)
        {
        case CHUNK_CCB:
          if (pCCB == NULL)
            {
              pCCB = (CCB *)&((CCC *)pChunk)->ccb_Flags;
              pCCB->ccb_Flags
                  |= CCB_SPABS | CCB_PPABS | CCB_NPABS | CCB_YOXY
                     | CCB_LAST; /* V32 anims might not have these set */
            }
          break;

        case CHUNK_PDAT:
          if (pPDAT == NULL)
            {
              pPDAT = (CelData *)((PixelChunk *)pChunk)->pixels;
            }
          break;

        case CHUNK_PLUT:
          if (pPLUT == NULL)
            {
              pPLUT = (long *)((PLUTChunk *)pChunk)->PLUT;
            }
          break;

        case CHUNK_CPYR:
        case CHUNK_DESC:
        case CHUNK_KWRD:
        case CHUNK_CRDT:
        case CHUNK_XTRA:
          break;

        default:
          DIAGNOSE (("Unexpected chunk ID %.4s, ignored\n", pChunk));
          break;
        }
    }

    /*------------------------------------------------------------------------
     * do a couple basic sanity checks
     *----------------------------------------------------------------------*/

#ifdef DEBUG

  if (pCCB == NULL)
    {
      DIAGNOSE (("No CCB found in buffer\n"));
      goto ERROR_EXIT;
    }

  if (pPDAT == NULL)
    {
      DIAGNOSE (("No pixel data chunk found buffer\n"));
      goto ERROR_EXIT;
    }

#endif

  /*------------------------------------------------------------------------
   * link the PDAT and (optional) PLUT to the CCB.
   *----------------------------------------------------------------------*/

  pCCB->ccb_SourcePtr = pPDAT;
  pCCB->ccb_PLUTPtr = pPLUT;

  return pCCB;

ERROR_EXIT:

  return NULL;
}

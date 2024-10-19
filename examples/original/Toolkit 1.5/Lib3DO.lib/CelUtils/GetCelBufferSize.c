/*****************************************************************************
 *	File:			GetCelBufferSize.c
 *
 *	Contains:		Routines to get cel data buffer size, and similar
 *values that require poking around in the cel in tricky ways to obtain simple
 *values.
 *
 *	Copyright:	(c) 1994 The 3DO Company. 	All Rights Reserved.
 *
 *	History:
 *	04/09/94  Ian	Fixed GetCelDataBufferSize() to add 8 bytes if the
 *CCBPRE flag is not set. 03/03/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	GetCelDataBufferSize() returns the size the data buffer needs to be,
 *based on the cel format.  This is not necessarily the size of the buffer as
 *it exists (if in fact it exists at all; ccb_SourcePtr can be NULL and the
 *	function still works).
 ****************************************************************************/

#include "CelUtils.h"

static ubyte bppTable[] = { 0, 1, 2, 4, 6, 8, 16, 0 };

/*----------------------------------------------------------------------------
 * GetCelBitsPerPixel()
 *	Calc and return the bits-per-pixel of a cel.
 *--------------------------------------------------------------------------*/

int32
GetCelBitsPerPixel (CCB *cel)
{
  return bppTable[CEL_PRE0WORD (cel) & PRE0_BPP_MASK];
}

/*----------------------------------------------------------------------------
 * GetCelBytesPerRow()
 *	Calc and return the bytes-per-row of a cel.
 *--------------------------------------------------------------------------*/

int32
GetCelBytesPerRow (CCB *cel)
{
  int32 woffset;
  uint32 pre1;

  pre1 = CEL_PRE1WORD (cel);

  if (GetCelBitsPerPixel (cel) < 8)
    {
      woffset = (pre1 & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT;
    }
  else
    {
      woffset = (pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;
    }

  return ((woffset + PRE1_WOFFSET_PREFETCH) * sizeof (int32));
}

/*----------------------------------------------------------------------------
 * GetCelDataBufferSize()
 *	Calc and return the size needed for a cel's data buffer.
 *--------------------------------------------------------------------------*/

int32
GetCelDataBufferSize (CCB *cel)
{
  return (cel->ccb_Height * GetCelBytesPerRow (cel))
                 + (cel->ccb_Flags & CCB_CCBPRE)
             ? 0
             : 8;
}

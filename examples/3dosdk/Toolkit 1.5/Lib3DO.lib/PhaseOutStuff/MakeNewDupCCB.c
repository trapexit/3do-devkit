/*****************************************************************************
 *	File:			MakeNewDupCCB.c
 *
 *	Contains:		Routine to clone a cel.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This routine isn't very useful; use CloneCel() instead.
 ****************************************************************************/

#include "Utils3DO.h"
#include "mem.h"
#include "string.h"

CCB *
MakeNewDupCCB (CCB *ccb)
{
  CCB *newCCB;

  if ((newCCB = (CCB *)ALLOCMEM (sizeof (CCB), MEMTYPE_CEL)) == NULL)
    return NULL;

  memcpy (newCCB, ccb, sizeof (CCB));

  return newCCB;
}

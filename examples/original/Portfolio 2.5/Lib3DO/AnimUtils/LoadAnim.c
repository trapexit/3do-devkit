
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: LoadAnim.c,v 1.9 1994/11/02 00:25:05 vertex Exp $
**
**  Lib3DO routine to allocate a buffer and load an anim file
**  into it, using fast block I/O, then parse the anim file into an
**  ANIM structure with attached AnimFrames.
**
**  Iterate through each chunk and fill the AnimFrames with pointers
**  to CCBs, PLUTs, and Pixel Data for each frame. If an Anim has one
**  CCB and many PDATs the CCB pointer is used repeatedly for each frame
**  in the AnimFrame. We end up with an array of AnimEntries that we can
**  index into at any point and display a valid frame from each entry.
**
**  If we run across an ANIM chunk before we've allocated any AnimFrames,
**  we gain a bit of efficiency by allocating all the AnimFrames we'll
**  need at once.  If there is no ANIM chunk in the file, we'll expand
**  the array of AnimFrames on the fly as needed.
**
**  We handle both types of ANIM files which are currently fashionable:
**  3DO animator format, where PLUTs come before PDATs, and "glom-them-
**  together-with-MPW-cat-command" where PDATs come before PLUTs. In an
**  ideal world, there'd be a tool to create Animator-like ANIM files
**  from a collection of cels, and everyone would promptly convert all
**  their existing ANIMs.  ::sigh::
**
******************************************************************************/

#include "animutils.h"
#include "blockfile.h"
#include "debug3do.h"
#include "mem.h"
#include "types.h"

/*----------------------------------------------------------------------------
 * LoadAnim()
 *	Load an ANIM file, parse it into an ANIM structure and a set of
 *AnimFrames.
 *--------------------------------------------------------------------------*/

ANIM *
LoadAnim (char *name, uint32 memTypeBits)
{
  int32 filesize;
  void *filebuf;
  ANIM *pAnim;

  /*------------------------------------------------------------------------
   * Load the file into a buffer, call ParseAnim() to do the real work.
   *----------------------------------------------------------------------*/

  if (NULL == (filebuf = LoadFile (name, &filesize, memTypeBits)))
    {
      DIAGNOSE (("Can't load file %s\n", name));
      return NULL;
    }

  if (NULL == (pAnim = ParseAnim (filebuf, filesize, memTypeBits)))
    {
      UnloadFile (filebuf);
      DIAGNOSE (("Can't parse file %s\n", name));
      return NULL;
    }

  pAnim->dataBuffer = filebuf; /* attach buffer so Unload can free it. */

  return pAnim;
}

/*----------------------------------------------------------------------------
 * UnloadAnim()
 *	Unload an ANIM file and free all resources acquired for it by
 *LoadAnim().
 *--------------------------------------------------------------------------*/

void
UnloadAnim (ANIM *anim)
{
  if (anim != NULL)
    {
      UnloadFile (anim->dataBuffer);
      FreeMem (anim->pentries, -1);
      FreeMem (anim, -1);
    }
}

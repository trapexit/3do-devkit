/*****************************************************************************
 *	File:			LoadCel.c
 *
 *	Contains:		Routine to allocate a buffer and load a cel
 *					into it, using fast block I/O, then parse the
 *cel.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/25/93 	ian Now uses new faster LoadFile() block I/O instead of
 *					filestream I/O.
 *
 *	Implementation notes:
 *
 *	We have to meet a pair of goals here:  Load a cel file in such a way
 *that it can later be unloaded with a call to UnloadCel(), and do so without
 *	loading into one buffer then copying to another (which requires twice
 *	as much memory, and is slower anyway).	LoadCel() returns a pointer to
 *	a CCB which it has located within the file buffer.	That pointer
 *won't be at a known offset from the start of the buffer however, so we need
 *	a way for UnloadCel() to be able to find the start of the allocated
 *	block containing the CCB we found when parsing the buffer.
 *
 *	One given that we do have to work with is that wherever we find the
 *	CCB within the file buffer, we know there are 8 bytes of groodah in
 *	front of it that can be safely trashed (the chunk_ID and chunk_size for
 *	the CCC chunk).  So, after we've parsed out the CCB from the cel and
 *	have its pointer all ready to return to the caller, we lay a magic
 *	cookie into the 8 bytes in front of the CCB.  The first longword of
 *	the cookie is a magic number that validates the cookie, and the second
 *	longword is a pointer to the start of the file buffer.	If UnloadCel()
 *	recognizes the magic number, it knows it can safely use the following
 *	longword as a pointer that can be Free()'d.
 *
 *	Another fine item from the Kludges 'R Us catalog, offering the best in
 *	elegant hacks and designer workarounds for the fashion-forward
 *programmer.
 ****************************************************************************/

#include "BlockFile.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "Debug3DO.h"

typedef struct UnloadCookie
{
  ulong magic;
  void *buffer;
} UnloadCookie;

#define UNLOAD_MAGIC CHAR4LITERAL ('C', 'C', 'B', 'u')

/*----------------------------------------------------------------------------
 * CCB * LoadCel(char *filename, uint32 memTypeBits)
 *
 *	Loads a cel from a 3DO file.  Returns a pointer to the CCB for the cel;
 *	the CCB will contain pointers to the pixels and (optional) PLUT.
 *	The cel buffer can be freed later by passing the CCB pointer to
 *UnloadCel().
 *--------------------------------------------------------------------------*/

CCB *
LoadCel (char *name, uint32 memTypeBits)
{
  long filesize;
  void *filebuf;
  CCB *pCCB;
  UnloadCookie *cookie;

  if (NULL == (filebuf = LoadFile (name, &filesize, memTypeBits)))
    {
      DIAGNOSE (("Can't load file %s\n", name));
      return NULL;
    }

  if (NULL == (pCCB = ParseCel (filebuf, filesize)))
    {
      UnloadFile (filebuf);
      DIAGNOSE (("Can't parse cel file %s\n", name));
      return NULL;
    }

  cookie = ((UnloadCookie *)pCCB) - 1;
  cookie->magic = UNLOAD_MAGIC;
  cookie->buffer = filebuf;

  return pCCB;
}

/*----------------------------------------------------------------------------
 * UnloadCel
 *	Unload a cel file previously loaded via LoadCel().
 *--------------------------------------------------------------------------*/

void
UnloadCel (CCB *celbuf)
{
  UnloadCookie *cookie;

  if (celbuf)
    {
      cookie = ((UnloadCookie *)celbuf) - 1;
#if DEBUG
      if (cookie->magic != UNLOAD_MAGIC)
        {
          DIAGNOSE (("Attempt to UnloadCel() on a CCB not returned from "
                     "LoadCel()\n"));
        }
#endif
      Free (cookie->buffer);
    }
}

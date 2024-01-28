/*****************************************************************************
 *	File:			LoadFile.c
 *
 * 	Contains:		Routine to allocate a buffer and load an entire
 * 					file into it, using fast block I/O. Also, a
 *routine to unload the file (ie, free its buffer).
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reseerred.
 *
 *	History:
 *	07/12/94  Ian	Added LoadFileHere() and rearranged things so that both
 *					it and LoadFile() are serviced by a single
 *internal routine. 04/12/94  Ian	Modified so that a file of size zero
 *doesn't cause Malloc() to spit out an error about allocating a block of size
 *zero. Zero-length files are indicated by a NULL return code with *pFileSize
 *set to zero; whether this should be interpreted as an error is up to the
 *caller; we don't DIAGNOSE() it. 02/18/94  Ian	Reworked error detection logic.
 *The old way was failing to return bad status if the open worked but the read
 *failed. 09/06/93  JAY	change free to Free in two error paths in LoadFile.
 *	08/20/93  JAY	first checked in
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "BlockFile.h"
#include "Debug3DO.h"
#include "UMemory.h"
#include "operror.h"

/*----------------------------------------------------------------------------
 * load_file()
 *	Routine to service both LoadFile() and LoadFileHere().
 *--------------------------------------------------------------------------*/

static void *
load_file (char *fileName, int32 *pFileSize, uint32 memTypeBits,
           void *userBuffer, int32 userBufSize)
{
  Err err;
  int32 fileSize;
  int32 blockSize;
  int32 bufSize;
  void *buffer;
  void *internalBuffer;
  Item ioreq;
  BlockFile theFile;

  buffer = NULL;
  internalBuffer = NULL;
  ioreq = 0;
  theFile.fDevice = 0;

  if ((err = OpenBlockFile (fileName, &theFile)) < 0)
    {
      DIAGNOSE_SYSERR (err, ("Can't open file %s\n", fileName));
      goto ERROR_EXIT;
    }

  fileSize = theFile.fStatus.fs_ByteCount;
  blockSize = theFile.fStatus.fs.ds_DeviceBlockSize;
  bufSize = ((fileSize + blockSize - 1) / blockSize) * blockSize;

  if (fileSize > 0)
    {
      if (userBuffer != NULL)
        {
          if (userBufSize != 0 && userBufSize < bufSize)
            {
              err = BADSIZE; // status constant from operror.h
              DIAGNOSE_SYSERR (
                  err,
                  ("Supplied buffer is too small for file %s\n", fileName));
              goto ERROR_EXIT;
            }
          buffer = userBuffer;
        }
      else
        {
          if ((internalBuffer = Malloc (bufSize, memTypeBits)) == NULL)
            {
              err = NOMEM; // status constant from operror.h
              DIAGNOSE_SYSERR (
                  err, ("Can't allocate buffer for file %s\n", fileName));
              goto ERROR_EXIT;
            }
          buffer = internalBuffer;
        }

      if ((ioreq = CreateBlockFileIOReq (theFile.fDevice, 0)) < 0)
        {
          err = ioreq;
          DIAGNOSE_SYSERR (err,
                           ("Can't create IOReq for file %s\n", fileName));
          goto ERROR_EXIT;
        }

      if ((err = AsynchReadBlockFile (&theFile, ioreq, buffer, bufSize, 0))
          >= 0)
        {
          if ((err = WaitReadDoneBlockFile (ioreq)) >= 0)
            {
              err = ((IOReq *)LookupItem (ioreq))->io_Error;
            }
        }

      if (err < 0)
        {
          DIAGNOSE_SYSERR (err, ("Error reading file %s\n", fileName));
          goto ERROR_EXIT;
        }
    }

  err = fileSize;

ERROR_EXIT:

  CloseBlockFile (&theFile); // this is safe even if file isn't open.

  if (ioreq > 0)
    {                     // if we obtained an IOReq,
      DeleteItem (ioreq); // delete it.
    }

  if (err < 0)
    { // if exiting with bad status
      if (internalBuffer)
        {                        // and we acquired a file buffer,
          Free (internalBuffer); // free it, and set the pointer
        }                        // to NULL so that the function
      buffer = NULL;             // return value indicates error.
    }

  if (pFileSize)
    {                   // if the caller provided a status/size
      *pFileSize = err; // return value pointer, return the value.
    }

  return buffer; // return buffer pointer (NULL on error)
}

/*******************************************************************************************
 * void UnloadFile(void *fileBuf)
 *	Releases a file buffer acquired by a call to LoadFile().
 *******************************************************************************************/

void
UnloadFile (void *fileBuf)
{
  if (fileBuf)
    {
      Free (fileBuf);
    }
}

/*******************************************************************************************
 * void *LoadFile(char *filename, int32 *pFileSize, uint32 memTypeBits)
 *	Allocates a buffer (according to memTypeBits) and loads the named file
 *into it.
 *
 *	If the load succeeds, the return value is a pointer to the buffer, and
 *	the longword at *pFileSize is set to the length of the file.  (Note
 *that the buffer may be larger than the fileSize, due to the need to load
 *	entire device-sized blocks.  The value at *pFileSize reflects the
 *amount of valid data in the file, not the blockSize-aligned buffer size.)
 *
 *	If the load fails, the return value is NULL, and the longword at
 *	*pFileSize contains a negative error code value.
 *
 *******************************************************************************************/

void *
LoadFile (char *fileName, int32 *pFileSize, uint32 memTypeBits)
{
  return load_file (fileName, pFileSize, memTypeBits, NULL, 0);
}

/*******************************************************************************************
 * void *LoadFileHere(char *filename, int32 *pFileSize, void *buffer, int32
 *bufSize) Loads the named file into the buffer provided by the caller.
 *
 *	If the load succeeds, the return value is a pointer to the buffer, and
 *the longword at *pFileSize is set to the length of the file.  (The returned
 *pointer will always be the same as the passed-in buffer parm, or NULL to
 *indicate error.)
 *
 *	If the load fails, the return value is NULL, and the longword at
 *	*pFileSize contains a negative error code value.
 *
 *	If the bufSize parm is non-zero, a check is made to ensure the entire
 *file will fit into the buffer; if it won't, the return value is NULL and
 **pFileSize is set to BADSIZE. If the bufSize parm is zero, no check of buffer
 *size is made, you're on your own.
 *******************************************************************************************/

void *
LoadFileHere (char *fileName, int32 *pFileSize, void *buffer, int32 bufSize)
{
  return load_file (fileName, pFileSize, 0, buffer, bufSize);
}

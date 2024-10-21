/*******************************************************************************************
 *	File:			PrepareStream.c
 *
 *	Contains:		definitions for high level stream playback
 *preparation
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  05/24/94		DLD		Added UseDefaultStreamHeader
 *	10/20/93		jb		New today
 *
 *******************************************************************************************/

#include "PrepareStream.h"
#include "BlockFile.h"
#include "SAnimSubscriber.h"
#include "SAudioSubscriber.h"
#include "Types.h"
#include "UMemory.h"

#define CONTROL_CHUNK_TYPE                                                    \
  CHAR4LITERAL ('C', 'T', 'R', 'L') /* Chunk type for this subscriber */

/*********************************************************************************************
 * Routine to allocate and initialize a buffer list for use with the streamer
 *********************************************************************************************/
DSDataBufPtr
CreateBufferList (long numBuffers, long bufferSize)
{
#define ROUND_TO_LONG(x) ((x + 3) & ~3)
  DSDataBufPtr bp;
  DSDataBufPtr firstBp;
  long totalBufferSpace;
  long actualEntrySize;
  long bufferNum;

  actualEntrySize = sizeof (DSDataBuf) + ROUND_TO_LONG (bufferSize);
  totalBufferSpace = numBuffers * actualEntrySize;

  /* Try to allocate the needed memory */
  firstBp = (DSDataBufPtr)NewPtr (totalBufferSpace, MEMTYPE_ANY);
  if (firstBp == NULL)
    return NULL;

  /* Loop to take the big buffer and split it into "N" buffers
   * of size "bufferSize", linked together.
   */
  for (bp = firstBp, bufferNum = 0; bufferNum < (numBuffers - 1); bufferNum++)
    {
      bp->next = (DSDataBufPtr)(((char *)bp) + actualEntrySize);
      bp->permanentNext = bp->next;

      /* Advance to point to the next entry */
      bp = bp->next;
    }

  /* Make last entry's forward link point to nil */
  bp->next = NULL;
  bp->permanentNext = NULL;

  /* Return a pointer to the first buffer in the list
   */
  return firstBp;
}

/*********************************************************************************************
 * Routine to find and load a stream header into memory given a stream file
 *name.
 *********************************************************************************************/
int32
FindAndLoadStreamHeader (DSHeaderChunkPtr headerPtr, char *fileName)
{
  int32 status;
  BlockFile blockFile;
  Item ioDoneReplyPort;
  Item ioReqItem;
  long *pLong;
  char *buffer;

  /* Init our stuff */
  blockFile.fDevice = 0;
  ioDoneReplyPort = 0;
  ioReqItem = 0;
  buffer = NULL;

  /* Allocate a buffer to hold some of the first data
   * in the stream
   */
  buffer = (char *)NewPtr (FIND_HEADER_BUFFER_SIZE, MEMTYPE_ANY);
  if (buffer == NULL)
    return kPSMemFullErr;

  /* Open the specified file */
  status = OpenBlockFile (fileName, &blockFile);
  if (status != 0)
    {
      printf ("File can't be opened -- maybe it was not found. \n");
      return status;
    }

  /* Create a reply port for the I/O message */
  ioDoneReplyPort = NewMsgPort (NULL);
  if (ioDoneReplyPort < 0)
    {
      status = ioDoneReplyPort;
      goto BAILOUT;
    }

  /* Create an I/O request item */
  ioReqItem = CreateBlockFileIOReq (blockFile.fDevice, ioDoneReplyPort);
  if (ioReqItem < 0)
    {
      status = ioReqItem;
      goto BAILOUT;
    }

  /* Read the first block from the purported stream file */
  status = AsynchReadBlockFile (
      &blockFile,              /* block file */
      ioReqItem,               /* ioreq Item */
      buffer,                  /* place to READ data into */
      FIND_HEADER_BUFFER_SIZE, /* amount of data to READ */
      0);                      /* offset of data in the file */
  if (status != 0)
    goto BAILOUT;

  /* Wait for the read to complete */
  status = WaitReadDoneBlockFile (ioReqItem);
  if (status != 0)
    goto BAILOUT;

  /* Done with the file so close it */
  CloseBlockFile (&blockFile);
  blockFile.fDevice = 0;

  /* Search the buffer for the header information */
  /* NOTE: еее FOR NOW, WE ASSUME THE HEADER IS THE FIRST DATA IN THE FILE еее
   */
  pLong = (long *)buffer;

  /* Copy the entire header into the caller's structure */
  if (*pLong == HEADER_CHUNK_TYPE)
    {
      *headerPtr = *((DSHeaderChunkPtr)pLong);
      status = 0;
    }
  else
    status = kPSHeaderNotFound;

BAILOUT:
  if (buffer)
    FreePtr (buffer);
  if (blockFile.fDevice)
    CloseBlockFile (&blockFile);
  if (ioDoneReplyPort)
    DeleteItem (ioDoneReplyPort);
  if (ioReqItem)
    DeleteItem (ioReqItem);

  return status;
}

void
PrintfSAError (int32 DSStatus)
{
#if DEBUG
  char *err;

  switch (DSStatus)
    {
    case kPSVersionErr:
      err = "kPSVersionErr - Streamed Anim version mismatch";
      break;
    case kPSMemFullErr:
      err = "kPSMemFullErr - memory full in Streamed Anim ";
      break;
    case kPSUnknownSubscriber:
      err = "kPSUnknownSubscriber - unkown subscriber type";
      break;
    case kPSHeaderNotFound:
      err = "kPSHeaderNotFound - No Stream Header Found. Add "
            "'writestreamheader' to weave script";
      break;

    default:
      err = NULL;
    }

  printf ("%s : %lx\n", err ? err : "unknown StreamedAnim error", DSStatus);

  if (err == NULL)
    PrintfSysErr (DSStatus);
#endif
}

int32
UseDefaultStreamHeader (DSHeaderChunkPtr headerPtr)
{
  int32 index;

  headerPtr->headerVersion = DS_STREAM_VERSION;
  headerPtr->streamBlockSize = kDefaultBlockSize;
  headerPtr->streamBuffers = kDefaultNumBuffers;
  headerPtr->streamerDeltaPri = kStreamerDeltaPri;
  headerPtr->dataAcqDeltaPri = kDataAcqDeltaPri;
  headerPtr->numSubsMsgs = kNumSubsMsgs;
  headerPtr->audioClockChan = kAudioClockChan;
  headerPtr->enableAudioChan = kEnableAudioChan;

  /* Clear the instrument preload table */
  for (index = 0; index < DS_HDR_MAX_PRELOADINST; index++)
    headerPtr->preloadInstList[index] = 0;

  /* Clear the subscriber tag table */
  for (index = 0; index < DS_HDR_MAX_SUBSCRIBER; index++)
    {
      headerPtr->subscriberList[index].subscriberType = 0;
      headerPtr->subscriberList[index].deltaPriority = 0;
    }

  /* Set up tags for the default subscribers */
  headerPtr->subscriberList[0].subscriberType = CONTROL_CHUNK_TYPE;
  headerPtr->subscriberList[0].deltaPriority = kControlPriority;
  headerPtr->subscriberList[1].subscriberType = SNDS_CHUNK_TYPE;
  headerPtr->subscriberList[1].deltaPriority = kSoundsPriority;
  headerPtr->subscriberList[2].subscriberType = SANM_CHUNK_TYPE;
  headerPtr->subscriberList[2].deltaPriority = kSAnimPriority;

  return 0;
}

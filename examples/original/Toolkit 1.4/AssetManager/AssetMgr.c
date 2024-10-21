/*******************************************************************************************
 *	File:			AssetMgr.c
 *
 *	Contains:		simple read-only asset management database for 3DO
 *applications
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/15/93		jb		New today
 *
 *******************************************************************************************/

#include "AssetMgr.h"

/*******************************************************************************************
 * Routine to load an asset file into memory
 *******************************************************************************************/
int32
LoadAssetFile (char *name, AssetFileDescPtr af)
{
  int32 status;
  BlockFile assetFile;
  long fileSizeInBytes;
  long fileBlocksize;
  long fileBufSize;
  Item ioItem;
  AFHeaderPtr hdr;
  AFDataChunkPtr afd;

  /* Open the file we will be using */
  status = OpenBlockFile (name, &assetFile);
  if (status != 0)
    return status;

  /* Create an I/O request item to do our work with */
  ioItem = CreateBlockFileIOReq (assetFile.fDevice, 0);
  if (ioItem < 0)
    return ioItem;

  /* Get the size of the file and allocate a buffer to hold
   * the file's contents.
   */
  fileSizeInBytes = GetBlockFileSize (&assetFile);
  fileBlocksize = GetBlockFIleBlockSize (&assetFile);
  fileBufSize = ((fileSizeInBytes + fileBlocksize - 1) / fileBlocksize)
                * fileBlocksize;

  af->assetBuffer = (char *)malloc (fileBufSize);
  if (af->assetBuffer == NULL)
    return kAMAllocErr;

  /* Read the file into memory */
  status = AsynchReadBlockFile (&assetFile, ioItem, af->assetBuffer,
                                fileBufSize, 0);
  if (status != 0)
    return status;

  status = WaitReadDoneBlockFile (ioItem);
  if (status != 0)
    return status;

  /* Close the file and get rid of system resources we no longer need */
  DeleteItem (ioItem);
  CloseBlockFile (&assetFile);

  /* See if the file we read in looks like an asset file */
  hdr = (AFHeaderPtr)af->assetBuffer;
  if ((hdr->chunkType != ASSET_CHUNK_TYPE)
      || (hdr->subChunkType != ASSET_HEAD_TYPE))
    {
      UnloadAssetFile (af);
      return kAMBadFileForm;
    }

  /* Make sure the asset data is stored adjacent to the header
   * data in the file.
   * NOTE: THIS VERSION ONLY SUPPORTS MEMORY RESIDENT ASSET FILES!!!
   *			A FUTURE VERSION WILL USE THE FILE POSITION TO ACTUALLY
   *			LOAD THE ASSET DATA, IN PAGES IF NECESSARY.
   */
  afd = (AFDataChunkPtr)(af->assetBuffer + hdr->chunkSize);
  if ((afd->chunkType != ASSET_CHUNK_TYPE)
      || (afd->subChunkType != ASSET_DATA_TYPE))
    {
      UnloadAssetFile (af);
      return kAMBadFileForm;
    }

  /* Everything looks good */
  return kAMNoErr;
}

/*******************************************************************************************
 * Routine to close an asset file. Free's the data buffer associated with the
 *file and actually closes the file.
 *******************************************************************************************/
void
UnloadAssetFile (AssetFileDescPtr afd)
{
  if (afd->assetBuffer != NULL)
    free (afd->assetBuffer);
}

/*******************************************************************************************
 * Routine to get a pointer to asset key information for the specified asset.
 * NOTE: THIS VERSION ONLY SUPPORTS MEMORY RESIDENT ASSET FILES!!!
 *******************************************************************************************/
int32
GetAssetInfo (AssetFileDescPtr afd, AssetType type, AssetID id,
              AssetKeyPtr *pAssetKeyPtr)
{
  AFHeaderPtr hdr;
  AFTableEntryPtr aft;
  AssetKeyPtr akp;
  long typeNumber;
  long assetIndex;

  /* Search for the asset starting at the header */
  hdr = (AFHeaderPtr)afd->assetBuffer;
  aft = (AFTableEntryPtr)(((char *)hdr) + sizeof (AFHeader));
  for (typeNumber = 0; typeNumber < hdr->numAssetTypes; typeNumber++)
    {
      /* If requested type matches that of the table entry
       * then search the table.
       */
      if (aft->assetType == type)
        {
          /* Search each table entry for the requested id. If
           * found, return a pointer to it, otherwise the
           * requested asset is 'not found'.
           */
          for (assetIndex = 0; assetIndex < aft->assetCount; assetIndex++)
            {
              akp = (AssetKeyPtr)(((char *)aft) + sizeof (AFTableEntry));
              if (akp[assetIndex].assetID == id)
                {
                  *pAssetKeyPtr = akp + assetIndex;
                  return kAMNoErr;
                }
            }

          return kAMNotFound;
        }

      /* Advance to the table entry */
      aft = (AFTableEntryPtr)(((char *)aft)
                              + (sizeof (AFTableEntry)
                                 + aft->assetCount * sizeof (AssetKey)));
    }

  /* Asset type was not found */
  return kAMNotFound;
}

/*******************************************************************************************
 * Routine to get a pointer to asset data stored in an asset file. Loads the
 *data from disk if necessary. NOTE: THIS VERSION ONLY SUPPORTS MEMORY RESIDENT
 *ASSET FILES!!!
 *******************************************************************************************/
int32
GetAsset (AssetFileDescPtr afd, AssetType type, AssetID id, void **pAssetData)
{
  int32 status;
  AssetKeyPtr keyPtr;

  /* Locate the key for the requested asset data */
  status = GetAssetInfo (afd, type, id, &keyPtr);
  if (status != 0)
    return status;

  /* Return a pointer to the data */
  *pAssetData = afd->assetBuffer + keyPtr->assetFPOS;

  /* Success */
  return kAMNoErr;
}

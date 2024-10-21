/*******************************************************************************************
 *	File:			AssetMgr.h
 *
 *	Contains:		definitions for AssetMgr.c
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/15/93		jb		New today
 *
 *******************************************************************************************/
#ifndef __ASSETMGR_H__
#define __ASSETMGR_H__

#ifndef __BLOCKFILE_H__
#include "BlockFile.h"
#endif

#ifndef _ASSETFILEFORMAT_H_
#include "AssetFileFormat.h"
#endif

/******************************/
/* Types used by the AssetMgr */
/******************************/

typedef int32 AssetType;
typedef int32 AssetID;

enum
{
  kAMNoErr = 0,            /* success */
  kAMAllocErr = -13500,    /* couldn't allocate needed memory */
  kAMBadFileForm = -13501, /* file is not an AssetMgr file */
  kAMNotFound = -13502     /* requested asset not found */
};

/*************************************************/
/* Data structure for use with AssetMgr routines */
/*************************************************/

typedef struct AssetFileDesc
{
  char *assetBuffer;
} AssetFileDesc, *AssetFileDescPtr;

/**********************/
/* Routine prototypes */
/**********************/

int32 LoadAssetFile (char *name, AssetFileDescPtr pAfd);

void UnloadAssetFile (AssetFileDescPtr pAfd);

int32 GetAsset (AssetFileDescPtr pAfd, AssetType type, AssetID id,
                void **pAssetData);

int32 GetAssetInfo (AssetFileDescPtr pAfd, AssetType type, AssetID id,
                    AssetKeyPtr *pAssetKeyPtr);

#endif /* __ASSETMGR_H__ */

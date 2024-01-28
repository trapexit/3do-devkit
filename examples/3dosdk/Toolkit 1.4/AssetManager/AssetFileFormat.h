/*******************************************************************************************
 *	File:			AssetFileFormat.h
 *
 *	Contains:		platform independent description of the 3DO Asset
 *File Format
 *
 * The Asset File is a simple directorized, read-only database file. We borrow
 *from the Macintosh Resource Manager in using 32-bit data types and integer id
 *values to uniquely identify arbitrary, user-defined data. The index is
 *essentially a table of types, each table entry is variable length. There is
 *one table entry per resource type, and the entry tag is the same as the
 *resource type. The table entry contains pairs of id's and file offsets for
 *the corresponding data.
 *
 *	Overview of file format:
 *
 *							+-------------------+
 *							|	  AFHeader
 *|
 *							+-------------------+
 *							|	variable # of
 *| |	AFTableEntry's	| /					/ /
 */
 *							+-------------------+
 *							|	start of asset
 *| |		data		| /					/ /
 */
 *							+-------------------+
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/14/93		jb		New today
 *
 *******************************************************************************************/

#ifndef _ASSETFILEFORMAT_H_
#define _ASSETFILEFORMAT_H_

#ifndef __TYPES__
#include "Types.h"
#endif

/* One key exists for each asset contained in the asset file.
 * This structure is used to locate the associated asset data.
 */
typedef struct AssetKey
{
  long assetID;   /* asset id */
  long assetFPOS; /* FPOS in file where asset data is located */
  long assetSize; /* size of asset in BYTES */
} AssetKey, *AssetKeyPtr;

/* One table entry exists for each type of asset data in the asset file.
 * This can be regarded as the first level index.
 */
typedef struct AFTableEntry
{
  long assetType;  /* major asset type */
  long assetCount; /* number of these in the following table */
  // AssetKey	asset[0];		/* variable length array of AssetKey's
  // */
} AFTableEntry, *AFTableEntryPtr;

#ifndef SUBS_CHUNK_COMMON
/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define SUBS_CHUNK_COMMON                                                     \
  long chunkType;    /* chunk type */                                         \
  long chunkSize;    /* chunk size including header */                        \
  long time;         /* position in stream time */                            \
  long channel;      /* logical channel number */                             \
  long subChunkType; /* data sub-type */
#endif

/* One header is written to the beginning of the asset file. It is in the
 * form of most other 3DO IFF-style files. Note that the Asset Manager code
 * does not pay attention to IFF chunks, but depends on the stored file
 * position data to be valid. This means that this data cannot be relocated in
 * a stream of other data.
 */
typedef struct AFHeader
{
  SUBS_CHUNK_COMMON
  long numAssetTypes;  /* number of asset types */
  long dataOriginFPOS; /* beginning of asset data in file */
  // AFTableEntry	assetTable[0];	/* variable length array of table entries
  // */
} AFHeader, *AFHeaderPtr;

/* One IFF wrapper is written to encapsulate all of the asset data. We do
 * this to be nice to all the IFF tools.
 */
typedef struct AFDataChunk
{
  SUBS_CHUNK_COMMON
  long startOfData;
} AFDataChunk, *AFDataChunkPtr;

#define ASSET_CHUNK_TYPE (0x41535354) /* 'ASST' */
#define ASSET_HEAD_TYPE (0x48454144)  /* 'HEAD' */
#define ASSET_DATA_TYPE (0x41534454)  /* 'ASDT' */

#endif /* _ASSETFILEFORMAT_H_ */

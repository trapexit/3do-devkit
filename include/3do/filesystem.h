#ifndef __H_FILESYSTEM
#define __H_FILESYSTEM

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: filesystem.h,v 1.39 1994/10/21 21:06:16 shawn Exp $
**
**  Kernel data structures for filesystem access
**
******************************************************************************/


#include "types.h"
#include "io.h"
#include "discdata.h"
#include "device.h"
#include "driver.h"
#include "folio.h"
#include "semaphore.h"

#define FILEFOLIO          3

#define FILESYSTEMNODE  1
#define FILENODE        2
#define FILEALIASNODE   3

#define FILESYSTEM_DEFAULT_BLOCKSIZE   2048

#define FILESYSTEM_TAG_END             TAG_END
#define FILESYSTEM_TAG_PRI             TAG_ITEM_LAST+1

#define FILE_TAG_END                   TAG_END
#define FILE_TAG_DATASIZE              TAG_ITEM_LAST+1

#define FILETASK_TAG_CURRENTDIRECTORY  TAG_ITEM_LAST+1
#define FILETASK_TAG_PROGRAMDIRECTORY  TAG_ITEM_LAST+2

#define DEVICE_IS_READONLY 0x00000002

#define ALIAS_NAME_MAX_LEN     31
#define ALIAS_VALUE_MAX_LEN    255

#define FILESYSTEM_MAX_PATH_LEN		256
#define FILESYSTEM_PART_PATH_LEN	64

/*
  Supports WRITE (0), READ (1), and STATUS (2), plus other goodies.
*/

#define FILECMD_READDIR      3
#define FILECMD_GETPATH      4
#define FILECMD_READENTRY    5
#define FILECMD_ALLOCBLOCKS  6
#define FILECMD_SETEOF       7
#define FILECMD_ADDENTRY     8
#define FILECMD_DELETEENTRY  9
#define FILECMD_SETTYPE      10
#define FILECMD_OPENENTRY    11
#define FILECMD_FSSTAT       12

#define ER_Fs_NoFile            1
#define ER_Fs_NotADirectory     2
#define ER_Fs_NoFileSystem      3
#define ER_Fs_BadName           4
#define ER_Fs_MediaError        5
#define ER_Fs_Offline           6
#define ER_Fs_DeviceError       7
#define ER_Fs_ParamError        8
#define ER_Fs_NoSpace           9
#define ER_Fs_Damaged          10
#define ER_Fs_Busy             11
#define ER_FS_DuplicateFile    12
#define ER_FS_ReadOnly         13
#define ER_Fs_DuplicateLink    14
#define ER_Fs_CircularLink     15
#define ER_Fs_LAST             16

struct HighLevelDisk;
struct File;

typedef struct HighLevelDisk HighLevelDisk;
typedef struct File File;


/*
  N.B. - the HighLevelDisk structure devices the header for all
  "high-level" disk devices... those on which filesystems can reside.
  All high-level device structures should start out with a HighLevelDisk
  structure or its exact equivalent.  Also, the OpenFile device structure
  must contain a DeviceType field in the same position as HighLevelDisk's
  DeviceType field.
*/

struct HighLevelDisk {
  Device             hdi;
};


typedef struct OptimizedDisk {
  HighLevelDisk        odi;
} OptimizedDisk;


#ifndef notAFileErr
# define notAFileErr                   -1302
#endif


typedef struct MacDisk {
  HighLevelDisk      mdi;
} MacDisk;


typedef struct LinkedMemDisk {
  HighLevelDisk         lmd;
} LinkedMemDisk;

typedef struct FileSystem {
  ItemNode         fs;
} FileSystem;

struct File {
  ItemNode         fi;
};

/*
  File flag/status bits in the low byte are specific to the filesystem.
*/

#define FILE_IS_DIRECTORY       0x00000001
#define FILE_IS_READONLY        0x00000002
#define FILE_IS_FOR_FILESYSTEM  0x00000004
#define FILE_SUPPORTS_DIRSCAN   0x00000008
#define FILE_INFO_NOT_CACHED    0x00000010
#define FILE_SUPPORTS_ENTRY     0x00000020
#define FILE_BLOCKS_CACHED      0x00000040
#define FILE_NOT_READY          0x00000080
#define FILE_HAS_LINK      	0x00000100

#define FILE_TYPE_DIRECTORY     0x2a646972
#define FILE_TYPE_LABEL         0x2a6c626c
#define FILE_TYPE_CATAPULT      0x2a7a6170

#define FILE_HIGHEST_AVATAR    255

typedef struct OpenFile {
  Device             ofi;
} OpenFile;

typedef struct FileIOReq {
  IOReq          fio;
} FileIOReq;

typedef struct FileStatus {
  DeviceStatus   fs;
  uint32         fs_ByteCount;
} FileStatus;


typedef struct Alias {
  ItemNode       a;
  uchar         *a_Value;
} Alias;


typedef struct FileFolio {
  Folio          ff;
} FileFolio;


/*
 *	Filesystem status definitions (FILECMD_FSSTAT)
 */
#define	FSSTAT_CREATETIME	0x1
#define	FSSTAT_BLOCKSIZE	0x2
#define	FSSTAT_SIZE		0x4
#define	FSSTAT_MAXFILESIZE	0x8
#define	FSSTAT_FREE		0x10
#define	FSSTAT_USED		0x20

#define	FSSTAT_ISSET(bmap, bit)	(bmap & bit)
#define	FSSTAT_SET(bmap, bit)	(bmap |= bit)

#ifndef	HOWMANY
#define	HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#endif	/* HOWMANY */

typedef struct FileSystemStat {
  uint32     fst_BitMap;	/* field bitmap */
  uint32     fst_CreateTime;	/* filesystem creation time */
  uint32     fst_BlockSize;	/* block size of the filesystem */
  uint32     fst_Size;		/* size of the filesystem in blocks */
  uint32     fst_MaxFileSize;	/* max blocks that can be allocated */
  uint32     fst_Free;		/* total number of free blocks available */
  uint32     fst_Used;		/* total number of blocks in use */
} FileSystemStat;



/*****************************************************************************/


#endif /* __H_FILESYSTEM */

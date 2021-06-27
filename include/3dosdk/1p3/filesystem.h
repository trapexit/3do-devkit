/*****

$Id: filesystem.h,v 1.28 1994/04/07 00:40:33 shawn Exp $

$Log: filesystem.h,v $
 * Revision 1.28  1994/04/07  00:40:33  shawn
 * dirsema disbled.
 *
 * Revision 1.26  1994/03/31  00:45:30  dplatt
 * Add a new linked-memory filesystem operating phase (after writing data,
 * check to see if EOF needs to be set).
 *
 * Revision 1.25  1994/03/26  01:26:56  dplatt
 * Move CodeHandle typedef to types.h.
 *
 * Revision 1.24  1994/03/25  22:28:26  dplatt
 * Add CodeHandle typedef
 *
 * Revision 1.23  1994/03/24  01:46:27  dplatt
 * Move the "raw device unit number" field into the standard
 * HighLevelDisk structure, and add a "raw device Item" right next
 * to it.  We're going to need 'em for hot dismount...
 *
 * Revision 1.22  1994/02/04  22:15:09  shawn
 * Changes for FSSTAT call.
 *
 * Revision 1.21  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.22  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.21  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.20  1993/12/16  00:46:25  dplatt
 * Add preliminary support for BARF filesystems.
 *
 * Revision 1.19  1993/09/01  23:15:02  dplatt
 * Add zero-use-count tracking and limit
 *
 * Revision 1.18  1993/07/23  23:55:20  dplatt
 * Fix cache flags, and add include for folio.h.
 *
 * Revision 1.17  1993/06/15  20:15:34  dplatt
 * Don't cache file info for Macintosh /remote files
 *
 * Revision 1.16  1993/06/14  00:59:05  dplatt
 * Dragon beta release
 *
 * Revision 1.15  1993/05/28  21:41:53  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.14  1993/05/08  01:05:08  dplatt
 * Add NVRAM support and recover RCS
 *
 * Revision 1.13  1993/04/23  21:48:49  dplatt
 * Remove unneeded and wasteful IOReq
 *
 * Revision 1.12  1993/03/16  06:36:03  dplatt
 * Functional Freeze release
 *
 * Revision 1.11  1993/02/11  19:37:54  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.10  1993/02/09  01:50:02  dplatt
 * Reorganize and fix for developer release
 *
 * Revision 1.9  1993/01/05  20:56:45  dplatt
 * CES changes redux
 *
 * Revision 1.8  1992/12/22  09:38:10  dplatt
 * Fix file-folio tags
 *
 * Revision 1.7  1992/12/22  07:57:07  dplatt
 * Magneto 3 changes and CD support
 *
 * Revision 1.6  1992/12/08  05:58:20  dplatt
 * Magenta changes
 *
 * Revision 1.5  1992/11/06  01:11:56  dplatt
 * Make LoadProgram() into a library routine
 *
 * Revision 1.4  1992/10/20  06:00:13  dplatt
 * Blue changes
 *
 * Revision 1.3  1992/10/16  01:20:27  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.2  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:44:19  dplatt
 * Initial revision
 *

 *****/

#ifndef __H_FILESYSTEM
#define __H_FILESYSTEM

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Kernel data structures for filesystem access
*/

# include "io.h"
# include "discdata.h"
# include "device.h"
# include "driver.h"
# include "folio.h"
# include "timer.h"
# include "semaphore.h"

#pragma include_only_once

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
#define DEVICE_SORT_COUNT  1

#define ALIAS_NAME_MAX_LEN     31
#define ALIAS_VALUE_MAX_LEN    255

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
#define ER_Fs_LAST             13

struct HighLevelDisk;
struct File;

typedef struct HighLevelDisk HighLevelDisk;
typedef struct File File;

typedef int32 (*FileDriverQueueit)
     (HighLevelDisk *theDevice, IOReq *theRequest);
typedef void (*FileDriverHook) (HighLevelDisk *theDevice);
typedef IOReq * (*FileDriverEA) (IOReq *theRequest);
typedef void (*FileDriverAbort) (IOReq *theRequest);
typedef Item (*FileDriverEntry) (File *parent, char *name);
typedef Err (*FileDriverAlloc) (IOReq *theRequest);
typedef void (*FileDriverClose) (File *theFile);

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
  uchar              hdi_DeviceType;
  uchar              hdi_DeviceBusy;
  uchar              hdi_RequestPriority;
  uchar              hdi_RunningPriority;
  FileDriverQueueit  hdi_QueueRequest;
  FileDriverHook     hdi_ScheduleIO;
  FileDriverHook     hdi_StartIO;
  FileDriverAbort    hdi_AbortIO;
  FileDriverEA       hdi_EndAction;
  FileDriverEntry    hdi_CreateEntry;
  FileDriverEntry    hdi_DeleteEntry;
  FileDriverAlloc    hdi_AllocSpace;
  FileDriverClose    hdi_CloseFile;
  List               hdi_RequestsToDo;
  List               hdi_RequestsRunning;
  List               hdi_RequestsDeferred;
  Item               hdi_RawDeviceItem;
  uchar              hdi_RawDeviceUnit;
  uchar              hdi_rfu[3];
};

/*
  OptimizedDisk defines a high-level disk which resides on a real
  mass-storage medium, and whose performance characteristics are such
  that doing seek optimization is important.
*/

typedef struct OptimizedDisk {
  HighLevelDisk      odi;
  struct FileIOReq **odi_RequestSort;
  IOReq             *odi_RawDeviceRequest;
  int32              odi_RequestSortLimit;
  uint32             odi_BlockSize;
  uint32             odi_BlockCount;
  uint32             odi_LastBlockRead;
  uint32             odi_RawDeviceBlockOffset;
  uchar              odi_DeferredPriority;
} OptimizedDisk;

#define DRIVER_FLAW_MASK              0x000000FF
#define DRIVER_FLAW_SHIFT             24
#define DRIVER_BLOCK_MASK             0x00FFFFFF
#define DRIVER_FLAW_SOFTLIMIT         0x000000FE
#define DRIVER_FLAW_HARDERROR         0x000000FF

#define DEVICE_BOINK                  0x80

#define MAC_MAX_NAME_LEN              256
#define MAX_ZERO_USE_FILES            16
#define CLEANUP_TIMER_LIMIT           8

#ifndef notAFileErr
# define notAFileErr                   -1302
#endif

/*
  MacDisk defines a high-level "disk" which is actually mapped onto
  a Macintosh folder/file hierarchy.
*/

typedef struct MacFileInfo {
  uint32     mfi_Length;
  uint32     mfi_Info;
  uint32     mfi_NumEntries;
  uint32     rfu[5];
} MacFileInfo;

#define MAC_PATH_IS_DIRECTORY    0x00000001

typedef struct MacDirectoryEntry {
  MacFileInfo   mde_Info;
  char          mde_Name[64];
} MacDirectoryEntry;

typedef struct MacDisk {
  HighLevelDisk      mdi;
  IOReq             *mdi_RawDeviceRequest;
  uchar              mdi_CurrentPathName[MAC_MAX_NAME_LEN];
  MacDirectoryEntry  mdi_DirectoryEntry;
} MacDisk;

/*
  LinkedMemDisk defines a high-level "disk" which consists of a
  doubly-linked list of storage blocks in memory (RAM, ROM, NVRAM,
  or out on a gamesaver cartridge.  LinkedMemDisks have a standard
  Opera label at offset 0, with a type code of 2.  The linked list
  normally begins immediately after the label;  its offset is given
  by the zero'th avatar of the root directory.  LinkedMemDisks are,
  by definition, flat file systems and cannot contain directories.
*/

typedef struct LinkedMemBlock {
  uint32             lmb_Fingerprint; /* block fingerprint identification */
  uint32             lmb_FlinkOffset; /* 0 means first block */
  uint32             lmb_BlinkOffset; /* 0 means last block */
  uint32             lmb_BlockCount; /* includes entire header */
  uint32             lmb_HeaderBlockCount;
} LinkedMemBlock;

#define FINGERPRINT_FILEBLOCK   0xBE4F32A6
#define FINGERPRINT_FREEBLOCK   0x7AA565BD
#define FINGERPRINT_ANCHORBLOCK 0x855A02B6

#define LINKED_MEM_SLACK 32
/* the copy-buffer size must be at least FILESYSTEM_MAX_NAME_LEN! */
#define LINKED_MEM_COPY_BUFFER_SIZE 256

typedef struct LinkedMemFileEntry {
  LinkedMemBlock     lmfe;
  uint32             lmfe_ByteCount;
  uint32             lmfe_UniqueIdentifier;
  uint32             lmfe_Type;
  char               lmfe_FileName[32];
} LinkedMemFileEntry;

enum LinkedMemDiskFSM 
{
  LMD_Idle,
  LMD_Done,
  LMD_Fault,
  LMD_Initialization,
  LMD_CheckIsThisLast,
  LMD_CheckSuccessor,
  LMD_CutTheSlack,
  LMD_CutOffExcess,
  LMD_GetOldBackLink,
  LMD_FixOldBackLink,
  LMD_SuccessfulChomp,
  LMD_TryNewBlock,
  LMD_ExamineNewBlock,
  LMD_ReadToCopy,
  LMD_WriteCopiedData,
  LMD_CopyDone,
  LMD_FetchHeader,
  LMD_MarkItFree,
  LMD_BackUpOne,
  LMD_ScanAhead,
  LMD_AttemptMerge,
  LMD_FixFlink,
  LMD_DoneDeleting,
  LMD_InitScan,
  LMD_ExamineEntry,
  LMD_ReadToSetEOF,
  LMD_WriteWithNewEOF,
  LMD_ReadToSetType,
  LMD_WriteWithNewType,
  LMD_FsStat,
  LMD_ExtendEOF
};

typedef struct LinkedMemDisk {
  HighLevelDisk         lmd;
  IOReq                *lmd_RawDeviceRequest;
  uint32                lmd_BlockSize;
  uint32                lmd_BlockCount;
  uint32                lmd_RawDeviceBlockOffset;
  uint32                lmd_CurrentEntryOffset;
  int32                 lmd_CurrentEntryIndex;
  struct File          *lmd_CurrentFileActingOn;
  int32                 lmd_ThisBlockCursor;
  int32                 lmd_ThisBlockIndex;
  int32                 lmd_OtherBlockCursor;
  int32                 lmd_MergeBlockCursor;
  int32                 lmd_HaltCursor;
  int32                 lmd_DesiredSize;
  int32                 lmd_ContentOffset;
  int32                 lmd_BlocksToCopy;
  int32                 lmd_BlocksToRead;
  int32                 lmd_CopyBlockSize;
  int32                 lmd_FileHeaderBlockSize;
  int32                 lmd_FileHeaderByteSize;
  enum LinkedMemDiskFSM lmd_FSM;
  LinkedMemFileEntry    lmd_ThisBlock;
  LinkedMemFileEntry    lmd_OtherBlock;
  uchar                 lmd_CopyBuffer[LINKED_MEM_COPY_BUFFER_SIZE];
} LinkedMemDisk;

typedef struct FileSystem {
  ItemNode         fs;
  char             fs_FileSystemName[FILESYSTEM_MAX_NAME_LEN];
  HighLevelDisk   *fs_Device;
  uint32           fs_Flags;
  uint32           fs_VolumeBlockSize;
  uint32           fs_VolumeBlockCount;
  uint32           fs_VolumeUniqueIdentifier;
  uint32           fs_RootDirectoryBlockCount;
  int32            fs_DeviceBlocksPerFilesystemBlock;
  struct File     *fs_RootDirectory;
} FileSystem;

#define FILESYSTEM_IS_READONLY 0x00000002
#define FILESYSTEM_IS_IMPORTED 0x00000004
#define FILESYSTEM_IS_OFFLINE  0x00000008
#define FILESYSTEM_NEEDS_VBLS  0x00000020
#define FILESYSTEM_CACHEWORTHY 0x00000040
#define FILESYSTEM_NOT_READY   0x00000080

#define FILE_DEVICE_OPTIMIZED_DISK 1
#define FILE_DEVICE_OPENFILE       2
#define FILE_DEVICE_MAC_DISK       3
#define FILE_DEVICE_LINKED_STORAGE 4
#define FILE_DEVICE_BARF_DISK      5

/* #define	FS_DIRSEMA */
struct File {
  ItemNode         fi;
  char             fi_FileName[FILESYSTEM_MAX_NAME_LEN];
  FileSystem      *fi_FileSystem;
  struct File     *fi_ParentDirectory;
#ifdef	FS_DIRSEMA
  Item		   fi_DirSema;
#endif	/* FS_DIRSEMA */
  uint32           fi_UniqueIdentifier;
  uint32           fi_Type;
  uint32           fi_Flags;
  uint32           fi_UseCount;
  uint32           fi_BlockSize;
  uint32           fi_ByteCount;
  uint32           fi_BlockCount;
  uint32           fi_Burst;
  uint32           fi_Gap;
  uint32           fi_LastAvatarIndex;
  uint32           fi_FilesystemSpecificData;
  uint32           fi_AvatarList[1];
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

#define FILE_TYPE_DIRECTORY    0x2a646972
#define FILE_TYPE_LABEL        0x2a6c626c

#define FILE_HIGHEST_AVATAR    255

typedef struct OpenFile {
  Device             ofi;
  uchar              ofi_DeviceType;
  uchar              ofi_pad[3];
  File              *ofi_File;
  uint32             ofi_Flags;
  int32              ofi_NextBlock;
  uint32             ofi_BufferUseCount;   /* if zero, MM may scavenge */
  void              *ofi_RegisteredBuffer;
  IOReq             *ofi_InternalIOReq;
  uint32             ofi_BufferBlocks;
  uint32             ofi_BufferBlocksAvail;
  uint32             ofi_BufferBlocksFilled;
} OpenFile;

#define OFILE_CONTINUOUS_IO         0x00000001
#define OFILE_CONTINUOUS_BLOCKED    0x00000002
#define OFILE_IS_SHARED             0x00000004

typedef struct BufferedFile {
  ItemNode           bfi;
  Item               bfi_OpenFile;
  void              *bfi_Buffer;
  uint32             bfi_BufferBlocks;
  uint32             bfi_BufferBlocksAvail;
  uint32             bfi_BufferBlocksFilled;
  uint32             bfi_BufferSeekBase;
  uint32             bfi_SeekNow;
} BufferedFile;

typedef struct FileIOReq {
  IOReq          fio;
  uint32         fio_Flags;
  uint32         fio_BlockCount;
  uint32         fio_BlockBurst;
  uint32         fio_DevBlocksPerFileBlock;
  uint32         fio_BlockInterleave;
  uint32         fio_AbsoluteBlockNumber;
  uint32         fio_AvatarIndex;
} FileIOReq;

#define FIO_CONTINUOUS_IO           0x00000001
#define FIO_INTERLEAVE_IO           0x00000002

typedef struct FileStatus {
  DeviceStatus   fs;
  uint32         fs_ByteCount;
} FileStatus;

typedef struct IoCacheEntry {
  Node           ioce;
  uint32         ioce_FilesystemUniqueIdentifier;
  uint32         ioce_FileUniqueIdentifier;
  uint32         ioce_FileBlockOffset;
  uint32         ioce_Priority;
  uint32         ioce_CacheFormat;
  uint32         ioce_CacheFirstIndex;
  uint32         ioce_CacheMiscValue;
  uint32         ioce_CachedBlockSize;
  void          *ioce_CachedBlock;
} IoCacheEntry;

#define CACHE_PRIO_HIT 16
#define CACHE_PRIO_MISS 8

#define CACHE_OPERA_DIRECTORY         1
#define CACHE_BARF_DIRECTORY          2

typedef struct IoCache {
  Node           ioc;
  uint32         ioc_EntriesAllowed;
  uint32         ioc_EntriesPresent;
  List           ioc_CachedBlocks;
} IoCache;

typedef enum SchedulerSweepDirection {
  BottomIsCloser = 1,
  TopIsCloser = 2
} SchedulerSweepDirection;

typedef struct Alias {
  ItemNode       a;
  uchar         *a_Value;
} Alias;

typedef struct FileFolio {
  Folio          ff;
  File          *ff_Root;
  List           ff_Devices;
  List           ff_Filesystems;
  List           ff_Files;
  List           ff_OpenFiles;
  int32          ff_NextUniqueID;
  int32          ff_OpensSinceCleanup;
  struct {
      Task          *ffd_Task;
      TimerDevice   *ffd_TimerDevice;
      uint32         ffd_QueuedSignal;    /* "first I/O queued for a device" */
      uint32         ffd_WaitingSignal;   /* "I'm need an I/O to wake me     */
      uint32         ffd_RescheduleSignal;/* "Did last I/O for a device      */
      uint32         ffd_CDRomSignal;     /* "Reschedule CD-ROM"             */
    } ff_Daemon;
} FileFolio;

typedef struct FileFolioTaskData {
  File          *fftd_CurrentDirectory;
  File          *fftd_ProgramDirectory;
  uint32         fftd_ErrorCode;
  List           fftd_AliasList;
} FileFolioTaskData;

typedef struct FileSystemEntry {
  uint32     fse_Flags;
  char       fse_Name[FILESYSTEM_MAX_NAME_LEN];
} FileSystemEntry;

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

#ifndef	FS_DIRSEMA
#define LockDirSema(fp, msg)	/* no sema */
#define UnlockDirSema(fp, msg)	/* no sema */
#endif	/* FS_DIRSEMA */
#endif

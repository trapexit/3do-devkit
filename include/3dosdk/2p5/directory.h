#ifndef __H_DIRECTORY
#define __H_DIRECTORY

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: directory.h,v 1.7 1994/09/10 01:36:15 peabody Exp $
**
**  Folio data structures for semi-device-independent access to entries
**  in filesystem directories.
**
******************************************************************************/


typedef struct Directory {
  Item           dir_OpenFileItem;
  Item           dir_IOReqItem;
  IOReq         *dir_IOReq;
  uint32         dir_Flags;
  uint32         dir_BlockSize;
  uint32         dir_BlockCount;
  int32          dir_BlockNumber;
  uint32         dir_BlockOffset;
  uint32         dir_EntryNum;
  char          *dir_BlockBuf;
} Directory;

typedef struct DirectoryEntry {
  uint32     de_Flags;
  uint32     de_UniqueIdentifier;
  uint32     de_Type;
  uint32     de_BlockSize;
  uint32     de_ByteCount;
  uint32     de_BlockCount;
  uint32     de_Burst;
  uint32     de_Gap;
  uint32     de_AvatarCount;
  char       de_FileName[FILESYSTEM_MAX_NAME_LEN];
  uint32     de_Location;
} DirectoryEntry;

#endif /* __H_DIRECTORY */

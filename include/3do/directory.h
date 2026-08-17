#pragma once
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

#include "filesystem.h"
#include "io.h"
#include "types.h"


typedef struct Directory Directory;
struct Directory
{
  Item    dir_OpenFileItem;
  Item    dir_IOReqItem;
  IOReq  *dir_IOReq;
  u32  dir_Flags;
  u32  dir_BlockSize;
  u32  dir_BlockCount;
  s32   dir_BlockNumber;
  u32  dir_BlockOffset;
  u32  dir_EntryNum;
  char   *dir_BlockBuf;
};

typedef struct DirectoryEntry DirectoryEntry;
struct DirectoryEntry
{
  u32 de_Flags;
  u32 de_UniqueIdentifier;
  u32 de_Type;
  u32 de_BlockSize;
  u32 de_ByteCount;
  u32 de_BlockCount;
  u32 de_Burst;
  u32 de_Gap;
  u32 de_AvatarCount;
  char   de_FileName[FILESYSTEM_MAX_NAME_LEN];
  u32 de_Location;
};

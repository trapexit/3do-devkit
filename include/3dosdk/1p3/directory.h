/*****

$Id: directory.h,v 1.5 1994/01/21 00:26:16 dplatt Exp $

$Log: directory.h,v $
 * Revision 1.5  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.6  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.5  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.4  1993/05/08  01:05:08  dplatt
 * Add NVRAM support and recover RCS
 *
 * Revision 1.3  1993/02/11  19:37:54  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.2  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:44:13  dplatt
 * Initial revision
 *

 *****/

#ifndef __H_DIRECTORY
#define __H_DIRECTORY

#pragma include_only_once


/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Folio data structures for semi-device-independent access to entries
  in filesystem directories.
*/

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

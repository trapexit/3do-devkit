#ifndef __H_FILESTREAM
#define __H_FILESTREAM

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: filestream.h,v 1.7 1994/09/10 01:36:15 peabody Exp $
**
**  Folio data structures for bytestream-oriented file access
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#include "filesystem.h"

enum SeekOrigin {
  SEEK_NOT = 0,
  SEEK_SET = 1,
  SEEK_CUR = 2,
  SEEK_END = 3
  };

#define FILESTREAM_BUFFER_MIN_BLOCKS 2
#define FILESTREAM_BUFFER_MIN_BYTES  256

typedef struct Stream {
  Item            st_OpenFileItem;
  IOReq          *st_IOReq;
  uchar          *st_Buffer;
  int32           st_BufferLen;
  int32           st_NextBufByteOffset;
  int32           st_BufBytesAvail;
  uint32          st_FileOffset;
  int32           st_FileBlockSize;
  int32           st_FileBlockCount;
  int32           st_FileLength;
  int32           st_CursorPosition;
  int32           st_SeekTo;
  uchar           st_IOInProgress;
  uchar           st_HadError;
  enum SeekOrigin st_SeekOrigin;
} Stream;


/*****************************************************************************/


#endif /* __H_FILESTREAM_H */

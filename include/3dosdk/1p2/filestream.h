/*****

$Id: filestream.h,v 1.5 1994/01/21 00:26:16 dplatt Exp $

$Log: filestream.h,v $
 * Revision 1.5  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.6  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.5  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.4  1993/06/14  00:59:05  dplatt
 * Dragon beta release
 *
 * Revision 1.3  1992/12/08  05:58:20  dplatt
 * Magenta changes
 *
 * Revision 1.2  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:44:17  dplatt
 * Initial revision
 *

 *****/

#ifndef __H_FILESTREAM
#define __H_FILESTREAM

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#include "filesystem.h"

/*
  Folio data structures for bytestream-oriented file access
*/

#pragma include_only_once

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

#endif

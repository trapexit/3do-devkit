#pragma once

/****************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: iff_fs_tools.h,v 1.17 1994/09/10 00:17:48 peabody Exp $
 **
 **  Include file for simple IFF reader using 3DO FIle System
 **
 ****************************************************************************/

#include "extern_c.h"

#include "types.h"
#include "stdarg.h"
#include "string.h"
#include "driver.h"
#include "folio.h"
#include "list.h"

#ifndef SEEK_END
#include "filestream.h"
#endif
#include "filestreamfunctions.h"

#include "flex_stream.h"

typedef struct
{
  FlexStream iffc_FlexStream;   /* For reading file or memory. */
  s32  iffc_length;
  s32  iffc_NextPos;          /* Position at end of next chunk */
  s32 (*iffc_ChunkHandler)(); /* Callback function for Parser */
  s32 (*iffc_FormHandler)();  /* Callback function for Parser */
  void  *iffc_UserContext;
  char  *iffc_LastChanceDir;    /* Directory to look in if not found. */
  s32  iffc_Level;            /* Level of recursion in file. */
} iff_control;

#if 0
#define IFFWRITE(buf,len)                                               \
  Result = fwrite ((char *) buf, 1, len, iffc->iffc_FileStream);        \
  iffc->iffc_length += len
#endif

#define EVENUP(n) { if(n & 1) n++; }

EXTERN_C_BEGIN

/* function prototypes  */
s32 iffScanChunks(iff_control *iffc, u32 Length);
s32 iffParseFile(iff_control *iffc, const char *FilePathName);
s32 iffParseImage(iff_control *iffc, char *Image, s32 NumBytes);
s32 iffParseChunk(iff_control *iffc);
s32 iffOpenFileRead(iff_control *iffc, const char *filename);
s32 iffCloseFile(iff_control *iffc);

s32 iffSkipChunkData(iff_control *iffc, u32 numbytes);
s32 iffReadChunkData(iff_control *iffc,
                       void *data, u32 numbytes);
s32 iffReadChunkHeader(iff_control *iffc,
                         u32 *type, u32 *size);
s32 iffReadForm(iff_control *iffc, u32 *type);
s32 iffReadStream( iff_control *iffc, char *Addr, s32 NumBytes);
s32 iffSeekStream( iff_control *iffc, s32 Offset, enum SeekOrigin Mode);

s32 iffCloseImage(iff_control *iffc);
s32 iffOpenImage(iff_control *iffc, char *Image, s32 NumBytes);

#if 0
s32 iffOpenFileWrite(iff_control *iffc, const char *filename);
s32 iffBeginForm(iff_control *iffc, u32 type);
s32 iffEndForm(iff_control *iffc);
s32 iffWriteChunk(iff_control *iffc, u32 type,
                    void *data, s32 numbytes);
#endif

#define	MAKE_ID(a,b,c,d)                                                \
  ((u32) (a)<<24 | (u32) (b)<<16 | (u32) (c)<<8 | (u32) (d))

/*
 * Universal IFF identifiers.
 */
#define	ID_FORM	MAKE_ID('F','O','R','M')
#define	ID_XREF	MAKE_ID('X','R','E','F')

/* Old obsolete names. */
#if 0
s32 iff_scan_chunks(iff_control *iffc, u32 Length);
s32 iff_parse_file(iff_control *iffc, char *FilePathName);
s32 iff_parse_chunk(iff_control *iffc);
s32 iff_open_file_read(iff_control *iffc, char *filename);
s32 iff_open_file_write(iff_control *iffc, char *filename);
s32 iff_close_file(iff_control *iffc);
s32 iff_begin_form(iff_control *iffc, u32 type);
s32 iff_end_form(iff_control *iffc);
s32 iff_write_chunk(iff_control *iffc, u32 type,
                      void *data, s32 numbytes);

s32 iff_skip_chunk_data(iff_control *iffc, u32 numbytes);
s32 iff_read_chunk_data(iff_control *iffc,
                          void *data, u32 numbytes);
s32 iff_read_chunk_header(iff_control *iffc,
                            u32 *type, u32 *size);
s32 iff_read_form(iff_control *iffc, u32 *type);
#endif

EXTERN_C_END

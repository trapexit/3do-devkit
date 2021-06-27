/* $Id: flex_stream.h,v 1.5 1994/02/09 02:04:35 limes Exp $ */
#pragma include_only_once
/*
** Include file for flexible stream I/O
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#ifndef _flex_stream_h
#define _flex_stream_h

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

typedef struct FlexStream
{
	Stream *flxs_FileStream;
/* The following fields are used for parsing from an in memory image. */
	char   *flxs_Image;            /* Image in memory. */
	int32   flxs_Cursor;           /* Position in image. */
	int32   flxs_Size;             /* Size of image. */
} FlexStream;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 CloseFlexStreamFile( FlexStream *flxs );
int32 CloseFlexStreamImage( FlexStream *flxs );
int32 OpenFlexStreamFile( FlexStream *flxs, char *filename );
int32 OpenFlexStreamImage( FlexStream *flxs, char *Image, int32 NumBytes );
int32 ReadFlexStream( FlexStream *flxs, char *Addr, int32 NumBytes );
int32 SeekFlexStream( FlexStream *flxs, int32 Offset, enum SeekOrigin Mode );
int32 TellFlexStream( FlexStream *flxs );
char *TellFlexStreamAddress( FlexStream *flxs );
char *LoadFileImage( char *Name, int32 *NumBytesPtr );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

#ifndef __FLEX_STREAM_H
#define __FLEX_STREAM_H

#pragma force_top_level
#pragma include_only_once


/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: flex_stream.h,v 1.8 1994/09/10 00:17:48 peabody Exp $
**
**  Flexible stream I/O
**
****************************************************************************/


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
int32 OpenFlexStreamFile( FlexStream *flxs, const char *filename );
int32 OpenFlexStreamImage( FlexStream *flxs, char *Image, int32 NumBytes );
int32 ReadFlexStream( FlexStream *flxs, char *Addr, int32 NumBytes );
int32 SeekFlexStream( FlexStream *flxs, int32 Offset, enum SeekOrigin Mode );
int32 TellFlexStream( const FlexStream *flxs );
char *TellFlexStreamAddress( const FlexStream *flxs );
char *LoadFileImage( const char *Name, int32 *NumBytesPtr );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __FLEX_STREAM_H */

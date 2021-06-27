/*****

$Id: filestreamfunctions.h,v 1.3 1994/01/21 00:26:16 dplatt Exp $

$Log: filestreamfunctions.h,v $
 * Revision 1.3  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.3  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.2  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:44:18  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1992.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#ifndef __H_FILESTREAMFUNCTIONS
#define __H_FILESTREAMFUNCTIONS
#include "filestream.h"

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

#pragma include_only_once

/*
  Function prototypes for bytestream-oriented file access
*/

extern Stream *OpenDiskStream(char *theName, int32 bSize);
extern int32 SeekDiskStream(Stream *theStream, int32 offset,
			       enum SeekOrigin whence);
extern void CloseDiskStream(Stream *theStream);
extern int32 ReadDiskStream(Stream *theStream, char *buffer, int32 nBytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

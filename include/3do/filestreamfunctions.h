#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: filestreamfunctions.h,v 1.6 1994/09/10 01:36:15 peabody Exp $
**
**  Function prototypes for bytestream-oriented file access
**
******************************************************************************/

#include "extern_c.h"

#include "filestream.h"

EXTERN_C_BEGIN

extern Stream *OpenDiskStream(char *theName,
                              int32 bSize);
extern int32 SeekDiskStream(Stream *theStream,
                            int32   offset,
                            enum SeekOrigin whence);
extern void CloseDiskStream(Stream *theStream);
extern int32 ReadDiskStream(Stream *theStream,
                            char *buffer,
                            int32 nBytes);

EXTERN_C_END

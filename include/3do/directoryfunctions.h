#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: directoryfunctions.h,v 1.6 1994/09/10 01:36:15 peabody Exp $
**
**  Function prototypes for the userland interface to the directory walker.
**
******************************************************************************/

#include "extern_c.h"

#include "types.h"

EXTERN_C_BEGIN

Directory *OpenDirectoryItem(Item openFileItem);
Directory *OpenDirectoryPath(char *thePath);
int32      ReadDirectory (Directory *dir, DirectoryEntry *de);
void       CloseDirectory (Directory *dir);

EXTERN_C_END

/*****************************************************************************/

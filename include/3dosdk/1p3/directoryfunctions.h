/*****

$Id: directoryfunctions.h,v 1.4 1994/01/21 00:26:16 dplatt Exp $

$Log: directoryfunctions.h,v $
 * Revision 1.4  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.4  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.3  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.2  1992/09/16  20:16:16  dplatt
 * Copyrights
 *
 * Revision 1.1  1992/09/11  00:44:13  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1992.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Function prototypes for the userland interface to the directory
  walker.
*/

#ifndef __H_DIRECTORYFUNCTIONS
# define __H_DIRECTORYFUNCTIONS

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

#pragma include_only_once

Directory *OpenDirectoryItem(Item openFileItem);
Directory *OpenDirectoryPath(char *thePath);
int32 ReadDirectory (Directory *dir, DirectoryEntry *de);
void CloseDirectory (Directory *dir);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __H_DIRECTORYFUNCTIONS */

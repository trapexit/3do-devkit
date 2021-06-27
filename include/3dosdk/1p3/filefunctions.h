/*****

$Id: filefunctions.h,v 1.17 1994/03/26 01:26:56 dplatt Exp $

$Log: filefunctions.h,v $
 * Revision 1.17  1994/03/26  01:26:56  dplatt
 * Move CodeHandle typedef to types.h.
 *
 * Revision 1.16  1994/03/26  01:13:41  dplatt
 * filefunctions.h now requires filesystem.h as a prerequisite.
 *
 * Revision 1.15  1994/03/25  23:03:39  dplatt
 * Change UnloadCode() to return an Err rather than void - otherwise the
 * glue-builder barfs.
 *
 * Revision 1.14  1994/03/25  22:47:27  dplatt
 * Add Martin's neat new code-management routines.
 *
 * Revision 1.13  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.13  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.12  1993/06/14  00:59:05  dplatt
 * Dragon beta release
 *
 * Revision 1.11  1993/05/31  03:32:14  dplatt
 * Add a SWI to unmount file systems.
 *
 * Revision 1.10  1993/05/28  21:41:53  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.9  1993/05/08  01:05:08  dplatt
 * Add NVRAM support and recover RCS
 *
 * Revision 1.8  1993/03/16  06:36:03  dplatt
 * Functional Freeze release
 *
 * Revision 1.7  1992/12/22  07:57:07  dplatt
 * Magneto 3 changes and CD support
 *
 * Revision 1.6  1992/12/08  05:58:20  dplatt
 * Magenta changes
 *
 * Revision 1.5  1992/11/06  01:11:56  dplatt
 * Make LoadProgram() into a library routine
 *
 * Revision 1.4  1992/10/16  01:20:27  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.3  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.2  1992/09/16  20:16:16  dplatt
 * Copyrights
 *
 * Revision 1.1  1992/09/11  00:44:16  dplatt
 * Initial revision
 *

 *****/

#ifndef __H_FILEFUNCTIONS
#define __H_FILEFUNCTIONS

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

#pragma include_only_once

/*
  Copyright New Technologies Group, 1992.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Function prototypes for the userland interfaces to the File Folio.
*/

#define FILEFOLIOSWI 0x00030000

extern Item  __swi(FILEFOLIOSWI+0) OpenDiskFile(char *path);
extern int32 __swi(FILEFOLIOSWI+1) CloseDiskFile(Item fileItem);
extern Item  __swi(FILEFOLIOSWI+4) MountFileSystem(Item deviceItem,
						   int32 unit,
						   uint32 blockOffset);
extern Item  __swi(FILEFOLIOSWI+5) OpenDiskFileInDir(Item dirItem, char *path);
extern Item  __swi(FILEFOLIOSWI+6) MountMacFileSystem(char *path);
extern Item  __swi(FILEFOLIOSWI+7) ChangeDirectory(char *path);
extern Item  __swi(FILEFOLIOSWI+8) GetDirectory(char *pathBuf, int pathBufLen);
extern Item  __swi(FILEFOLIOSWI+9) CreateFile(char *path);
extern Err   __swi(FILEFOLIOSWI+10) DeleteFile(char *path);
extern Item  __swi(FILEFOLIOSWI+11) CreateAlias(char *aliasPath,
						char *realPath);
extern int32 __swi(FILEFOLIOSWI+12) LoadOverlay(void *mumble); /* internal */
extern Err   __swi(FILEFOLIOSWI+13) DismountFileSystem(char *name);


extern Item                        LoadProgram(char *path);
extern Item                        LoadProgramPrio(char *path, int32 prio);
extern Err                         LoadCode(char *fileName, CodeHandle *code);
extern Err                         UnloadCode(CodeHandle code);
extern int32                       ExecuteAsSubroutine(CodeHandle code,
						       uint32 argc,
						       char **argv);
extern Item                        ExecuteAsThread(CodeHandle code,
						   uint32 argc,
						   char **argv,
						   char *threadName,
						   int32 priority);

extern void                        InitFileFolioGlue(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __H_FILEFUNCTIONS */

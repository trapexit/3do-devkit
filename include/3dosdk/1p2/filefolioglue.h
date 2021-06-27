#ifndef __H_FILEFOLIOGLUE
#define __H_FILEFOLIOGLUE

/*****

$Id: filefolioglue.h,v 1.2 1994/01/21 00:26:16 dplatt Exp $

$Log: filefolioglue.h,v $
 * Revision 1.2  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.3  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.2  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.1  1993/06/29  17:31:39  dplatt
 * Initial revision
 *

*****/

#pragma include_only_once

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Utility glue for vectoring off into the filesystem
*/

#define FolioGlue(name,func,params,args,type) \
type name params \
{ \
  int32 (* *_f)(); \
  _f =  (int32 (* *)())GetFileFolio(); \
  return (type) (*_f[func])args; \
}

#define FolioGlueVoid(name,func,params,args) \
void name params \
{ \
  int32 (* *_f)(); \
  _f =  (int32 (* *)())GetFileFolio(); \
  (*_f[func])args; \
}

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

extern FileFolio *GetFileFolio(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

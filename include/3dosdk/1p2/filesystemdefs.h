/*****

$Id: filesystemdefs.h,v 1.2 1994/01/21 00:26:02 dplatt Exp $

$Log: filesystemdefs.h,v $
 * Revision 1.2  1994/01/21  00:26:02  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.3  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.2  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.1  1992/09/11  00:44:19  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#ifndef __H_FILESYSTEMDEFS
#define __H_FILESYSTEMDEFS

#pragma include_only_once

/*
  Kernel data definitions for the filesystem and I/O driver kit
*/

extern FileFolio *fileFolio;
extern Driver *fileDriver;

#endif

/*******************************************************************************************
 *	File:			Weaver.h
 *
 *	Contains:		definitions for Weaver.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/25/93	jb		Add MAX_MARKERS_FLAG_STRING
 *	10/20/93	jb		Add ASCII TAB to the whitespace list of
 *characters 7/11/93		jb		Add MARKEROUT_FLAG_STRING
 *	6/27/93		jb		Add IOBUFSIZE_FLAG_STRING
 *	6/17/93		jb		Make names of command line flag strings
 *more mnemonic. 5/15/93		jb		New today. Copied
 *'StreamChunk' struct from DataStream.h and added the 'streamChunkTime' common
 *time stamp field.
 *
 *******************************************************************************************/

#ifndef __WEAVER_H__
#define __WEAVER_H__

#define BASETIME_FLAG_STRING                                                  \
  "-b" /* flag for beginning time for output stream */
#define STREAMBLK_FLAG_STRING "-s" /* flag for size of stream blocks */
#define MEDIABLK_FLAG_STRING "-m"  /* flag for size of media blocks */
#define OUTDATA_FLAG_STRING "-o"   /* flag for output data file name */
#define VERBOSE_FLAG_STRING "-v"   /* flag to enable verbose output */
#define IOBUFSIZE_FLAG_STRING                                                 \
  "-iobs" /* flag for I/O block size specification */
#define MAX_MARKERS_FLAG_STRING                                               \
  "-mm" /* flag for max number of markers to allow */

#define IsWhiteSpace(c)                                                       \
  ((c == ' ') || (c == '\t') || (c == 0x0A) || (c == 0x0D))

typedef long (*CallBackProcPtr) ();

#endif /* __WEAVER_H__ */

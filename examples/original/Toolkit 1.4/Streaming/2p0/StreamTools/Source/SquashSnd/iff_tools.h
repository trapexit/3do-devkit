/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1
 *with
 **							squareroot-delta-exactencoding and
 *write an AIFC file.
 **							Throw out all chunks but SSND FORM
 *COMM MARK INST FVER
 **
 **	This File:		iff_tools.h
 **
 ** Contains:		Include file for simple IFF reader
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and
 *proprietary *	information of the 3DO Company and shall not be used by any
 *Person or for any *	purpose except as expressly authorized in writing by
 *the 3DO Company.
 **
 **
 **  History:
 **  -------
 **	 6/17/93	RDG		version 1.0
 **  6/17/93	RDG		added iff_getpos
 **	 6/15/93	RDG		version .12
 **	 6/15/93	RDG		modify to use MacFileIO routines
 **	 2/16/93	RDG		ported to back to MPW 3.2.3 and release
 *first version .10
 **  2/15/93	RDG		added iff_begin_snd and iff_end_snd to more
 *easily handle SSND
 **						 chunk size... also modified the
 *iff_control struct by adding
 **						 member iffc_ssnd_pos to hold file
 *position of the ssnd data *	 2/7/93		RDG		added
 *uint,uint32,int32 typedefs
 **	 2/7/93		RDG		added iff_get_16 and put_8
 **	 2/4/93		RDG		ported to think C
 **
 **************************************************************************************/

#ifndef __IFFTOOLS_H__
#define __IFFTOOLS_H__

#include <MacFileIO.h>
#include <aiff.h>

typedef unsigned int uint;
typedef unsigned long uint32;
typedef long int32;

typedef struct
{
  long iffc_fileid;
  int32 iffc_length;
  long iffc_ssnd_pos; /* position of start of SSND chunk */
} iff_control;

typedef iff_control *iff_control_ptr;

/* function prototypes  */
long iff_open_file (iff_control *iffc, char *filename);
long iff_create_file (iff_control *iffc, char *filename, long creator,
                      long fileType);
long iff_close_file (iff_control *iffc);
long iff_getpos (iff_control *iffc);
long iff_rewind (iff_control *iffc);
long iff_begin_form (iff_control *iffc, uint32 type);
long iff_end_form (iff_control *iffc);
long iff_write_chunk (iff_control *iffc, uint32 type, void *data,
                      int32 numbytes);

long iff_skip_chunk_data (iff_control *iffc, uint32 numbytes);
long iff_read_chunk_data (iff_control *iffc, void *data, uint32 numbytes);
long iff_read_chunk_header (iff_control *iffc, uint32 *type, uint32 *size);
long iff_read_form (iff_control *iffc, uint32 *type);
long iff_begin_ssnd (iff_control *iffc);
long iff_end_ssnd (iff_control *iffc);
long iff_get_block (iff_control *iffc, char *sampleData, long size);
long iff_put_block (iff_control *iffc, char *sampleData, long size);
long iff_get_16 (iff_control *iffc, short *sample16);
long iff_put_8 (iff_control *iffc, char sample8);

char *TypeToString (uint32 type);

#define MAKE_ID(a, b, c, d)                                                   \
  ((uint32)(a) << 24 | (uint32)(b) << 16 | (uint32)(c) << 8 | (uint32)(d))

/*
 * Universal IFF identifiers.
 */
#define ID_FORM MAKE_ID ('F', 'O', 'R', 'M')
#define ID_SSND MAKE_ID ('S', 'S', 'N', 'D')
#define ID_AIFC MAKE_ID ('A', 'I', 'F', 'C')
#define ID_AIFF MAKE_ID ('A', 'I', 'F', 'F')
#define ID_MARK MAKE_ID ('M', 'A', 'R', 'K')
#define ID_INST MAKE_ID ('I', 'N', 'S', 'T')
#define ID_COMM MAKE_ID ('C', 'O', 'M', 'M')
#define ID_FVER MAKE_ID ('F', 'V', 'E', 'R')
#define ID_NONE MAKE_ID ('N', 'O', 'N', 'E')

#endif /* __IFFTOOLS_H__ */

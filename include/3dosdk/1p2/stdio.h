#ifndef __STDIO_H
#define __STDIO_H

/*****

$Id: stdio.h,v 1.8 1994/01/21 02:28:45 limes Exp $

$Log: stdio.h,v $
 * Revision 1.8  1994/01/21  02:28:45  limes
 * recover from rcs bobble
 *
 * Revision 1.9  1994/01/20  02:11:23  sdas
 * C++ compatibility - updated
 *
 * Revision 1.8  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.7  1992/12/19  03:10:11  dale
 * added printf and sprintf prototypes
 *
 * Revision 1.6  1992/12/16  21:56:13  dale
 * kernel printf
 *
 * Revision 1.5  1992/10/24  01:43:54  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1992, New Technologies Group, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/
/* Fake stdio.h */

#pragma force_top_level
#pragma include_only_once

#include "types.h"

#define EOF (-1)

#define FCB_BUFSIZE 1024

#define FCB_READ_MODE 1
#define FCB_WRITE_MODE 2

typedef struct
{
	int32 fcb_currentpos;
	int32 fcb_filesize;	/* total filesize */
	int32 fcb_bytesleft;	/* bytes left to read in file */
	int32 fcb_numinbuf;	/* number of unread chars in fcb_buffer */
	uint8 *fcb_cp;	/* ptr to next char in buffer */
	uint8 fcb_mode;
	uint8 pad[3];  
	uint8 *fcb_buffer;
	int32	fcb_buffsize;
	char fcb_filename[128];
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#define putchar(c) putc(c,stdout)
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

int32 system(char *s);
void perror(char *s);

extern FILE	*tmpfile(void);

FILE *fopen(char *filename, char *type);
int32 fread(void *ptr, int32 size, int32 nitems, FILE *stream);
int32 fclose (FILE *stream);
int32 getc(FILE *stream);
int32 putc(char c, FILE *stream);
int32 fputs(char *s, FILE *stream);
int32 fseek(FILE *stream, int32 offset, int32 prtname);
int32 ftell(FILE *stream);
int32 fflush(FILE *stream);
int32 ungetc(char s, FILE *stream);
int32 printf(const char *fmt, ...);
int32 sprintf(char *,const char *fmt, ...);

int32 MacExpect(char *buff, int32 maxchars);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STDIO_H */

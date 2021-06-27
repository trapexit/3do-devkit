#ifndef __STDIO_H
#define __STDIO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: stdio.h,v 1.14 1994/12/21 21:28:30 peabody Exp $
**
**  Standard C file IO definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __VARARGS_H
#include "varargs.h"
#endif


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

void perror(char *s);

extern FILE	*tmpfile(void);

FILE *fopen(char *filename, char *type);
int32 fread(void *ptr, int32 size, int32 nitems, FILE *stream);
int32 fwrite(const void *ptr, int32 size, int32 nitems, FILE *stream);
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
int32 vprintf(const char *fmt, va_list a);
int32 vsprintf(char *buf, const char *fmt, va_list a);
int32 remove(const char *);

int32 MacExpect(char *buff, int32 maxchars);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STDIO_H */

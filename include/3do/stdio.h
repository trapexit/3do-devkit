#pragma once

#include "extern_c.h"

#include "types.h"
#include "varargs.h"

#define EOF (-1)

#define FCB_BUFSIZE 1024

#define FCB_READ_MODE 1
#define FCB_WRITE_MODE 2

typedef struct
{
  s32  fcb_currentpos;
  s32  fcb_filesize;	/* total filesize */
  s32  fcb_bytesleft;	/* bytes left to read in file */
  s32  fcb_numinbuf;	/* number of unread chars in fcb_buffer */
  u8 *fcb_cp;          /* ptr to next char in buffer */
  u8  fcb_mode;
  u8  pad[3];
  u8 *fcb_buffer;
  s32  fcb_buffsize;
  char   fcb_filename[128];
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#define putchar(c) putc(c,stdout)

EXTERN_C_BEGIN

void perror(char *s);

extern FILE *tmpfile(void);

FILE *fopen(const char *filename, char *type);
s32 fread(void *ptr, s32 size, s32 nitems, FILE *stream);
s32 fwrite(const void *ptr, s32 size, s32 nitems, FILE *stream);
s32 fclose (FILE *stream);
s32 getc(FILE *stream);
s32 putc(char c, FILE *stream);
s32 fputs(char *s, FILE *stream);
s32 fseek(FILE *stream, s32 offset, s32 prtname);
s32 ftell(FILE *stream);
s32 fflush(FILE *stream);
s32 ungetc(char s, FILE *stream);
s32 printf(const char *fmt, ...);
s32 sprintf(char *,const char *fmt, ...);
s32 vprintf(const char *fmt, va_list a);
s32 vsprintf(char *buf, const char *fmt, va_list a);
s32 remove(const char *);

s32 MacExpect(char *buff, s32 maxchars);

EXTERN_C_END

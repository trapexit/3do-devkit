#pragma include_only_once

#include "extern_c.h"

#include "types.h"
#include "varargs.h"

#define EOF (-1)

#define FCB_BUFSIZE 1024

#define FCB_READ_MODE 1
#define FCB_WRITE_MODE 2

typedef struct
{
  int32  fcb_currentpos;
  int32  fcb_filesize;	/* total filesize */
  int32  fcb_bytesleft;	/* bytes left to read in file */
  int32  fcb_numinbuf;	/* number of unread chars in fcb_buffer */
  uint8 *fcb_cp;          /* ptr to next char in buffer */
  uint8  fcb_mode;
  uint8  pad[3];
  uint8 *fcb_buffer;
  int32  fcb_buffsize;
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

EXTERN_C_END

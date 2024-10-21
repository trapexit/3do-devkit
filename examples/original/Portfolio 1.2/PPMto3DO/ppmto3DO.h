/* includes */

#include <CType.h>
#include <Files.h>
#include <Math.h>
#include <Memory.h>
#include <StdLib.h>
#include <Strings.h>
#include <Types.h>
#include <stdio.h>

#define ASCII_CR (0x0D)
#define ASCII_LF (0x0A)
#define ASCII_SPACE (0x20)

#include <Form3DOmc.h>

typedef unsigned char uchar;

/* constants */

#define MAXWIDTH 320
#define MAXHEIGHT 240
#define MAXCLUTS MAXHEIGHT
#define CLUTADDR 0x01026000
#define PAGESIZE 512
#define PAGEMASK (PAGESIZE - 1)
#define FIRSTLINE 20

#define MAXCOLORS 32
#define HASHLIMIT 65536

main (int argc, char *argv[]);
void parse (char *option);
void usage (void);
void initpops (void);

void WriteOutImageChunk (void);
void WriteOutPDATChunk (void);
void WriteOutImageData (void);
void WriteOutImage (void);
void WriteOutTotalColors (void);
void ProcessColors (void);

void readline (int line, int pass);
void sortcolors (void);
void writeclutline (int line);

// Sorts must maintain same parameters
void evenspacing (long *pop, long *index, long *maxcount);
void z24BitSort (long *pop, long *index, long *maxcount);
void popsort (long *pop, long *index, long *maxcount);
void popspace (long *pop, long *index, long *maxcount);
void boxsort (long *pop, long *index, long *maxcount);
void boxpopsort (long *pop, long *index, long *maxcount);
void boxpoplengthsort (long *pop, long *index, long *maxcount);

void clutword (long value);
int myfgets (char *string, int count, FILE *stream);
long mystrtol (char *s);
int getxc (int c);
int getyc (int c);
void inithash (void);
void initCLUT (void);
void findhash (uchar r, uchar g, uchar b);
void ParsePPM (void);
void SetTheBlues (void);
void SetBackgroundColor (void);
void SetZ24Bit (void);

/* global variables */

int gOutwidth, gOutheight;
int gInwidth, gInheight, gRgbmax;
FILE *gFin, *gFout, *gFout2;
unsigned char ycc_array[MAXWIDTH * 4][3], gOutarray[MAXWIDTH * 4];
int gMaxcolorflag;
long gCLUTBase, gPalptr;
int gMaxcolors, gMincolor;
int gMaxblues, gMinblue;
Boolean gFixedFlag, g15541DisplayMode, gBGColorFlag, gYCCFlag, gZ24BitFlag,
    gSortBypassFlag, gPrintColorsFlag;
Boolean gLiteral16Flag, gOpffFlag;
long gTotalcolors;
#ifdef ALLOWCLUTS
long *pCLUTs;
int pCLUTindex;
#endif

void (*sortroutine) ();

unsigned char gBackred, gBackgreen, gBackblue;
long popy[256], popcr[256], popcb[256];
int yindex[256], crindex[256], cbindex[256];
long ycount = 0, crcount = 0, cbcount = 0;
long maxcount;

typedef struct colorhash
{
  uchar hit;
  uchar r;
  uchar g;
  uchar b;
} colorhash;

colorhash *ch;

/* macros */

#define min(a, b) ((a) > (b) ? b : a)
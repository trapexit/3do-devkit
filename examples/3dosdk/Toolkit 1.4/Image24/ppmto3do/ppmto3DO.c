
/*********************************************************\
*                                                         *
* File:     ppmto3DO.c                                    *
* By:       Stephen H. Landrum                            *
* Created:  16-Jul-91                                     *
*							  *
* Formerly "ppmopera.c"					  *
*							  *
* 9/15/92	jb	Split scan of height and width    *
*			to allow for CR/LF differences    *
*			on different platforms.		  *
*                                                         *
* 9/28/92	njc	Write CLUTs out in 3DO format     *
*							  *
*11/15/92	crm	changed name and command-line	  *
*			options for release to the	  *
*			outside world			  *
*                                                         *
* Program to convert images from ppm generic image        *
* to opera memory bitmap and CLUT list format             *
*                                                         *
* This document is proprietary and confidential           *
*                                                         *
* Copyright (c) 1991,1992 The 3DO Company		  *
*                                                         *
\*********************************************************/

/* enables some diagnostic output */
/* #define SHL_DEBUG */

/* includes */

#ifdef SUN
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <Files.h>
#include <Math.h>
#include <Memory.h>
#include <StdLib.h>
#include <Strings.h>
#include <Types.h>
#include <stdio.h>
#endif

#define ASCII_CR (0x0D)
#define ASCII_LF (0x0A)
#define ASCII_SPACE (0x20)

#include <Form3DOmac.h>
#include <item.h>
#include <list.h>
#include <nodes.h>
#include <types3do.h>
// #include <graphics.h>
#include <VDLmac.h>

typedef unsigned char uchar;

#define ALLOWCLUTS 1

/* constants */

// #define MAXWIDTH	320
// #define MAXWIDTH	320
#define MAXWIDTH 384
#define MAXHEIGHT 288
#define MAXCLUTS MAXHEIGHT
#define CLUTADDR 0x01026000
#define PAGESIZE 512
#define PAGEMASK (PAGESIZE - 1)
#define FIRSTLINE 20

#define MAXCOLORS 32
#define HASHLIMIT 65536

/* prototypes */

#ifdef SUN
long mystrtol ();
void evenspacing ();
void z24BitSort ();
void popsort ();
void popspace ();
void boxsort ();
void boxpopsort ();
void boxpoplengthsort ();
void inithash ();
void findhash ();
#else
long mystrtol (char *s);
void evenspacing (long *pop, long *index, long *maxcount);
void z24BitSort (long *pop, long *index, long *maxcount);
void popsort (long *pop, long *index, long *maxcount);
void popspace (long *pop, long *index, long *maxcount);
void boxsort (long *pop, long *index, long *maxcount);
void boxpopsort (long *pop, long *index, long *maxcount);
void boxpoplengthsort (long *pop, long *index, long *maxcount);
void inithash (void);
void initCLUT (void);
void findhash (uchar r, uchar g, uchar b);
#endif

#ifdef SUN
typedef int Boolean;
#define false 0
#define true 1
#endif

/* global variables */

int outwidth, outheight;
int width, height, rgbmax;
FILE *fin, *fout, *fout2;
unsigned char ycc_array[MAXWIDTH * 4][3], outarray[MAXWIDTH * 4];
int maxcolorflag;
long CLUTBase, palptr;
int maxcolors, mincolor;
int maxblues, minblue;
Boolean fixedFlag, halfBlueFlag, BGColorFlag, YCCFlag, z24BitFlag,
    sortBypassFlag, totalFlag;
Boolean literal16Flag, opffFlag;
long totalcolors;
#ifdef ALLOWCLUTS
long *pCLUTs;
int pCLUTindex;
#endif

void (*sortroutine) ();

unsigned char backred, backgreen, backblue;
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

/* code */

main (argc, argv) int argc;
char *argv[];
{
  int i, j;
  char instring[80];
  long position;
  long r, g, b;
  ImageCC imgChunk;
  // clutchunk clut;		// CLUTs not supported for now
  PixelChunk pxlChunk;

  outwidth = MAXWIDTH;
  outheight = MAXHEIGHT;
  width = 0;
  height = 0;
  maxcolorflag = 0;
  literal16Flag = 0;
  opffFlag = true;
  CLUTBase = CLUTADDR;
  maxcolors = MAXCOLORS;
  mincolor = 0;
  fixedFlag = true; // default to single fixed CLUT
  halfBlueFlag = false;
  BGColorFlag = false;
  YCCFlag = false;
  totalFlag = false;
  z24BitFlag = false;
  fin = NULL;
  fout = NULL;
  fout2 = NULL;
  sortroutine = evenspacing;
  sortBypassFlag = true;

  for (i = 1; i < argc; i++)
    {
      parse (argv[i]);
    }
  if (!fin)
    fin = stdin;
  if (!fout)
    fout = stdout;
  if (!fout)
    {
      usage ();
    }
  if (z24BitFlag)
    {
      fixedFlag = true;
      halfBlueFlag = false;
      mincolor = 0;
      maxcolors = 32;
      sortroutine = z24BitSort;
      sortBypassFlag = true;
      literal16Flag = false;
    }
  if ((!fout2) && (!fixedFlag) && (!opffFlag))
    {
      fprintf (stderr, "### Warning: Dynamic palette with no CLUT file\n");
    }

  if (opffFlag)
    initCLUT ();

  palptr = CLUTBase >> 2;

  if (BGColorFlag && YCCFlag)
    {
      r = backred;
      g = backgreen;
      b = backblue;
      backred = (299 * r + 587 * g + 114 * b + 500) / 1000;
      backgreen = ((r - backred + 179) * 255 + 179) / 358;
      backblue = ((b - backgreen + 226) * 255 + 226) / 452;
    }

  maxblues = maxcolors;
  minblue = mincolor;

  if (halfBlueFlag)
    {
      minblue = (mincolor + 1) & (~1);
      maxblues = (maxcolors + 1 - (mincolor & 1)) / 2;
    }

  if (totalFlag)
    {
      inithash ();
    }

  myfgets (instring, 80, fin);
  if ((instring[0] != 'P') || (instring[1] != '6'))
    {
      fprintf (stderr,
               "Error - file does not seem to be in ppm binary format\n");
      exit (1);
    }

  myfgets (instring, 80, fin);
  sscanf (instring, "%d", &width);

  myfgets (instring, 80, fin);
  sscanf (instring, "%d", &height);

  if (width < 1 || width > MAXWIDTH || height < 1 || height > MAXHEIGHT)
    {
      fprintf (stderr, "Error - invalid width or height (%d,%d)\n", width,
               height);
      exit (1);
    }
  else if (totalFlag)
    fprintf (stderr, "width, height = (%d, %d)\n", width, height);
  outwidth = width;
  outheight = height;

  myfgets (instring, 80, fin);
  sscanf (instring, "%d", &rgbmax);
  if (rgbmax != 255 && rgbmax != 127)
    { /* some ppm's compress color space for color reduction */
      fprintf (stderr, "Error - bad rgbmax value (%d)\n", rgbmax);
      exit (1);
    }
  else if (totalFlag)
    fprintf (stderr, "rgbmax = %d\n", rgbmax);

  position = ftell (fin);

  if (fixedFlag)
    {
      if (totalFlag)
        fprintf (stderr, "building color table\n");
      if (!sortBypassFlag)
        {
          initpops ();
          for (i = 0; i < height; i++)
            {
              doline (i, 1);
            }
        }
      sortcolors ();
      writeclutline (0);
      fseek (fin, position, SEEK_SET);
    }

  if (opffFlag)
    {
      imgChunk.chunk_ID = imagetype;
      imgChunk.chunk_size = sizeof (imgChunk);
      imgChunk.w = width;
      imgChunk.h = height;
      imgChunk.bytesperrow = width * (z24BitFlag ? 4 : 2);
      imgChunk.bitsperpixel = (z24BitFlag ? 32 : 16);
      imgChunk.numcomponents = 3;              /* R, G, and B */
      imgChunk.numplanes = 1;                  /* 1 means chunky */
      imgChunk.colorspace = (YCCFlag) ? 1 : 0; /* 0 = RGB */
      imgChunk.comptype = 0;                   /* no comp */
      imgChunk.hvformat = 0;                   /* 0 means 555 */
      imgChunk.pixelorder
          = (literal16Flag) ? 0 : 1; /* 1 means sherrie LRForm */
      imgChunk.pixelorder
          = (z24BitFlag) ? 3 : imgChunk.pixelorder; /* 3 means 24 bit mode */
      imgChunk.version = 0;

      if (fwrite (&imgChunk, 1, sizeof (imgChunk), fout) != sizeof (imgChunk))
        {
          fprintf (stderr, "Error writing image header to output file\n");
          exit (1);
        }

      pxlChunk.chunk_ID = 'PDAT';
      pxlChunk.chunk_size = width * height * (z24BitFlag ? 4 : 2) + 8;

      if (fwrite (&pxlChunk, 1, 8, fout) != 8)
        {
          fprintf (stderr, "Error writing pixel header to output file\n");
          exit (1);
        }
    }

  for (i = 0; i < outheight; i++)
    {
      if (z24BitFlag || !(i & 1))
        {
          for (j = 0; j < sizeof (outarray); j++)
            {
              outarray[j] = 0;
            }
        }
      if (i < height)
        doline (i, 0);
      if (i & 1 || i == outheight - 1 || z24BitFlag)
        {
          if (!literal16Flag)
            {
              if (fwrite (outarray, 4, outwidth, fout) != outwidth)
                {
                  fprintf (stderr, "Error writing to output file\n");
                  exit (1);
                }
            }
          else
            {
              for (j = 0; j < outwidth; j++)
                {
                  if ((putc (outarray[j * 4], fout) < 0)
                      || (putc (outarray[j * 4 + 1], fout) < 0))
                    {
                      fprintf (stderr, "Error writing to output file\n");
                      exit (1);
                    }
                }
              for (j = 0; j < outwidth; j++)
                {
                  if ((putc (outarray[j * 4 + 2], fout) < 0)
                      || (putc (outarray[j * 4 + 3], fout) < 0))
                    {
                      fprintf (stderr, "Error writing to output file\n");
                      exit (1);
                    }
                }
            }
        }
    }

#ifdef ALLOWCLUTS
  if (opffFlag & !sortBypassFlag)
    {
      int clutSize;

      /*  Chunk ID, size, and count are first 3 longs */
      clutSize = pCLUTindex * 4;

      pCLUTs[0] = 'VDL ';   /* CLUT chunk ID */
      pCLUTs[1] = clutSize; /* size of CLUT chunk */
      pCLUTs[2]
          = (fixedFlag)
                ? 1
                : height; /* one clut for each scanline unless fixed (-f) */

      if (fwrite (pCLUTs, 1, clutSize, fout) != clutSize)
        {
          fprintf (stderr, "Error writing to output file\n");
          exit (1);
        }
    }
#endif

  if (totalFlag)
    {
      if (totalcolors < HASHLIMIT)
        {
          fprintf (stderr, "total number of colors in output image: %ld\n",
                   totalcolors);
        }
      else
        {
          fprintf (stderr,
                   "total number of colors in image exceeds limit: %ld\n",
                   (long)HASHLIMIT);
        }
    }
  exit (0);
}

parse (option) char *option;
{
  FInfo fInfo;
  short err;
  long temprgb;

  if ((*option != '-') && (*option != '+'))
    {
      if (fout2)
        {
          usage ();
        }
      if (fout)
        {
          if (!(fout2 = fopen (option, "wb")))
            {
              fprintf (stderr,
                       "Error - could not open clut file %s for output\n",
                       option);
              exit (1);
            }
          return;
        }
      if (fin)
        {
          if (!(fout = fopen (option, "wb")))
            {
              fprintf (stderr,
                       "Error - could not open bitmap file %s for output\n",
                       option);
              exit (1);
            }
          err = GetFInfo (c2pstr (option), 0, &fInfo);
          if (!err)
            {
              fInfo.fdType = '3DOi';
              fInfo.fdCreator = '3DOe';
              err = SetFInfo (option, 0, &fInfo);
            }
          p2cstr (option);
          return;
        }
      else
        {
          if (!(fin = fopen (option, "rb")))
            {
              fprintf (stderr, "Error - could not open file %s for input\n",
                       option);
              exit (1);
            }
          return;
        }
    }
  else
    {
      switch (tolower (option[1]))
        {
        case 'd':
          fixedFlag = false;
          sortBypassFlag = false;
          sortroutine = boxpoplengthsort;
          // fprintf(stderr, "Warning - Unimplemented option: -d\n");
          return;
        case 'f':
          fixedFlag = true;
          sortBypassFlag = false;
          sortroutine = boxpoplengthsort;
          // fprintf(stderr, "Warning - Unimplemented option: -f\n");
          return;
        case 'b':
          halfBlueFlag = true;
          return;
        case 'v':
          totalFlag = true;
          return;
        case 'c':
          literal16Flag = true;
          fixedFlag = true;
          sortroutine = evenspacing;
          sortBypassFlag = true;
          return;
        case 'm':
          maxcolors = strtol (option + 2, NULL, 0);
          if (maxcolors + mincolor > 32 || maxcolors < 2)
            {
              fprintf (stderr,
                       "Illegal number of colors or illegal minimum color\n");
              exit (1);
            }
          return;
        case 'e':
          mincolor = strtol (option + 2, NULL, 0);
          if (maxcolors + mincolor > 32 || mincolor < 0)
            {
              fprintf (stderr,
                       "Illegal number of colors or illegal minimum color\n");
              exit (1);
            }
          return;
        case 't':
          temprgb = strtol (option + 2, NULL, 16);
          backred = (temprgb >> 16) & 0xff;
          backgreen = (temprgb >> 8) & 0xff;
          backblue = (temprgb)&0xff;
          BGColorFlag = true;
          return;
        case 'z':
          z24BitFlag = true;
          return;
        case 's':
          switch (strtol (option + 2, NULL, 0))
            {
            case 0:
              sortroutine = evenspacing;
              fixedFlag = true;
              sortBypassFlag = true;
              return;
            case 1:
              sortroutine = popsort;
              sortBypassFlag = false;
              return;
            case 2:
              sortroutine = popspace;
              sortBypassFlag = false;
              return;
            case 3:
              sortroutine = boxsort;
              sortBypassFlag = false;
              return;
            case 4:
              sortroutine = boxpopsort;
              sortBypassFlag = false;
              return;
            case 5:
              sortroutine = boxpoplengthsort;
              sortBypassFlag = false;
              return;
            default:
              fprintf (stderr, "### Unknown sorting option: %s\n", option);
              usage ();
            }
        default:
          fprintf (stderr, "### Unknown option: %s\n", option);
          usage ();
        }
    }
}

usage ()
{
  fprintf (stderr, "Usage:\nppmto3DO [options] ppmimagefile 3DOimagefile\n");
  fprintf (stderr, "options:\n");
  fprintf (stderr, "\t-f\tuse fixed color lookup table for whole image \n");
  fprintf (stderr, "\t-d\tuse dynamic color lookup table (reload CLUT on each "
                   "display line) \n");
  fprintf (
      stderr,
      "\t-b\tuse half the blue entries (assume least bit is h-position)\n");
  fprintf (stderr, "\t-v\tprint total of number of unique colors generated in "
                   "output image\n");
  fprintf (
      stderr,
      "\t-c\toutput a 16-bit, non-LR-form image for processing by makecel\n");
  fprintf (stderr, "\t-z\toutput a 24-bit image, stored as 32 bits/pixel for "
                   "static display with custom CLUT\n");
  fprintf (stderr,
           "\t-m<n>\tuse at most <n> entries in color table (default: 32)\n");
  fprintf (
      stderr,
      "\t-e<n>\tstart using color tables entries at index <n> (default: 0)\n");
  fprintf (
      stderr,
      "\t-t<n>\tspecify value (hex 555-RGB) of transparent color in input\n");
  fprintf (
      stderr,
      "\t-s0\tuse evenly distributed color space (also sets fixed CLUT)\n");
  fprintf (stderr, "\t-s1\tuse most popular values for CLUT selections\n");
  fprintf (stderr, "\t-s2\tdivide color space into (approximately) equal "
                   "population regions\n");
  fprintf (stderr, "\t-s3\tdivide color axes into tightest fitting boxes "
                   "(split longest spans)\n");
  fprintf (stderr,
           "\t-s4\tsame as s3 except split spans with largest populations\n");
  fprintf (stderr,
           "\t-s5\tsame as s3 except split largest population*span length\n");
  // fprintf (stderr, "\"-c\" option sets \"-s0\" option, so specify other
  // \"-s\" option(s) after \"-c\"\n"); fprintf (stderr, "to override.  The
  // \"-s0\" option assumes the \"-f\" (fixed color table) option, so\n");
  // fprintf (stderr, "specify \"-d\" after \"-s0\" to override.\n");
  exit (1);
}

initpops ()
{
  int i;

  ycount = 0;
  crcount = 0;
  cbcount = 0;

  for (i = 0; i < 256; i++)
    {
      popy[i] = 0;
      popcr[i] = 0;
      popcb[i] = 0;
      yindex[i] = i;
      crindex[i] = i;
      cbindex[i] = i;
    }
}

doline (line, pass) int line, pass;
{
  int i, m;
  long r, g, b;
  int y, cr, cb, yerror, crerror, cberror;
  int phase;
  /* int filler; */

  /* read in a line of pixel values */
  if (fread (ycc_array, 3, width, fin) != width)
    {
      fprintf (stderr, "Error - short input file\n");
      exit (1);
    }

  if (!fixedFlag)
    initpops ();

  /* convert rgb values to ycc and generate population counts */
  for (i = 0; i < width; i++)
    {
      if (YCCFlag)
        {
          r = ycc_array[i][0];
          g = ycc_array[i][1];
          b = ycc_array[i][2];
          y = (299 * r + 587 * g + 114 * b + 500) / 1000;
          cr = ((r - y + 179) * 255 + 179) / 358;
          cb = ((b - y + 226) * 255 + 226) / 452;
          ycc_array[i][0] = y;
          ycc_array[i][1] = cr;
          ycc_array[i][2] = cb;
        }
      else
        {
          y = ycc_array[i][0];
          cr = ycc_array[i][1];
          cb = ycc_array[i][2];
        }
      if ((!fixedFlag) || pass)
        {
          if ((popy[y]++) == 0)
            ycount++;
          if ((popcr[cr]++) == 0)
            crcount++;
          if ((popcb[cb]++) == 0)
            cbcount++;
        }
    }

  if (!fixedFlag)
    sortcolors ();

  if (!z24BitFlag)
    {
      phase = line & 1 ? 2 : 0;
      yerror = 0;
      crerror = 0;
      cberror = 0;
      for (i = 0; i < width; i++)
        {
#ifdef SHL_DEBUG
          if (line == 1)
            {
              if (popy[ycc_array[i][0]] >= maxcolors
                  || popcr[ycc_array[i][1]] >= maxcolors
                  || popcb[ycc_array[i][2]] >= maxcolors)
                {
                  fprintf (stderr, "Error: pixel = %d, %d, %d\n",
                           ycc_array[i][0], ycc_array[i][1], ycc_array[i][2]);
                }
            }
#endif

          y = popy[ycc_array[i][0]];
          cr = popcr[ycc_array[i][1]];
          cb = popcb[ycc_array[i][2]];
          /*
                y = popy[ycc_array[i][0]];
                k = ycc_array[i][1];
                cr = popcr[k];
                l = yindex[k];
                if (k-128) m = (((ycc_array[i][2]-128)*(l-128)) / (k-128))  +
             128; else m = ycc_array[i][2]; if (m<0) m=0; if (m>255) m=255; cb
             = popcb[m];
          */
          /*
                y = ycc_array[i][0]+yerror;
                k = y;
                if (y<0) y=0;
                if (y>255) y=255;
                y = popy[y];
                yerror = (k-yindex[y])>>1;
                cr = ycc_array[i][1]+crerror;
                k = cr;
                if (cr<0) cr=0;
                if (cr>255) cr=255;
                cr = popcr[cr];
                crerror = (k-crindex[cr])>>1;
                cb = ycc_array[i][2]+cberror;
                k = cb;
                if (cb<0) cb=0;
                if (cb>255) cb=255;
                cb = popcb[cb];
                cberror = (k-cbindex[cb])>>1;
          */
          m = ((y + mincolor) << 10) + ((cr + mincolor) << 5) + (cb + minblue);
          if (BGColorFlag && ycc_array[i][0] == backred
              && ycc_array[i][1] == backgreen && ycc_array[i][2] == backblue)
            {
              m = 0;
            }
          outarray[i * 4 + phase] = (m >> 8) & 0xff;
          outarray[i * 4 + phase + 1] = m & 0xff;
          if (totalFlag && (!pass))
            {
              findhash (yindex[y], crindex[cr], cbindex[cb]);
            }
        }
    }
  else
    {
      for (i = 0; i < width; i++)
        {
          y = getxc (ycc_array[i][0]);
          cr = getxc (ycc_array[i][1]);
          cb = getxc (ycc_array[i][2]);
          m = (y << 10) + (cr << 5) + (cb);
          if (BGColorFlag && ycc_array[i][0] == backred
              && ycc_array[i][1] == backgreen && ycc_array[i][2] == backblue)
            {
              m = 0;
            }
          outarray[i * 4] = (m >> 8) & 0xff;
          outarray[i * 4 + 1] = m & 0xff;
          y = getyc (ycc_array[i][0]);
          cr = getyc (ycc_array[i][1]);
          cb = getyc (ycc_array[i][2]);
          m = (y << 10) + (cr << 5) + (cb);
          if (BGColorFlag && ycc_array[i][0] == backred
              && ycc_array[i][1] == backgreen && ycc_array[i][2] == backblue)
            {
              m = 0;
            }
          outarray[i * 4 + 2] = (m >> 8) & 0xff;
          outarray[i * 4 + 3] = m & 0xff;
          if (totalFlag && (!pass))
            {
              findhash (ycc_array[i][0], ycc_array[i][1], ycc_array[i][2]);
            }
        }
    }

  if (!fixedFlag)
    writeclutline (line);
}

sortcolors ()
{
  int i, k, l, m;

  if (sortBypassFlag)
    {
      ycount = maxcolors;
      crcount = maxcolors;
      cbcount = maxblues;
    }
  else
    {
      if (ycount > maxcolors)
        {
          ycount = maxcolors;
          maxcolorflag |= 1;
        }
      if (crcount > maxcolors)
        {
          crcount = maxcolors;
          maxcolorflag |= 1;
        }
      if (cbcount > maxblues)
        {
          cbcount = maxblues;
          maxcolorflag |= 1;
        }
    }

  maxcount = cbcount;
  if (halfBlueFlag)
    maxcount = cbcount * 2 - 1 + (mincolor & 1);
  if (ycount > maxcount)
    maxcount = ycount;
  if (crcount > maxcount)
    maxcount = crcount;
  /* print a warning if more than 32 different Y, Cr or Cb values needed */
  if (maxcolorflag == 1)
    {
      fprintf (stderr, "%d colors exceeded on line(s) in image\n", maxcolors);
      maxcolorflag = 3;
    }
  if (maxcount > maxcolors)
    maxcount = maxcolors;
  if (maxcount < 3)
    maxcount = 3;

  (*sortroutine) (popy, yindex, &ycount);
  (*sortroutine) (popcr, crindex, &crcount);
  (*sortroutine) (popcb, cbindex, &cbcount);

  /* create reverse index in population arrays */
  for (i = 0; i < 256; i++)
    {
      k = abs (i - yindex[0]);
      popy[i] = 0;
      for (m = 1; m < ycount; m++)
        {
          if ((l = abs (i - yindex[m])) < k)
            {
              popy[i] = m;
              k = l;
            }
        }
      k = abs (i - crindex[0]);
      popcr[i] = 0;
      for (m = 1; m < crcount; m++)
        {
          if ((l = abs (i - crindex[m])) < k)
            {
              popcr[i] = m;
              k = l;
            }
        }
      k = abs (i - cbindex[0]);
      popcb[i] = 0;
      for (m = 1; m < cbcount; m++)
        {
          if ((l = abs (i - cbindex[m])) < k)
            {
              popcb[i] = m;
              if (halfBlueFlag)
                popcb[i] = m << 1;
              k = l;
            }
        }
    }
}

writeclutline (line) int line;
{
  int i, mflag;
  long j;
  int filler;

  if ((!fout2) && (!opffFlag))
    return; /* return if we're not going to write a clut file */

  mflag = 0; /* if using color 0 need to use color 32 as well */
  if (mincolor == 0)
    mflag = 1;
  filler = 0;
  if ((maxcount + mflag) & 1)
    filler = 1;
  palptr = palptr + maxcount + mflag + 2;
  if ((palptr & PAGEMASK) > PAGESIZE - (maxcolors + 3))
    {
      filler = PAGESIZE - (palptr & PAGEMASK);
    }
  j = ((maxcount + mflag) << 9) + 1;
  if ((line == height - 1) || fixedFlag)
    {
      j = ((maxcount + mflag) << 9) + 0;
      palptr = CLUTBase >> 2;
      filler = 0;
    }
  palptr += filler;
  //  clutword (palptr);

  //  clutword (j);
  //   Tests to create VDL like those produced by PhotoShop PlugIns
  filler = maxcolors - maxcount + 1;
  if (filler < 0)
    filler = 0;
  clutword (0x0025c401);
  clutword (0);
  clutword (0);
  clutword (0x90);
  clutword (0xc001602c);
  for (i = 0; i < maxcount; i++)
    {
      j = ((i + mincolor) << 24) + (yindex[i] << 16) + (crindex[i] << 8)
          + (cbindex[i]);
      if (halfBlueFlag)
        {
          j = ((i + mincolor) << 24) + (yindex[i] << 16) + (crindex[i] << 8)
              + (cbindex[i >> 1]);
        }

      clutword (j);
    }
  if (mflag)
    {
      j = (32 << 24) + (yindex[0] << 16) + (crindex[0] << 8) + (cbindex[0]);
      clutword (j);
    }

  for (; filler > 0; filler--)
    {
      clutword (0xe1000000L);
    }
  clutword (0xe1000000L);
}

void evenspacing (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i;

  /* *maxcount = maxcolors; */
  for (i = 0; i < *maxcount; i++)
    {
      index[i] = (255 * i + ((*maxcount - 1) / 2)) / (*maxcount - 1);
    }
}

void z24BitSort (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i;

  for (i = 0; i < 8; i++)
    {
      index[i] = i + i;
    }
  for (i = 8; i < 16; i++)
    {
      index[i] = i + i + 225;
    }
  for (i = 16; i < 32; i++)
    {
      index[i] = (31 - i) * 16;
    }
}

void popsort (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i, j, k;

  for (k = 0; k < *maxcount; k++)
    {
      for (i = 254; i >= 0; i--)
        {
          if (pop[index[i]] < pop[index[i + 1]])
            {
              j = index[i];
              index[i] = index[i + 1];
              index[i + 1] = j;
            }
        }
    }
}

void popspace (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i, j, k;
  long weight, accum;

  weight = width;
  if (fixedFlag)
    weight *= height;

  accum = 0;
  i = 0;
  k = 0;
  for (j = 0; j < 256; j++)
    {
      accum += pop[j];
      if ((accum * (*maxcount)) >= ((i + 1) * weight))
        {
          index[i++] = (j + k) / 2;
          k = j + 1;
        }
    }
}

void boxsort (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i, j;
  struct
  {
    int top;
    int bot;
    int weight
  } box[MAXCOLORS];
  int next;

  next = 1;
  box[0].top = 0;
  box[0].bot = 255;
  while (!pop[box[0].top])
    box[0].top++;
  while (!pop[box[0].bot])
    box[0].bot--;
  box[0].weight = box[0].bot - box[0].top;
  while (next < *maxcount)
    {
      j = 0;
      /* find a box to split */
      for (i = 1; i < next; i++)
        {
          if (box[i].weight > box[j].weight)
            j = i;
        }
      if (!box[j].weight)
        break;
      if (box[j].top == box[j].bot)
        {
          box[j].weight = 0;
          continue;
        }
      i = (box[j].top + box[j].bot + 1) / 2;
      box[next].top = i;
      box[next].bot = box[j].bot;
      box[j].bot = i - 1;
      while (!pop[box[next].top])
        box[next].top++;
      while (!pop[box[j].bot])
        box[j].bot--;
      box[next].weight = box[next].bot - box[next].top;
      box[j].weight = box[j].bot - box[j].top;
      next++;
    }
  for (i = 0; i < next; i++)
    {
      index[i] = (box[i].top + box[i].bot) >> 1;
    }
}

void boxpopsort (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i, j;
  struct
  {
    int top;
    int bot;
    long weight
  } box[MAXCOLORS];
  int next, popup, popdown;

  next = 1;
  box[0].top = 0;
  box[0].bot = 255;
  box[0].weight = width;
  if (fixedFlag)
    box[0].weight *= height;
  while (!pop[box[0].top])
    box[0].top++;
  while (!pop[box[0].bot])
    box[0].bot--;
  while (next < *maxcount)
    {
      j = 0;
      /* find a box to split */
      for (i = 1; i < next; i++)
        {
          /* if (box[i].weight > box[j].weight) j=i; */
          if (box[i].weight > box[j].weight)
            {
              j = i;
            }
        }
      if (!box[j].weight)
        break;
      if (box[j].top == box[j].bot)
        {
          box[j].weight = 0;
          continue;
        }
      popup = 0;
      popdown = box[j].weight;
      for (i = box[j].top; i < box[j].bot; i++)
        {
          if (popup + pop[i] >= popdown - pop[i])
            break;
          popup += pop[i];
          popdown -= pop[i];
        }
      if (popup + pop[i] <= popdown)
        {
          popup += pop[i];
          popdown -= pop[i];
          i++;
        }
      box[next].top = i;
      box[next].bot = box[j].bot;
      box[next].weight = popdown;
      box[j].bot = i - 1;
      box[j].weight = popup;
      while (!pop[box[next].top])
        box[next].top++;
      while (!pop[box[j].bot])
        box[j].bot--;
      next++;
    }
  for (i = 0; i < next; i++)
    {
      index[i] = (box[i].top + box[i].bot) >> 1;
    }
}

void boxpoplengthsort (pop, index, maxcount) long *pop, *index, *maxcount;
{
  int i, j;
  struct
  {
    int top;
    int bot;
    long weight
  } box[MAXCOLORS];
  int next, popup, popdown;

  next = 1;
  box[0].top = 0;
  box[0].bot = 255;
  box[0].weight = width;
  if (fixedFlag)
    box[0].weight *= height;
  while (!pop[box[0].top])
    box[0].top++;
  while (!pop[box[0].bot])
    box[0].bot--;
  while (next < *maxcount)
    {
      j = 0;
      /* find a box to split */
      for (i = 1; i < next; i++)
        {
          /* if (box[i].weight > box[j].weight) j=i; */
          if (box[i].weight * (box[i].bot - box[i].top)
              > box[j].weight * (box[j].bot - box[j].top))
            {
              j = i;
            }
        }
      if (!box[j].weight)
        break;
      if (box[j].top == box[j].bot)
        {
          box[j].weight = 0;
          continue;
        }
      popup = 0;
      popdown = box[j].weight;
      for (i = box[j].top; i < box[j].bot; i++)
        {
          if (popup + pop[i] >= popdown - pop[i])
            break;
          popup += pop[i];
          popdown -= pop[i];
        }
      if (popup + pop[i] <= popdown)
        {
          popup += pop[i];
          popdown -= pop[i];
          i++;
        }
      box[next].top = i;
      box[next].bot = box[j].bot;
      box[next].weight = popdown;
      box[j].bot = i - 1;
      box[j].weight = popup;
      while (!pop[box[next].top])
        box[next].top++;
      while (!pop[box[j].bot])
        box[j].bot--;
      /*
          printf ("%d->%d: %d %d %d - %d %d %d\n",
          j, next, box[j].top, box[j].bot, box[j].weight,
          box[next].top, box[next].bot, box[next].weight);
      */
      next++;
    }
  for (i = 0; i < next; i++)
    {
      /*
          j = box[i].top;
          for (k=j+1; k<=box[i].bot; k++) {
            if (pop[k] > pop[j]) j=k;
          }
          index[i] = j;
      */
      index[i] = (box[i].top + box[i].bot) >> 1;
      /* printf ("%d : %d %d %d %d\n", i, box[i].top, box[i].bot,
       * box[i].weight, j); */
    }
}

clutword (value) long value;
{
  if (opffFlag)
    {
#ifdef ALLOWCLUTS
      pCLUTs[pCLUTindex++] = value;
#endif
    }
  else
    {
      putc ((char)(value >> 24) & 0xff, fout2);
      putc ((char)(value >> 16) & 0xff, fout2);
      putc ((char)(value >> 8) & 0xff, fout2);
      putc ((char)(value)&0xff, fout2);
    }
}

int
myfgets (string, count, stream)
char *string;
int count;
FILE *stream;
{
  int i, j;

  for (i = 0; i < count - 1;)
    {
      j = getc (stream);
      if (j < 0)
        break;
      string[i++] = j;
      if (j == ASCII_CR || j == ASCII_LF || j == ASCII_SPACE)
        break;
    }

  string[i] = '\0';
  return i;
}

long
mystrtol (s)
char *s;
{
  if (*s == '$')
    return strtol (s + 1, NULL, 16);
  else
    return strtol (s, NULL, 0);
}

int
getxc (c)
int c;
{
  if (c < 128)
    return (31 - (c / 8));
  else
    return (46 - (c / 8));
}

int
getyc (c)
int c;
{
  if (c < 128)
    return (c % 8);
  else
    return ((c % 8) + 8);
}

void
inithash ()
{
  long i;

  ch = (colorhash *)malloc (HASHLIMIT * sizeof (colorhash));

  for (i = 0; i < HASHLIMIT; i++)
    {
      ch[i].hit = 0;
    }

  totalcolors = 0;
}

void
initCLUT ()
{
#ifdef ALLOWCLUTS
  pCLUTindex = 3; /* skip the id and size for now */
  pCLUTs = malloc (MAXCLUTS * (sizeof (VDL_REC) + 32)
                   + 12); /* + 12 for chunk ID and size */
#endif
}

void findhash (r, g, b) uchar r, g, b;
{
  ulong i, j;
  long k;
  if (totalcolors >= HASHLIMIT)
    return;

  i = (((ulong)r * (ulong)33329 + (ulong)g) * (ulong)33349 + (ulong)b)
      % (ulong)HASHLIMIT;
  j = 1;
  k = 1;
  while (ch[i].hit && ((ch[i].r != r) || (ch[i].g != g) || (ch[i].b != b)))
    {
      i = (i + HASHLIMIT + j * k) % HASHLIMIT;
      j++;
      k = -k;
    }
  if (!ch[i].hit)
    {
      ch[i].hit = 1;
      ch[i].r = r;
      ch[i].g = g;
      ch[i].b = b;
      totalcolors++;
    }
}

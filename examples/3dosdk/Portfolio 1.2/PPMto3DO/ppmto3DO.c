
/***********************************************************\
*                                                         	*
* File:     ppmto3DO.c                                    	*
* By:       Stephen H. Landrum                            	*
* Created:  16-Jul-91                                    	*
*
*
* Formerly "ppmopera.c"
* *
*
* 9/15/92	jb	Split scan of gInheight and gInwidth
* *			to allow for CR/LF differences
* *			on different platforms.
*
*                                                         	*
* 9/28/92	njc	Write CLUTs out in 3DO format * *
* *11/15/92	crm	changed name and command-line * *
options for release to the	  					*
*			outside world
*
*                                                         	*
* Program to convert images from ppm generic image       	*
* to opera memory bitmap and CLUT list format             	*
*
* *
* *	1/20/94 3:04:59 PM Add ppmto3DO.h
* *					Add CType.h for tolower function
*
*                     Make code readable                  	*
*                                                         	*
*                                                         	*
*                                                         	*
* This document is proprietary and confidential           	*
*                                                        	*
* Copyright (c) 1991,1992 The 3DO Company *
*                                                         	*
\***********************************************************/

// enables some diagnostic output
//#define SHL_DEBUG

#include "ppmto3DO.h"

main (int argc, char *argv[])
{
  int i;
  // clutchunk clut;				// CLUTs not supported for now

  gOutwidth = MAXWIDTH;
  gOutheight = MAXHEIGHT;
  gInwidth = 0;
  gInheight = 0;
  gMaxcolorflag = 0;
  gLiteral16Flag = 0; // it is not 16-bit non LR form image
  gOpffFlag = true;
  gCLUTBase = CLUTADDR;
  gMaxcolors = MAXCOLORS;
  gMincolor = 0;
  gFixedFlag = true;         // default to single fixed CLUT
  g15541DisplayMode = false; // default: display mode is 1555
  gBGColorFlag = false;
  gYCCFlag = false;
  gPrintColorsFlag = false;
  gZ24BitFlag = false; // it is always false
  gFin = NULL;
  gFout = NULL;
  gFout2 = NULL;
  sortroutine = evenspacing;
  gSortBypassFlag = true;

  for (i = 1; i < argc; i++)
    parse (argv[i]); // Read in the arguments
  if (!gFout)
    usage (); // No output file, quit
  SetZ24Bit ();

  if ((!gFout2) && (!gFixedFlag) && (!gOpffFlag))
    fprintf (stderr, "### Warning: Dynamic palette with no CLUT file\n");

  if (gOpffFlag) // Always true
    initCLUT ();

  gPalptr = gCLUTBase >> 2;

  SetBackgroundColor ();
  SetTheBlues ();
  if (gPrintColorsFlag)
    inithash ();
  ParsePPM ();
  ProcessColors ();
  WriteOutImage ();
  WriteOutTotalColors ();
  exit (0);
}

void
SetZ24Bit (void)
{
  if (gZ24BitFlag) // always false
    {
      gFixedFlag = true;
      g15541DisplayMode = false;
      gMincolor = 0;
      gMaxcolors = 32;
      sortroutine = z24BitSort;
      gSortBypassFlag = true;
      gLiteral16Flag = false;
    }
}

void
SetBackgroundColor (void)
{
  long r, g, b;

  if (gBGColorFlag && gYCCFlag)
    {
      r = gBackred; // Passed into the tool
      g = gBackgreen;
      b = gBackblue;
      gBackred = (299 * r + 587 * g + 114 * b + 500) / 1000;
      gBackgreen = ((r - gBackred + 179) * 255 + 179) / 358;
      gBackblue = ((b - gBackgreen + 226) * 255 + 226) / 452;
    }
}

void
WriteOutTotalColors (void)
{
  if (gPrintColorsFlag)
    {
      if (gTotalcolors < HASHLIMIT)
        fprintf (stderr, "total number of colors in output image: %ld\n",
                 gTotalcolors);
      else
        fprintf (stderr,
                 "total number of colors in image exceeds limit: %ld\n",
                 (long)HASHLIMIT);
    }
}

void
SetTheBlues (void)
{
  gMaxblues = gMaxcolors;
  gMinblue = gMincolor;

  if (g15541DisplayMode)
    {
      gMinblue = (gMincolor + 1) & (~1);
      gMaxblues = (gMaxcolors + 1 - (gMincolor & 1)) / 2;
    }
}

void
ProcessColors (void)
{
  if (gFixedFlag) // Default is true
    {
      long position;
      int i;

      position = ftell (gFin);
      if (gPrintColorsFlag)
        fprintf (stderr, "building color table\n");
      if (!gSortBypassFlag)
        {
          initpops ();
          for (i = 0; i < gInheight; i++)
            readline (i, 1);
        }
      sortcolors ();
      writeclutline (0); // does nothing for now
      fseek (gFin, position, SEEK_SET);
    }
}

void
ParsePPM (void)
{
  char instring[80];

  // PPM Parse Code
  // Read first 80 characters of ppm
  myfgets (instring, 80, gFin);

  // P6 denotes a ppm binary format
  if ((instring[0] != 'P') || (instring[1] != '6'))
    {
      fprintf (stderr,
               "Error - file does not seem to be in ppm binary format\n");
      exit (1);
    }

  // Get PPM width and height
  myfgets (instring, 80, gFin);
  sscanf (instring, "%d", &gInwidth);
  myfgets (instring, 80, gFin);
  sscanf (instring, "%d", &gInheight);
  if (gInwidth < 1 || gInwidth > MAXWIDTH || gInheight < 1
      || gInheight > MAXHEIGHT)
    {
      fprintf (stderr, "Error - invalid gInwidth or gInheight (%d,%d)\n",
               gInwidth, gInheight);
      exit (1);
    }
  else if (gPrintColorsFlag)
    fprintf (stderr, "gInwidth, gInheight = (%d, %d)\n", gInwidth, gInheight);

  // Default out width and height equals ppm's width and height
  gOutwidth = gInwidth;
  gOutheight = gInheight;

  // Get max rgb color
  myfgets (instring, 80, gFin);
  sscanf (instring, "%d", &gRgbmax);
  if (gRgbmax != 255 && gRgbmax != 127)
    { // some ppm's compress color space for color reduction
      fprintf (stderr, "Error - bad gRgbmax value (%d)\n", gRgbmax);
      exit (1);
    }
  else if (gPrintColorsFlag)
    fprintf (stderr, "gRgbmax = %d\n", gRgbmax);
}

void
WriteOutImage (void)
{
  if (gOpffFlag)
    {
      WriteOutImageChunk ();
      WriteOutPDATChunk ();
    }
  else
    fprintf (stderr, "Someone turned off the gOpffFlag");

  WriteOutImageData ();
}

void
WriteOutImageChunk (void)
{
  ImageCC imgChunk;

  imgChunk.chunk_ID = imagetype; // Defined in form3do.h
  imgChunk.chunk_size = sizeof (imgChunk);
  imgChunk.w = gInwidth;
  imgChunk.h = gInheight;
  imgChunk.bytesperrow = gInwidth * 2;
  imgChunk.bitsperpixel = 16;
  imgChunk.numcomponents = 3;                     // R, G, and B
  imgChunk.numplanes = 1;                         // 1 means chunky
  imgChunk.colorspace = (gYCCFlag) ? 1 : 0;       // 0 = RGB
  imgChunk.comptype = 0;                          // no comp
  imgChunk.hvformat = 0;                          // 0 means 555
  imgChunk.pixelorder = (gLiteral16Flag) ? 0 : 1; // 1 means sherrie LRForm
  imgChunk.version = 0;

  // Write out the image chunk
  if (fwrite (&imgChunk, 1, sizeof (imgChunk), gFout) != sizeof (imgChunk))
    {
      fprintf (stderr, "Error writing image header to output file\n");
      exit (1);
    }
}

void
WriteOutPDATChunk (void)
{
  PixelChunk pxlChunk;

  pxlChunk.chunk_ID = 'PDAT';
  pxlChunk.chunk_size = gInwidth * gInheight * 2 + 8;

  if (fwrite (&pxlChunk, 1, 8, gFout) != 8)
    {
      fprintf (stderr, "Error writing pixel header to output file\n");
      exit (1);
    }
}

void
WriteOutImageData (void)
{
  int i, j;

  for (i = 0; i < gOutheight; i++)
    {
      if (gZ24BitFlag || !(i & 1))
        {
          for (j = 0; j < sizeof (gOutarray); j++)
            gOutarray[j] = 0;
        }

      if (i < gInheight)
        readline (i, 0);

      if (i & 1 || i == gOutheight - 1 || gZ24BitFlag)
        {
          if (!gLiteral16Flag)
            {
              if (fwrite (gOutarray, 4, gOutwidth, gFout) != gOutwidth)
                {
                  fprintf (stderr, "Error writing to output file\n");
                  exit (1);
                }
            }
          else
            {
              for (j = 0; j < gOutwidth; j++)
                {
                  if ((putc (gOutarray[j * 4], gFout) < 0)
                      || (putc (gOutarray[j * 4 + 1], gFout) < 0))
                    {
                      fprintf (stderr, "Error writing to output file\n");
                      exit (1);
                    }
                }
              for (j = 0; j < gOutwidth; j++)
                {
                  if ((putc (gOutarray[j * 4 + 2], gFout) < 0)
                      || (putc (gOutarray[j * 4 + 3], gFout) < 0))
                    {
                      fprintf (stderr, "Error writing to output file\n");
                      exit (1);
                    } // end if
                }     // end for
            }         // end else
        }             // end if
      else
        ; // do nothing

    } // end for
#ifdef ALLOWCLUTS
  if (gOpffFlag)
    {
      int clutSize;

      clutSize
          = pCLUTindex * 4; //  Chunk ID, size, and count are first 3 longs

      pCLUTs[0] = 'CLUT';   // CLUT chunk ID
      pCLUTs[1] = clutSize; // size of CLUT chunk
      pCLUTs[2]
          = (gFixedFlag)
                ? 1
                : gInheight; // one clut for each scanline unless fixed (-f)

      if (fwrite (pCLUTs, 1, clutSize, gFout) != clutSize)
        {
          fprintf (stderr, "Error writing to output file\n");
          exit (1);
        }
    }
#endif
}

void
parse (char *option)
{
  FInfo fInfo;
  short err;
  long temprgb;

  if ((*option != '-') && (*option != '+')) // It is a file
    {
      if (gFout2)
        usage ();

      if (gFout) // Create a second file output if we have the first one
        {
          if (!(gFout2 = fopen (option, "wb")))
            {
              fprintf (stderr,
                       "Error - could not open clut file %s for output\n",
                       option);
              exit (1);
            }
          return;
        }

      if (gFin) // Create the output file only if we have the input file
        {
          if (!(gFout = fopen (option, "wb")))
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
          else
            fprintf (stderr, "Error: GetFInfo returns %d", err);
          p2cstr (option);
          return;
        }
      else
        {
          if (!(gFin = fopen (option, "rb")))
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
          // gFixedFlag = false;
          // gSortBypassFlag = false;
          // sortroutine = boxpoplengthsort;
          fprintf (stderr, "Warning - Unimplemented option: -d\n");
          return;
        case 'f':
          // gFixedFlag = true;
          // gSortBypassFlag = false;
          // sortroutine = boxpoplengthsort;
          fprintf (stderr, "Warning - Unimplemented option: -f\n");
          return;
        case 'b':
          g15541DisplayMode = true;
          return;
        case 'v':
          gPrintColorsFlag = true;
          return;
        case 'c':
          gLiteral16Flag = true;
          gFixedFlag = true;
          sortroutine = evenspacing;
          gSortBypassFlag = true;
          return;
        case 'm':
          gMaxcolors = strtol (option + 2, NULL, 0);
          if (gMaxcolors + gMincolor > 32 || gMaxcolors < 2)
            {
              fprintf (stderr,
                       "Illegal number of colors or illegal minimum color\n");
              exit (1);
            }
          return;
        case 'e':
          gMincolor = strtol (option + 2, NULL, 0);
          if (gMaxcolors + gMincolor > 32 || gMincolor < 0)
            {
              fprintf (stderr,
                       "Illegal number of colors or illegal minimum color\n");
              exit (1);
            }
          return;
        case 't':
          temprgb = strtol (option + 2, NULL, 16);
          gBackred = (temprgb >> 16) & 0xff;
          gBackgreen = (temprgb >> 8) & 0xff;
          gBackblue = (temprgb)&0xff;
          gBGColorFlag = true;
          return;
        case 's':
          switch (strtol (option + 2, NULL, 0))
            {
            case 0:
              sortroutine = evenspacing;
              gFixedFlag = true;
              gSortBypassFlag = true;
              return;
            case 1:
              sortroutine = popsort;
              gSortBypassFlag = false;
              return;
            case 2:
              sortroutine = popspace;
              gSortBypassFlag = false;
              return;
            case 3:
              sortroutine = boxsort;
              gSortBypassFlag = false;
              return;
            case 4:
              sortroutine = boxpopsort;
              gSortBypassFlag = false;
              return;
            case 5:
              sortroutine = boxpoplengthsort;
              gSortBypassFlag = false;
              return;
            default:
              fprintf (stderr, "### Unknown sorting option: %s\n", option);
              usage ();
            }
        default:
          fprintf (stderr, "### Unknown option: %s\n", option);
          usage ();
        } // end switch
    }     // end else
} // end parse

void
usage (void)
{
  fprintf (stderr, "Usage:\nppmto3DO [options] ppmimagefile 3DOimagefile\n");
  fprintf (stderr, "options:\n");
  fprintf (stderr, "\t-f\tuse fixed color lookup table for whole image [NOT "
                   "IMPLEMENTED]\n");
  fprintf (stderr, "\t-d\tuse dynamic color lookup table (reload CLUT on each "
                   "display line) [NOT IMPLEMENTED]\n");
  fprintf (
      stderr,
      "\t-b\tuse half the blue entries (assume least bit is h-position)\n");
  fprintf (stderr, "\t-v\tprint total of number of unique colors generated in "
                   "output image\n");
  fprintf (
      stderr,
      "\t-c\toutput a 16-bit, non-LR-form image for processing by makecel\n");
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

void
initpops (void)
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

void
readline (int line, int pass)
{
  int i, m;
  long r, g, b;
  int y, cr, cb, yerror, crerror, cberror;
  int phase;
  /* int filler; */

  // read in a line of pixel values
  if (fread (ycc_array, 3, gInwidth, gFin) != gInwidth)
    {
      fprintf (stderr, "Error - short input file\n");
      exit (1);
    }

  if (!gFixedFlag)
    initpops (); // gFixedFlag is true by default

  /* convert rgb values to ycc and generate population counts */
  for (i = 0; i < gInwidth; i++)
    {
      if (gYCCFlag)
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
      if ((!gFixedFlag) || pass)
        {
          if ((popy[y]++) == 0)
            ycount++;
          if ((popcr[cr]++) == 0)
            crcount++;
          if ((popcb[cb]++) == 0)
            cbcount++;
        }
    }

  if (!gFixedFlag)
    sortcolors ();

  if (!gZ24BitFlag)
    {
      phase = line & 1 ? 2 : 0;
      yerror = 0;
      crerror = 0;
      cberror = 0;
      for (i = 0; i < gInwidth; i++)
        {
#ifdef SHL_DEBUG
          if (line == 1)
            {
              if (popy[ycc_array[i][0]] >= gMaxcolors
                  || popcr[ycc_array[i][1]] >= gMaxcolors
                  || popcb[ycc_array[i][2]] >= gMaxcolors)
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
          if (k-128) m = (((ycc_array[i][2]-128)*(l-128)) / (k-128))  +  128;
          else m = ycc_array[i][2];
          if (m<0) m=0;
          if (m>255) m=255;
          cb = popcb[m];
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
          m = ((y + gMincolor) << 10) + ((cr + gMincolor) << 5)
              + (cb + gMinblue);
          if (gBGColorFlag && ycc_array[i][0] == gBackred
              && ycc_array[i][1] == gBackgreen && ycc_array[i][2] == gBackblue)
            m = 0;

          gOutarray[i * 4 + phase] = (m >> 8) & 0xff;
          gOutarray[i * 4 + phase + 1] = m & 0xff;
          if (gPrintColorsFlag && (!pass))
            findhash (yindex[y], crindex[cr], cbindex[cb]);
        } // end for
    }     // end if !z24BitFlag
  else    // 24 bit
    {
      for (i = 0; i < gInwidth; i++)
        {
          y = getxc (ycc_array[i][0]);
          cr = getxc (ycc_array[i][1]);
          cb = getxc (ycc_array[i][2]);
          m = (y << 10) + (cr << 5) + (cb);
          if (gBGColorFlag && ycc_array[i][0] == gBackred
              && ycc_array[i][1] == gBackgreen && ycc_array[i][2] == gBackblue)
            m = 0;
          gOutarray[i * 4] = (m >> 8) & 0xff;
          gOutarray[i * 4 + 1] = m & 0xff;
          y = getyc (ycc_array[i][0]);
          cr = getyc (ycc_array[i][1]);
          cb = getyc (ycc_array[i][2]);
          m = (y << 10) + (cr << 5) + (cb);
          if (gBGColorFlag && ycc_array[i][0] == gBackred
              && ycc_array[i][1] == gBackgreen && ycc_array[i][2] == gBackblue)
            m = 0;
          gOutarray[i * 4 + 2] = (m >> 8) & 0xff;
          gOutarray[i * 4 + 3] = m & 0xff;
          if (gPrintColorsFlag && (!pass))
            findhash (ycc_array[i][0], ycc_array[i][1], ycc_array[i][2]);
        } // end for
    }     // end else

  if (!gFixedFlag)
    writeclutline (line);
}

void
sortcolors (void)
{
  int i, k, l, m;

  if (gSortBypassFlag) // true by default
    {
      ycount = gMaxcolors;
      crcount = gMaxcolors;
      cbcount = gMaxblues;
    }
  else
    {
      if (ycount > gMaxcolors)
        {
          ycount = gMaxcolors;
          gMaxcolorflag |= 1;
        }
      if (crcount > gMaxcolors)
        {
          crcount = gMaxcolors;
          gMaxcolorflag |= 1;
        }
      if (cbcount > gMaxblues)
        {
          cbcount = gMaxblues;
          gMaxcolorflag |= 1;
        }
    }

  maxcount = cbcount;
  if (g15541DisplayMode)
    maxcount = cbcount * 2 - 1 + (gMincolor & 1);
  if (ycount > maxcount)
    maxcount = ycount;
  if (crcount > maxcount)
    maxcount = crcount;

  // print a warning if more than 32 different Y, Cr or Cb values needed
  if (gMaxcolorflag == 1)
    {
      fprintf (stderr, "%d colors exceeded on line(s) in image\n", gMaxcolors);
      gMaxcolorflag = 3;
    }
  if (maxcount > gMaxcolors)
    maxcount = gMaxcolors;
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
              if (g15541DisplayMode)
                popcb[i] = m << 1;
              k = l;
            }
        } // end for
    }     // end for
} // end sortcolors

void
writeclutline (int line)
{
  int i, mflag;
  long j;
  int filler;

  if ((!gFout2) && (!gOpffFlag))
    return; // return if we're not going to write a clut file

  mflag = 0; // if using color 0 need to use color 32 as well
  if (gMincolor == 0)
    mflag = 1;
  filler = 0;
  if ((maxcount + mflag) & 1)
    filler = 1;
  gPalptr = gPalptr + maxcount + mflag + 2;
  if ((gPalptr & PAGEMASK) > PAGESIZE - (gMaxcolors + 3))
    filler = PAGESIZE - (gPalptr & PAGEMASK);
  j = ((maxcount + mflag) << 9) + 1;
  if ((line == gInheight - 1) || gFixedFlag)
    {
      j = ((maxcount + mflag) << 9) + 0;
      gPalptr = gCLUTBase >> 2;
      filler = 0;
    }
  gPalptr += filler;
  clutword (gPalptr);

  clutword (j);
  for (i = 0; i < maxcount; i++)
    {
      j = ((i + gMincolor) << 24) + (yindex[i] << 16) + (crindex[i] << 8)
          + (cbindex[i]);
      if (g15541DisplayMode)
        j = ((i + gMincolor) << 24) + (yindex[i] << 16) + (crindex[i] << 8)
            + (cbindex[i >> 1]);

      clutword (j);
    }
  if (mflag)
    {
      j = (32 << 24) + (yindex[0] << 16) + (crindex[0] << 8) + (cbindex[0]);
      clutword (j);
    }

  for (; filler > 0; filler--)
    clutword (0L);
}

void
evenspacing (long *pop, long *index, long *maxcount)
{
  int i;

  // *maxcount = gMaxcolors;
  for (i = 0; i < *maxcount; i++)
    index[i] = (255 * i + ((*maxcount - 1) / 2)) / (*maxcount - 1);
}

void
z24BitSort (long *pop, long *index, long *maxcount)
{
  int i;

  for (i = 0; i < 8; i++)
    index[i] = i + i;
  for (i = 8; i < 16; i++)
    index[i] = i + i + 225;
  for (i = 16; i < 32; i++)
    index[i] = (31 - i) * 16;
}

void
popsort (long *pop, long *index, long *maxcount)
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

void
popspace (long *pop, long *index, long *maxcount)
{
  int i, j, k;
  long weight, accum;

  weight = gInwidth;
  if (gFixedFlag)
    weight *= gInheight;

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

void
boxsort (long *pop, long *index, long *maxcount)
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
    index[i] = (box[i].top + box[i].bot) >> 1;
}

void
boxpopsort (long *pop, long *index, long *maxcount)

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
  box[0].weight = gInwidth;
  if (gFixedFlag)
    box[0].weight *= gInheight;
  while (!pop[box[0].top])
    box[0].top++;
  while (!pop[box[0].bot])
    box[0].bot--;
  while (next < *maxcount)
    {
      j = 0;
      // find a box to split
      for (i = 1; i < next; i++)
        {
          // if (box[i].weight > box[j].weight) j=i;
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
    index[i] = (box[i].top + box[i].bot) >> 1;
}

void
boxpoplengthsort (long *pop, long *index, long *maxcount)
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
  box[0].weight = gInwidth;
  if (gFixedFlag)
    box[0].weight *= gInheight;
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
            j = i;
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
      for (k=j+1; k<=box[i].bot; k++)
              if (pop[k] > pop[j]) j=k;
      index[i] = j;
      */
      index[i] = (box[i].top + box[i].bot) >> 1;
      // printf ("%d : %d %d %d %d\n", i, box[i].top, box[i].bot,
      // box[i].weight, j);
    }
}

void
clutword (long value)
{
  if (gOpffFlag)
    {
#ifdef ALLOWCLUTS
      pCLUTs[pCLUTindex++] = value;
#endif
    }
  else
    {
      putc ((char)(value >> 24) & 0xff, gFout2);
      putc ((char)(value >> 16) & 0xff, gFout2);
      putc ((char)(value >> 8) & 0xff, gFout2);
      putc ((char)(value)&0xff, gFout2);
    }
}

int
myfgets (char *string, int count, FILE *stream)
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
mystrtol (char *s)
{
  if (*s == '$')
    return strtol (s + 1, NULL, 16);
  else
    return strtol (s, NULL, 0);
}

int
getxc (int c)
{
  if (c < 128)
    return (31 - (c / 8));
  else
    return (46 - (c / 8));
}

int
getyc (int c)
{
  if (c < 128)
    return (c % 8);
  else
    return ((c % 8) + 8);
}

void
inithash (void)
{
  long i;

  ch = (colorhash *)malloc (HASHLIMIT * sizeof (colorhash));

  for (i = 0; i < HASHLIMIT; i++)
    ch[i].hit = 0;

  gTotalcolors = 0;
}

void
initCLUT (void)
{
#ifdef ALLOWCLUTS
  pCLUTindex = 3; // skip the id and size for now
  pCLUTs = malloc (MAXCLUTS * (sizeof (A_VDL) + 32)
                   + 12); // + 12 for chunk ID and size
#endif
}

void
findhash (uchar r, uchar g, uchar b)
{
  ulong i, j;
  long k;
  if (gTotalcolors >= HASHLIMIT)
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
      gTotalcolors++;
    }
}

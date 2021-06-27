/***************************************************************\
*                                                               *
* Cel packing tool for the 3DO hardware							*
*                                                               *
* By:  Stephen H. Landrum                                       *
*                                                               *
* Formerly "packbits.c"											*
*																*
* Last update:  9/9/92	NJC	supports 3DO file format.			*
*			   11/15/92	CRM changed name and command-line		*
*						    options for release to the outside	*
*						    world.								*
*                                                               *
* Copyright (c) 1992 The 3DO Company				            *
*                                                               *
\***************************************************************/


#ifdef SUN
#include <sys/types.h>
#include <stdio.h>
#else
#include <types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <Strings.h>
#include <Files.h>
#endif

#include <Form3DOmc.h>

#define TRUE	1
#define FALSE	0


#define COUNTMARKER	0x80000000
#define LITMARKER	0x00000040
#define TRANSMARKER	0x00000080
#define PACKMARKER	0x000000c0

#define MAXWIDTH	320
#define MAXHEIGHT	240


/* SCB control word bits */
#define SCB_SKIP	0x80000000
#define SCB_LAST	0x40000000
#define SCB_NPABS	0x20000000
#define SCB_SPABS	0x10000000
#define SCB_PPABS	0x08000000
#define SCB_LDSIZE	0x04000000
#define SCB_LDPRS	0x02000000
#define SCB_LDPPMP	0x01000000
#define SCB_LDPIP	0x00800000
#define SCB_SCBPRE	0x00400000
#define SCB_YOXY	0x00200000

#define SCB_ACW		0x00040000
#define SCB_ACCW	0x00020000
#define SCB_TWD		0x00010000
#define SCB_LCE		0x00008000
#define SCB_ACE		0x00004000
#define SCB_ASC		0x00002000

#define SCB_PACKED	0x00000200
#define SCB_DOVER_MASK	0x00000180
#define SCB_PIPPOS	0x00000040
#define SCB_BGND	0x00000020
#define SCB_NOBLK	0x00000010
#define SCB_PIPA_MASK	0x0000000f

/* Sprite first preamble word bits */
#define PRE0_LITERAL	0x80000000
#define PRE0_BGND	0x40000000

#define PRE0_VCNT_MASK	0x0000ffc0
#define PRE0_LINEAR	0x00000010
#define PRE0_REP8	0x00000008
#define PRE0_BPP_MASK	0x00000007

#define PRE0_VCNT_SHFT	6

#define PRE0_BPP_1	0x00000001
#define PRE0_BPP_2	0x00000002
#define PRE0_BPP_4	0x00000003
#define PRE0_BPP_6	0x00000004
#define PRE0_BPP_8	0x00000005
#define PRE0_BPP_16	0x00000006

/* Sprite second preamble word bits */
#define PRE1_WOFFSET8_MASK	0xff000000
#define PRE1_WOFFSET10_MASK	0x03ff0000
#define PRE1_NOSWAP	0x00004000
#define PRE1_TLLSB_MASK	0x00003000
#define PRE1_LRFORM	0x00000800
#define PRE1_TLHPCNT_MASK	0x000007ff

#define PRE1_WOFFSET8_SHFT	24
#define PRE1_WOFFSET10_SHFT	16
#define PRE1_TLHPCNT_SHFT	0

#define PRE1_TLLSB_0	0x00000000
#define PRE1_TLLSB_IPN0	0x00001000
#define PRE1_TLLSB_IPN4	0x00002000
#define PRE1_TLLSB_IPN5	0x00003000




#define MAXRUN	64
#define MAXPIP	32

typedef struct string {
  ulong s[2048];
  int index, cindex, bitcount;
} string;

string tmpstr;

int width, height;
string *strlit[MAXRUN], *strpack[MAXRUN], *strtrns[MAXRUN];
int numlit, numpack, numtrans, tokensize, wcount, literalflag, bgndflag, pipsize, opffFlag;
int opffVerb, preInDataflag;
long transparenttoken, pre0bits, pre1bits;
FILE *infile, *outfile, *outfile2;
fpos_t topOfData;
int codedSelected;

long bpptable[] = {
  NULL, PRE0_BPP_1, PRE0_BPP_2, NULL, PRE0_BPP_4, NULL, PRE0_BPP_6, NULL,
  PRE0_BPP_8, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  PRE0_BPP_16,
};

ulong outarray[4096];
long pip[MAXPIP];


void getsmallest ();
void addliteral ();
void addpacked ();
void addtrans ();
void addbits ();
void initstring ();
void copystring ();
long translatetoken ();
long getoken ();
void makepip ();
void writepip ();
int  packline ();
void readword ();
void writeword ();

void printccb (CCC *ccb)
{

	printf("CCB =\n");
	printf("    flags: %x\n", (long) ccb->ccb_Flags );
	printf("    PPMPC: %x\n", (long) ccb->ccb_PPMPC );
	printf("    ccb_PRE0: %x\n", (long) ccb->ccb_PRE0 );
	printf("    ccb_PRE1: %x\n", (long) ccb->ccb_PRE1 );
	printf("    ccb_PIPPtr: %x\n", (long) ccb->ccb_PIPPtr );
	printf("    ccb_CelData: %x\n", (long) ccb->ccb_CelData );
	printf("    Width: %x\n",  ccb->ccb_Width);
	printf("    Height: %x\n",  ccb->ccb_Height);
	printf("    ccb_X: %x\n",  ccb->ccb_X);
	printf("    ccb_Y: %x\n", ccb->ccb_Y);
	printf("    ccb_hdx: %x\n",  ccb->ccb_hdx );
	printf("    ccb_hdy: %x\n", ccb->ccb_hdy);
	printf("    ccb_vdx: %x\n", ccb->ccb_vdx );
	printf("    ccb_vdy: %x\n",  ccb->ccb_vdy);
	printf("    ccb_ddx: %x\n", ccb->ccb_ddx );
	printf("    ccb_ddy: %x\n", (long) ccb->ccb_ddy);
	if (ccb->ccb_Flags & SCB_SCBPRE) 
		{
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 0) printf(" Bits per pixel is 0\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 1) printf(" Bits per pixel =  1\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 2) printf(" Bits per pixel =  2\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 3) printf(" Bits per pixel =  4\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 4) printf(" Bits per pixel =  6\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 5) printf(" Bits per pixel =  8\n");
		if ( ( ccb->ccb_PRE0 & PRE0_BPP_MASK) == 0) printf(" Bits per pixel = 16\n");
		}
}


int main (argc, argv)
int argc;
char *argv[];
{
int		i, j;
int		woffset;
int		longsWritten;
CCC		celHdr;
long	*pCelHdr;
fpos_t	pdatSizePos;

  width = 96;
  height = 192;
  tokensize = 16;
  transparenttoken = -1;
  pre0bits = PRE0_LINEAR | PRE0_REP8;	/* NJC:  default REP8 to ON */
  infile = outfile = outfile2 = NULL;
  literalflag = FALSE;
  opffFlag = TRUE;		// now always writing 3DO-format file
  opffVerb = FALSE;
  preInDataflag = FALSE;
  bgndflag = TRUE;
  codedSelected = FALSE;

  if (argc < 2) {
    usage ();
  }

  for (i=1; i<argc; i++) {
    parse (argv[i]);
  }

  /* Cross-check arguments */
  if (codedSelected == FALSE) {
  	fprintf(stderr, "Error - one of \"-c\" (coded) or \"-uc\" (uncoded) must be specified\n");
	exit (1);
  }
  if (pre0bits & PRE0_LINEAR) {
  	if ((tokensize != 8) && (tokensize != 16)) {
		fprintf(stderr, "Error - Uncoded cels must be either 8- or 16-bits per pixel\n");
		exit (1);
	}
  }

  if (!infile) {
    fprintf (stderr, "Error - no input file specified\n");
  }

  if (!outfile) {
    fprintf (stderr, "Error - no output file specified\n");
    usage ();
  }

  if (tokensize < 8) {
    pre0bits &= ~PRE0_LINEAR;
  }

  if (pre0bits&PRE0_LINEAR && outfile2) {
    fprintf (stderr, "Warning - no PIP generated for linear sprites\n");
  }

  if ( !(pre0bits&PRE0_LINEAR || outfile2) && (opffFlag == FALSE)) {
    fprintf (stderr, "Error - no PIP output file specified\n");
    usage ();
  }

  pre0bits |= ((height-1)<<PRE0_VCNT_SHFT) | bpptable[tokensize];
  /* fprintf (stderr, "%08lx ", pre1bits); */
  pre1bits |= ((width-1)<<PRE1_TLHPCNT_SHFT);
  /* fprintf (stderr, "%08lx ", pre1bits); */
  woffset = (width*tokensize+31) / 32  -  2;
  if (woffset<0) woffset = 0;
  pre1bits |= woffset<<((tokensize<8)?PRE1_WOFFSET8_SHFT:PRE1_WOFFSET10_SHFT);
  pre1bits |= PRE1_TLLSB_IPN0;
  /* fprintf (stderr, "%08lx\n", pre1bits); */

  if (!(pre0bits&PRE0_LINEAR)) {
    makepip ();
  }

  if (opffFlag == TRUE) {

  	celHdr.chunk_ID		= 'CCB ';
  	//celHdr.chunk_ID		= 'SPRH';
  	celHdr.chunk_size		= sizeof(celHdr);
  	celHdr.ccbversion		= 0;
  	celHdr.ccb_NextPtr		= 0;
  	celHdr.ccb_CelData		= 0;
  	celHdr.ccb_PIPPtr		= 0;
  	celHdr.ccb_X			= 0;
  	celHdr.ccb_Y			= 0;
  	celHdr.ccb_hdx			= 0x00100000;
  	celHdr.ccb_hdy			= 0;
  	celHdr.ccb_vdx			= 0;
  	celHdr.ccb_vdy			= 0x00010000;
  	celHdr.ccb_ddx			= 0;
  	celHdr.ccb_ddy			= 0;
  	celHdr.ccb_PPMPC		= 0x1F001F00;
  	celHdr.ccb_Width		= width;
  	celHdr.ccb_Height		= height;
	
	celHdr.ccb_Flags		= SCB_LDSIZE		/* Load hdx-y and vdx-y from SCoB */
							| SCB_LDPRS			/* Load ddx-y from SCoB */
							| SCB_LDPPMP		/* Load the PPMP word from SCoB */
							| SCB_ACW			/* Pixels facing forward will be seen */
							| SCB_ACCW			/* Pixels facing backward will be seen */
							| SCB_LAST			/* last sprite in the list for DrawSprites */
							| SCB_YOXY;			/* initially position sprite based on scb_X,Y */

  	celHdr.ccb_PRE0		= pre0bits;
	celHdr.ccb_PRE1		= 0;
	
	if (!(pre0bits&PRE0_LINEAR))
		celHdr.ccb_Flags |= SCB_LDPIP;			/* load the PIP data */
		
	if (!literalflag)
		celHdr.ccb_Flags |= SCB_PACKED;		/* data is RLE encoded */
	else
		celHdr.ccb_PRE1 = pre1bits;

	if (!(preInDataflag))
		celHdr.ccb_Flags |= SCB_SCBPRE;		/* Load PRE0 ( and PRE1 ?) from SCoB */
	
	if (bgndflag)
		celHdr.ccb_Flags |= SCB_BGND;			/* black pixels are NOT transparent */

	
	/* Write out the Sprite Header Chunk */

	if (opffVerb) printccb (&celHdr );
	pCelHdr = (long *)&celHdr;
	for (i=0; i<sizeof(celHdr);i+=4)
		writeword (outfile, *(pCelHdr++));
	
	/* Write out the PIP Data Chunk */

	if (!(pre0bits&PRE0_LINEAR)) {
		if (opffFlag == TRUE) {
		  writeword (outfile, 'PLUT');
		  //writeword (outfile, 'PIPT');
		  writeword (outfile, pipsize*2+12);	/* pipsize is in shorts: PIP entries are 1555 = 16bits */
		  writeword (outfile, pipsize);			/* number of entries in PIP Table */
		}
		writepip ();
	}
	

	/* Write out the Pixel Data Chunk headers */
	
	writeword (outfile, 'PDAT');
	fgetpos(outfile, &pdatSizePos);  
	writeword (outfile, 0L);
	
  }
  else
	if (!(pre0bits&PRE0_LINEAR))
		writepip ();

	/* Write out the preamble word(s) if they are not expected in the SCB */
	if (preInDataflag) {
		writeword (outfile, pre0bits);
		if (literalflag)
			writeword (outfile, pre1bits);
	}

  if (opffVerb) {
  	fprintf (stderr, "height is %d\n", height);
  	fprintf (stderr, "width is %d\n", width);
  }
  longsWritten = 0;
  for (i=0; i<height; i++) {
    wcount = width;
    longsWritten += packline ();
  }

  if (opffFlag == TRUE) {
	fsetpos(outfile, &pdatSizePos);		/* еее go back and set the 'PDAT' chunksize to: */
	j = (longsWritten << 2) + 8;		/* еее packed data size + chunk header */
	
	if (preInDataflag)
		j += (literalflag) ? 8:4;		/* еее + preamble size: 4 if packed, 8 if literal */
    
	writeword (outfile, j);
  }

return (0);
}


void
readword (file, word)
FILE *file;
char *word;
{
int	c, i;

  for (i=0;i<4;i++)
    word[i] = (char)getc(file);
}


void
writeword (file, word)
FILE *file;
long word;
{
  putc ((int)((word>>24)&0xff), file);
  putc ((int)((word>>16)&0xff), file);
  putc ((int)((word>>8)&0xff), file);
  putc ((int)(word&0xff), file);
}


long
translatetoken(t)
long t;
{
  long i;

  if (t<0) return t;
  if ((t==transparenttoken) && bgndflag && !literalflag) return 0x7fffffff;
  if (pre0bits&PRE0_LINEAR) {
    if (tokensize==16) {	/* if 16-bit linear sprite, return token unaltered */
      return t;
    } else {			/* if 8-bit linear sprite, return token reduced to 8 bits */
      return ((t>>7) & (0x7<<5)) + ((t>>5) & (0x7<<2)) + ((t>>3) & 0x3);
    }
  } else {
    for (i=0; i<pipsize; i++) {
      if (t==pip[i]) return i;
    }
    fprintf (stderr, "Internal error - token unmatched in PIP table (%lx)\n", t);
    exit (1);
  }
}



long
gettoken ()
{
  int i, j;
  long t;

  if (wcount--) {
    i = getc(infile);
    j = getc(infile);
    if (i<0 || j<0) {
      fprintf (stderr, "Error - unexpected EOF on input file\n");
      exit (1);
    }
    t = (long)i*256+j;
    if ((literalflag || !bgndflag) && (t==transparenttoken)) t=0;
    return t;
  } else {
    return (long)-1;
  }
}



void
makepip ()
{
  int i, j, k;
  long newtoken;

  pipsize = 2<<tokensize;
  if (pipsize>MAXPIP) {
    pipsize = MAXPIP;
  }

  for (i=0; i<pipsize; i++) {
    pip[i] = 0;
  }

  i = 0;
  for (j=0; j<height; j++) {
    wcount = width;
    while ( (newtoken=gettoken()) >= 0) {
      if ((newtoken==transparenttoken) && bgndflag && !literalflag) continue;
      if (newtoken==transparenttoken) {
	    newtoken = 0;
      }
      for (k=0; k<i; k++) {
		if (newtoken == pip[k]) break;
      }
      if (k==i) {
	if (i<pipsize) {
	  pip[i++] = newtoken;
	} else {
	  fprintf (stderr, "Error - more than %d different colors in input file\n", pipsize);
	  exit (1);
	}
      }
    }
  }

  /* rewind input file to just below the PDAT header (or TOF if no header) */
  fsetpos(infile, &topOfData);  
}


void
writepip ()
{
  int i, j, k;
  long newtoken;

  for (i=0; i<pipsize; i+=2) {
    newtoken = (pip[i]<<16) + pip[i+1];
	
    if (opffFlag == TRUE)
      writeword (outfile, newtoken);
	else
      writeword (outfile2, newtoken);
  }

}


int
packline ()
{
  long newtoken, oldtoken, t;
  int i, j, numwords, cnt, bitcnt;

  oldtoken = -1;
  numlit = 0;
  numpack = 0;
  numtrans = 0;

  for (i=0; i<MAXRUN; i++) {
    initstring (&strlit[i]);
    initstring (&strpack[i]);
    initstring (&strtrns[i]);
  }

  while ( (newtoken=translatetoken(t=gettoken())) >= 0) {
    getsmallest();
    addliteral (newtoken);
    if (!literalflag) {
      if (newtoken != oldtoken) {
	numtrans = 0;
	numpack = 0;
      }
      addpacked (newtoken);
      if (t==((bgndflag&&!literalflag)?transparenttoken:0)) {
	addtrans (newtoken);
      }
    }
    oldtoken = newtoken;
  }

  getsmallest ();

  outarray[0] = 0;
  bitcnt = 0;
  cnt = 0;
  i = (tokensize<8) ? 8 : 16;
  numwords = (tmpstr.bitcount + i + 31) / 32;
  if (numwords<2) numwords = 2;
  if (!literalflag) {
    addbits (outarray, numwords-2, i, &cnt, &bitcnt);
  }
  for (i=0; i<tmpstr.index; i++) {
    j = tmpstr.s[i];
    if (j&COUNTMARKER) {
      if (!literalflag) {
	addbits (outarray, j&0xff, 8, &cnt, &bitcnt);
      }
    } else {
      addbits (outarray, j, tokensize, &cnt, &bitcnt);
    }
  }

#if 0
  if (!literalflag) {
    while (((bitcnt>0) && (bitcnt<=24))  ||  (cnt<1) || ((cnt==1) && (bitcnt<=24))) {
      addbits (outarray, 0x80, 8, &cnt, &bitcnt);
    }
  }
#endif

  if (bitcnt) cnt++;
  if (cnt<2) cnt = 2;

  if ((!literalflag) && (numwords != cnt)) {
    fprintf (stderr, "Internal error - numwords != cnt (%d,%d)\n", numwords, cnt);
    for (i=0; i<tmpstr.index; i++) {
      fprintf (stderr, "%d: %lx\n", i, tmpstr.s[i]);
    }
    exit (1);
  }

  for (i=0; i<cnt; i++) {
    writeword (outfile, outarray[i]);
  }
  
  return cnt;
}


void
getsmallest ()
{
  int i;
  string *j;

  if (bgndflag && (!literalflag) && numtrans) {
    j = strtrns[numtrans-1];
    numlit = 0;
    numpack = 0;
  } else {
    j = strlit[0];
    for (i=1; i<numlit; i++) {
      if (strlit[i]->bitcount<j->bitcount) {
	j = strlit[i];
      }
    }
    for (i=0; i<numpack; i++) {
      if (strpack[i]->bitcount<j->bitcount) {
	j = strpack[i];
      }
    }
    for (i=0; i<numtrans; i++) {
      if (strtrns[i]->bitcount<j->bitcount) {
	j = strtrns[i];
      }
    }
  }

  copystring (&tmpstr, j);
  tmpstr.cindex = tmpstr.index;
}


void
addliteral (token)
long token;
{
  int i;
  string *j;

  if (numlit<MAXRUN) {
    numlit++;
  }
  if (numlit>1) {
    j = strlit[numlit-1];
    for (i=numlit-1; i>0; i--) {
      strlit[i] = strlit[i-1];
    }
    strlit[0] = j;
  }
  copystring (strlit[0], &tmpstr);
  strlit[0]->s[strlit[0]->index++] = COUNTMARKER+LITMARKER-1;
  strlit[0]->bitcount += 8;
  for (i=0; i<numlit; i++) {
    strlit[i]->s[strlit[i]->index++] = token;
    strlit[i]->s[strlit[i]->cindex]++;
    strlit[i]->bitcount += tokensize;
  }
}


void
addtrans (token)
long token;
{
  int i;
  string *j;

  if (numtrans<MAXRUN) {
    numtrans++;
  }
  if (numtrans>1) {
    j = strtrns[numtrans-1];
    for (i=numtrans-1; i>0; i--) {
      strtrns[i] = strtrns[i-1];
    }
    strtrns[0] = j;
  }
  copystring (strtrns[0], &tmpstr);
  strtrns[0]->s[strtrns[0]->index++] = COUNTMARKER+TRANSMARKER-1;
  strtrns[0]->bitcount += 8;
  for (i=0; i<numtrans; i++) {
    strtrns[i]->s[strtrns[i]->cindex]++;
  }
}


void
addpacked (token)
long token;
{
  int i;
  string *j;

  if (numpack<MAXRUN) {
    numpack++;
  }
  if (numpack) {
    j = strpack[numpack-1];
    for (i=numpack-1; i>0; i--) {
      strpack[i] = strpack[i-1];
    }
    strpack[0] = j;
  }
  copystring (strpack[0], &tmpstr);
  strpack[0]->s[strpack[0]->index++] = COUNTMARKER+PACKMARKER-1;
  strpack[0]->s[strpack[0]->index++] = token;
  strpack[0]->bitcount += 8+tokensize;
  for (i=0; i<numpack; i++) {
    strpack[i]->s[strpack[i]->cindex]++;
  }
}


void
initstring (str)
string **str;
{
  if (!*str) {
    if (!(*str = (string *)malloc(sizeof(string)))) {
      fprintf (stderr, "Unable to allocate memory for string( %xH bytes)\n",sizeof(string));
      exit (1);
    }
  }
  (*str)->index = 0;
  (*str)->cindex = 0;
  (*str)->bitcount = 0;
}


void
copystring (str1, str2)
string *str1, *str2;
{
  int i;

  str1->index = str2->index;
  str1->cindex = str2->cindex;
  str1->bitcount = str2->bitcount;

  for (i=0; i<str1->index; i++) {
    str1->s[i] = str2->s[i];
  }
}


void
addbits (outarray, i, numbits, cnt, bitcnt)
ulong outarray[];
ulong i;
int numbits;
int *cnt, *bitcnt;
{
  *bitcnt += numbits;
  if (*bitcnt <= 32) {
    outarray[*cnt] |= i<<(32-*bitcnt);
    if (*bitcnt==32) {
      *bitcnt = 0;
      outarray[++(*cnt)] = 0;
    }
  } else {
    outarray[*cnt] |= i>>(*bitcnt-32);
    *bitcnt -= 32;
    outarray[++(*cnt)] = i<<(32-*bitcnt);
  }
}


parse (option)
char *option;
{
int	  i;
ImageCC   imgChunk;
char	  *imgChunkPtr;
ulong	  ldata;
  FInfo 	fInfo;
  short	err;

  if (!(*option=='-' || *option=='+')) {
    if (!infile) {
      infile = fopen (option, "rb");
      if (!infile) {
		fprintf (stderr, "Error opening input file %s\n", option);
		exit (1);
      }
	  
	  imgChunkPtr = (char *)&imgChunk;
	  for (i=0;i<sizeof(imgChunk);i++)
	  imgChunkPtr[i] = (char)getc(infile);
	
	/* If this file starts with IMAG then this is an OPFF format file
	 * and the width and height can be extracted from the header. The
	 * data header must be skipped to get to the actual data.
	 * NOTE: An older form of the OPFF image file has no 'PDAT' header.
	 */
	if ( imgChunk.chunk_ID == imagetype )
	{
	  width = imgChunk.w;
	  height = imgChunk.h;
	  
	  /* Skip the PDAT header if present */
	  
	  readword (infile, (char *)&ldata);
	  if (ldata == 'PDAT')
	    readword (infile, (char *)&ldata);
	  else
	    fseek (infile, -4L, SEEK_CUR);	/* rewind 4 if no PDAT header */
	}
	else
	  fseek (infile, 0L, 0);	/* rewind to top of infile if not OPFF format */
	
	fgetpos(infile, &topOfData);  
    }
	else {
      if (!outfile) {
	outfile = fopen (option, "wb");
	if (!outfile) {
	  fprintf (stderr, "Error opening output file %s\n", option);
	  exit (1);
	}
      err = GetFInfo( c2pstr(option), 0, &fInfo );
      if (!err)
		{
		fInfo.fdType = '3DOc';
		fInfo.fdCreator = '3DOe';
		err = SetFInfo( option, 0, &fInfo );
		}
      p2cstr(option);	
      } else {
	if (!outfile2) {
	  outfile2 = fopen (option, "wb");
	  if (!outfile2) {
	    fprintf (stderr, "Error opening PIP output file %s\n", option);
	    exit (1);
	  }
	}
      }
    }
  } else {
    switch (tolower(option[1])) {
    case 'b':
      tokensize = strtol(&option[2], NULL, 0);
      if ( tokensize<0 || tokensize>16 || !bpptable[tokensize] ) {
	fprintf (stderr, "Error - illegal number of bits per pixel: %s\n", option);
	usage ();
      }
      break;
    case 't':
      transparenttoken = strtol(&option[2], NULL, 16);
      if (transparenttoken > 65535) {
	fprintf (stderr, "Error - illegal value for transparent color: %s\n", option);
	usage ();
      }
      bgndflag = FALSE;
      break;
    case 'u':
	  switch (tolower(option[2])) {
	  	case 'p':
			literalflag = TRUE;
			break;
		case 'c':
			pre0bits |= PRE0_LINEAR;
			codedSelected = TRUE;
			break;
		default:
			fprintf (stderr, "Error - unrecognized option: %s\n", option);
			usage ();
			break;
	  }
      break;
	case 'c':
	  pre0bits &= ~PRE0_LINEAR;
	  codedSelected = TRUE;
	  break;
	case 'r':
	  if (tolower(option[2]) == 'c')
		pre0bits &= ~PRE0_REP8;
	  else {
		fprintf (stderr, "Error - unrecognized option: %s\n", option);
		usage ();
	  }
	  break;
    case 'v':
    	opffVerb = TRUE;	/* write file out in Opera File Format */
	break;
	case 'd':
		preInDataflag = TRUE;	/* preamble words are in data, not SCB */
	break;
	case 'f':
		bgndflag = FALSE;
		break;
    default:
      fprintf (stderr, "Error - unrecognized option: %s\n", option);
      usage ();
    }
  }
}


usage ()
{
  fprintf (stderr, "\nUsage:\n");
  fprintf (stderr, "\tmakecel [options] imagefile celfile\n\n");
  fprintf (stderr, "makecel reads a non-LR-form 3DO image from the file specified\n");
  fprintf (stderr, "by \"imagefile\" on the command line, and writes a 3DO cel to the\n");
  fprintf (stderr, "file specified as \"celfile\" on the command line.  If the output\n");
  fprintf (stderr, "format is other than an 8- or 16-bit uncoded cel then a PLUT will\n");
  fprintf (stderr, "also be written to the file \"celfile\"\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Valid options are:\n");
  fprintf (stderr, "\t-b<n>\tspecify number of bits per pixel for output cel (default: 16)\n");
  fprintf (stderr, "\t-f\tthis is a foreground cel (clears BGND bit); black is transparent\n");
  fprintf (stderr, "\t-t<n>\tspecify value (hex 555-RGB) of transparent color in input (clears BGND bit)\n");
  fprintf (stderr, "\t-up\tspecify that output is not packed\n");
  fprintf (stderr, "\t-v\tprint information about cel that is generated\n");
  fprintf (stderr, "\t-d\tput preamble words in the data, not the CCB\n");
  fprintf (stderr, "\t-uc\tcel data is uncoded\n");
  fprintf (stderr, "\t-rc\tclear the REP8 bit in the first cel preamble word\n");
  fprintf (stderr, "The number of bits per pixel must be one of:  1, 2, 4, 6, 8 or 16.\n");
  fprintf (stderr, "The LINEAR bit will be cleared if the number of bits/pixel is less than 8.\n");
  fprintf (stderr, "The second preamble word is only generated for totally literal sprites.\n");
  fprintf (stderr, "Numeric values are in the same format as C constants.\n");
  fprintf (stderr, "It is possible to have a packed background sprite that also has\n");
  fprintf (stderr, "transparency within it.  To do this, set the transparent color (-t option)\n");
  fprintf (stderr, "and then set the BGND bit (-g option) afterwards on the command line.\n");
  exit (1);
}



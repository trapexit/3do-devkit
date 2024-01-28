/*
        File:		AnimToSanm.c

        Contains:	MPW IFF Application to convert 3DO Animator files to
                                the 3DO streamed animation format (SANM).

        Written by:	Bart Besseling

        Copyright © 1993 The 3DO Company. All Rights Reserved.

        Change History (most recent first):
                                10/19/93	jb		Add
   PROGRAM_VERSION_STRING to usage <1>	  7/20/93	BJB		first
   checked in

        To Do:
*/

#define PROGRAM_VERSION_STRING "1.0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------
 *  supported datastructures
 * ------------------------------------------------------------
 */

typedef unsigned long Int32;

typedef struct
{
  Int32 type;
  Int32 length;
} chunk_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 version;    /*  current version = 0 */
  Int32 animType;   /* 	0 = multi-CCB ; 1 = single CCB  */
  Int32 numFrames;  /* 	number of frames for this animation */
  Int32 frameRate;  /* 	number of 1/60s of a sec to display each frame */
  Int32 startFrame; /* 	the first frame in the anim. Can be non zero */
  Int32 numLoops;   /* 	number of loops in loop array. Loops are executed
                       serially */
} anim_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 ccbversion;
  Int32 ccb_Flags;
  Int32 ccb_NextPtr;
  Int32 ccb_CelData;
  Int32 ccb_PLUTPtr;
  Int32 ccb_X;
  Int32 ccb_Y;
  long ccb_hdx;
  long ccb_hdy;
  long ccb_vdx;
  long ccb_vdy;
  long ccb_ddx;
  long ccb_ddy;
  Int32 ccb_PIXC;
  Int32 ccb_PRE0;
  Int32 ccb_PRE1;
  long ccb_Width;
  long ccb_Height;
} ccb_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 time;
  Int32 channel;
  Int32 subtype;
} stream_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 time;
  Int32 channel;
  Int32 subtype;
  Int32 ccb_Flags;
  Int32 ccb_NextPtr;
  Int32 ccb_CelData;
  Int32 ccb_PLUTPtr;
  Int32 ccb_X;
  Int32 ccb_Y;
  long ccb_hdx;
  long ccb_hdy;
  long ccb_vdx;
  long ccb_vdy;
  long ccb_ddx;
  long ccb_ddy;
  Int32 ccb_PIXC;
  Int32 ccb_PRE0;
  Int32 ccb_PRE1;
  long ccb_Width;
  long ccb_Height;
} accb_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 time;
  Int32 channel;
  Int32 subtype;
  Int32 duration;
} frme_header;

typedef struct
{
  Int32 type;
  Int32 length;
  Int32 time;
  Int32 channel;
  Int32 subtype;
  Int32 version;   /*  current version = 0			*/
  Int32 animType;  /* 	3 = streamed Anim			*/
  Int32 numFrames; /* 	number of frames in anim	*/
  Int32 frameRate; /* 	in frames per second		*/
} sanm_header;

/* ------------------------------------------------------------
 *  global data
 * ------------------------------------------------------------
 */

#define MAX_CHUNK (1 * 1024 * 1024)

#define FTL(f) ((double)(f / 65536.0))

char buf[4 * 1024], buf2[4 * 4096];

Int32 timeNow = 0;
Int32 timeInc = 16;
int optSkipPDAT = 0;

/* ------------------------------------------------------------
 *  parse options and process arguments
 * ------------------------------------------------------------
 */

int
main (int argc, char **argv)
{
  int c;
  extern char *optarg;
  extern int optind;
  int errflg = 0;
  char *ofile = "stdout";
  int getopt (int argc, char *const *argv, const char *optstring);
  int process (char *arg);

  while ((c = getopt (argc, argv, "o:1")) != EOF)
    switch (c)
      {

      case 'o':
        if (freopen (optarg, "w", stdout) == NULL)
          {
            perror (optarg);
            exit (1);
          }
        break;

      case '1':
        optSkipPDAT = 1;
        break;

      default:
        errflg = 1;
      }

  if (errflg)
    {

      fprintf (stderr, "usage: %s version %s [-1] [-o ofile] files...\n",
               argv[0], PROGRAM_VERSION_STRING);
      exit (EXIT_FAILURE);
    }

  if (optind >= argc)
    {

      process ("stdin");
    }
  else
    for (; optind < argc; optind++)
      {

        if (freopen (argv[optind], "r", stdin) == NULL)
          {
            perror (argv[optind]);
            exit (1);
          }

        process (argv[optind]);
      }

  return EXIT_SUCCESS;
}

/* ------------------------------------------------------------
 *  get_remainder  --  get remainder of structure
 * ------------------------------------------------------------
 */

void
get_remainder (chunk_header *h, int size)
{
  fprintf (stderr, "read %d\n", h->length);
  if (h->length > sizeof (buf))
    {
      fprintf (stderr,
               "chunk '%c%c%c%c' (0x%08x) of %d is larger than %d bytes\n",
               (h->type >> 24) & 0xff, (h->type >> 16) & 0xff,
               (h->type >> 8) & 0xff, h->type & 0xff, h->type, h->length,
               sizeof (buf));
      fprintf (stderr, "<abandoned>\n");
      exit (EXIT_FAILURE);
    }

  if (size > 0 && h->length != size)
    {
      fprintf (stderr, "expected chunk of %d bytes\n", size);
      fprintf (stderr, "<abandoned>\n");
      exit (EXIT_FAILURE);
    }

  if (fread (h + 1, 1, h->length - sizeof (chunk_header), stdin)
      != h->length - sizeof (chunk_header))
    {
      fprintf (stderr, "partial chunk\n");
      fprintf (stderr, "<abandoned>\n");
      exit (EXIT_FAILURE);
    }
}

/* ------------------------------------------------------------
 *  put_ahdr_chunk  --  construct an AHDR chunk from an ANIM chunk
 * ------------------------------------------------------------
 */

void
put_ahdr_chunk (chunk_header *h)
{
  sanm_header a;
  anim_header *b;

  b = (anim_header *)h;

  a.type = 'SANM';
  a.length = sizeof (a);
  a.time = timeNow;
  a.channel = 0;
  a.subtype = 'AHDR';
  a.version = 0;
  a.animType = 3;
  a.numFrames = b->numFrames;
  a.frameRate = (int)(60.0 / FTL (b->frameRate) + 0.5);
  timeInc = 240 / a.frameRate;

  if (fwrite (&a, sizeof (a), 1, stdout) != 1)
    {
      fprintf (stderr, "error while writing\n");
      exit (EXIT_FAILURE);
    }
}

/* ------------------------------------------------------------
 *  put_accb_chunk  --  construct an ACCB chunk from a CCB chunk
 * ------------------------------------------------------------
 */

void
put_accb_chunk (chunk_header *h)
{
  accb_header a;
  ccb_header *b;

  b = (ccb_header *)h;

  a.type = 'SANM';
  a.length = sizeof (a);
  a.time = timeNow;
  a.channel = 0;
  a.subtype = 'ACCB';
  memcpy ((char *)&a.ccb_Flags, (char *)&b->ccb_Flags,
          sizeof (a) - sizeof (stream_header));

  if (fwrite (&a, sizeof (a), 1, stdout) != 1)
    {
      fprintf (stderr, "error while writing\n");
      exit (EXIT_FAILURE);
    }
}

/* ------------------------------------------------------------
 *  put_frme_chunk  --  construct a FRME chunk from a PDAT chunk
 * ------------------------------------------------------------
 */

void
put_frme_chunk (chunk_header *h)
{
  frme_header a;
  int len;

  if (optSkipPDAT)
    {

      optSkipPDAT--;

      for (h->length -= sizeof (chunk_header); h->length > 0; h->length -= len)
        {

          if (h->length > sizeof (buf2))
            len = sizeof (buf2);
          else
            len = h->length;

          if (fread (buf2, 1, len, stdin) != len)
            {
              fprintf (stderr, "error while reading PDAT data\n");
              exit (EXIT_FAILURE);
            }
        }

      return;
    }

  a.type = 'SANM';
  a.length = sizeof (a) + h->length;
  a.time = timeNow;
  a.channel = 0;
  a.subtype = 'FRME';
  a.duration = 0;

  if (fwrite (&a, sizeof (a), 1, stdout) != 1)
    {
      fprintf (stderr, "error while writing\n");
      exit (EXIT_FAILURE);
    }

  if (fwrite (h, sizeof (chunk_header), 1, stdout) != 1)
    {
      fprintf (stderr, "error while writing\n");
      exit (EXIT_FAILURE);
    }

  for (h->length -= sizeof (chunk_header); h->length > 0; h->length -= len)
    {

      if (h->length > sizeof (buf2))
        len = sizeof (buf2);
      else
        len = h->length;

      if (fread (buf2, 1, len, stdin) != len)
        {
          fprintf (stderr, "error while reading PDAT data\n");
          exit (EXIT_FAILURE);
        }
      if (fwrite (buf2, 1, len, stdout) != len)
        {
          fprintf (stderr, "error while writing PDAT data\n");
          exit (EXIT_FAILURE);
        }
    }
  timeNow += timeInc;
}

/* ------------------------------------------------------------
 *  process one file (on stdin)
 * ------------------------------------------------------------
 */

int
process (char *arg)
{
  int len;
  chunk_header *h;

  fprintf (stderr, "\nfile: %s\n", arg);

  for (h = (chunk_header *)buf, h->type = h->length = 0;
       fread (buf, 1, sizeof (chunk_header), stdin) == sizeof (chunk_header);
       h->type = h->length = 0)
    {
    recover:

      fprintf (stderr, "type = '%c%c%c%c', (0x%08x), length = %ld (0x%08x)",
               (h->type >> 24) & 0xff, (h->type >> 16) & 0xff,
               (h->type >> 8) & 0xff, h->type & 0xff, h->type, h->length,
               h->length);

      switch (h->type)
        {

        case 'ANIM':
          fprintf (stderr, " animation info\n");
          get_remainder (h, sizeof (anim_header));
          put_ahdr_chunk (h);
          break;

        case 'CCB ':
          fprintf (stderr, " cel control block\n");
          get_remainder (h, sizeof (ccb_header));
          put_accb_chunk (h);
          break;

        case 'CRDT':
          fprintf (stderr, " text credits\n");
          goto skip_chunk;

        case 'CTRL':
          fprintf (stderr, " control chunk\n");
          goto skip_chunk;

        case 'CYPR':
          fprintf (stderr, " copyright notice\n");
          goto skip_chunk;

        case 'DESC':
          fprintf (stderr, " text description\n");
          goto skip_chunk;

        case 'IMAG':
          fprintf (stderr, " image control\n");
          goto skip_chunk;

        case 'KWRD':
          fprintf (stderr, " text keywords\n");
          goto skip_chunk;

        case 'PLUT':
          fprintf (stderr, " pixel lookup table\n");
          goto skip_chunk;

        case 'VDL ':
          fprintf (stderr, " video display list\n");
          goto skip_chunk;

        case 'FPDT':
          h->type = 'PDAT';
        case 'PDAT':
          fprintf (stderr, " pixel data\n");
          put_frme_chunk (h);
          break;

        case 'XTRA':
          fprintf (stderr, " extra info\n");
        skip_chunk:
          if (h->length >= MAX_CHUNK)
            {
              fprintf (stderr, "chunk length suspiciously large\n");
              exit (EXIT_FAILURE);
            }

          for (h->length -= sizeof (chunk_header); h->length > 0;
               h->length -= len)
            {

              if (h->length > sizeof (buf) - sizeof (chunk_header))
                len = sizeof (buf) - sizeof (chunk_header);
              else
                len = h->length;

              if (fread (h + 1, 1, len, stdin) != len)
                {
                  perror (arg);
                  exit (EXIT_FAILURE);
                }
            }
          break;

        default:
          fprintf (stderr, " not recognized\n");
          fprintf (stderr, "<abandoned>\n");
          return EXIT_FAILURE;
          break;
        }
    }
  // fprintf(stderr, "EOF\n");

  return EXIT_SUCCESS;
}

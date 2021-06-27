/*
	File:		greentest.c

	Contains:	Program to perform memory tests and other diagnostics on
			the GREEN Sherrie hardware

	Written by:	Stephen H. Landrum

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				 5/09/94	crm		removed explicit references to GrafBase struct
		 <1>	 8/20/93	JAY		first checked in

	To Do:
*/

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdlib.h"
#include "stdio.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

void *mem1, *mem2;
int32 size1, size2;
static ScreenContext *sc;
GrafCon			gGrafCon;

ubyte	*gBackPic = NULL;
int MACEXISTS = 0;

long	gScreenSelect = 0;
 char backfile[] = "vspics/reef.img";
char sbuf [100];

#define GRAPHICSMASK 	1
#define AUDIOMASK 		2		
#define SPORTIOMASK 	4
#define MACLINKMASK 	8
#define DBUGXY(s , x, y )	{ sprintf  s ;  /* if (MACEXISTS) printf ("%s", sbuf); */  gGrafCon.gc_PenX = x; \
	gGrafCon.gc_PenY = y; \
	DrawText8(&gGrafCon, sc->sc_BitmapItems[ 0 ], sbuf);  }

#define DBG( s )	{ sprintf s  ;  if (MACEXISTS) printf ("%s", sbuf); }

Item VRAMIOReq;
extern void kprintf (const char *fmt, ...);

extern void memtest (uint32 seed, void *lomem, void *himem, uint32 mask);
extern void memtest2 (uint32 seed, void *lomem, void *himem, uint32 mask);

// extern const char BuildDate[];

// #define DBG(x)  printf x
 #define DBUG(x)  { if (MACEXISTS) printf x ;}

int32 grabmem (void **m);

uint32 errorcount[32];
uint32 passmem (uint32 numpasses);


long StartUp(ScreenContext *sc)
	{
	long retval = 0;
	MACEXISTS = FALSE;
	if (!OpenGraphics(sc,1))
		{
		DIAGNOSTIC("Cannot initialize graphics");
		}
	else retval |= GRAPHICSMASK;
	

	if (!OpenMacLink())
		{
		DIAGNOSTIC("Cannot communicate with Mac");
		}
	else MACEXISTS = TRUE;
	return retval;	
	}


bool
InitBackPic( char *filename, ScreenContext *sc )
/* Allocates the BackPic buffer.  If a filename is specified the routine 
 * loads a picture from the Mac for backdrop purposes.  Presumes that 
 * the Mac link is already opened.  If no filename is specified, this 
 * routine merely clears the BackPic buffer to zero.  
 * 
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
	bool retvalue;

	retvalue = FALSE;
	
	gBackPic = (ubyte *)ALLOCMEM( (int)(sc->sc_nFrameBufferPages*GetPageSize(MEMTYPE_VRAM)),
			MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

	if ( NOT gBackPic)
		{
		DBUG (( "unable to allocate BackPic" ));
		goto DONE;
		}

	SetVRAMPages( VRAMIOReq, gBackPic, 0, sc->sc_nFrameBufferPages, -1 );

	if (LoadImage( filename,  gBackPic, (VdlChunk **)NULL, sc ) != gBackPic ) {
		DBUG(( "LoadImage failed" ));
		goto DONE;
	}
	retvalue = TRUE;
	
DONE:
	return(retvalue);
}

int setup()
{
	long opened;
	sc = (ScreenContext *) ALLOCMEM(sizeof(ScreenContext),MEMTYPE_CEL );
	if (sc == NULL)
		{
		DIAGNOSTIC("Cannot Allocate memory for ScreenContext ");
		return FALSE;
		}
	
	gBackPic = NULL;
	gScreenSelect = 0;
 

	opened = StartUp(sc);
	if ( opened == 0) return ( 0 );

	VRAMIOReq = GetVRAMIOReq();
	
	SetVRAMPages( VRAMIOReq, sc->sc_Bitmaps[0]->bm_Buffer, 
			0, sc->sc_nFrameBufferPages, 0xffffffff );
	
	DisplayScreen( sc->sc_Screens[0], 0 );
	SetFGPen( &gGrafCon, MakeRGB15(4,0,6) );
	return (0);
}

void
CopyBackPic( Bitmap *bitmap )
{
	CopyVRAMPages( VRAMIOReq, bitmap->bm_Buffer, gBackPic, sc->sc_nFrameBufferPages, -1 );
}


int
main ()
{
  uint32 i;
  uint32 k;
  setup();

//  DBG (("Green system test code\n%s\n\n", BuildDate));

  size1 = grabmem (&mem1);
//  size2 = grabmem (&mem2);
//  size2 = 768*1024;
//  mem2 = malloc(size2);
  k = 768;
  mem2 = NULL;
  while ( mem2 == NULL)
  	{
  	size2 = k*1024;
	mem2 = AllocMem(size2,MEMTYPE_VRAM);
	k--;
	}
  if (!mem2) {
    DBUGXY ((sbuf, "Error allocating VRAM\n"), 20, 20);
    for (i = 0 ; i< 0xc0000; i++ ) k = i;
	exit(1);
  }

  DBUGXY ((sbuf, "DRAM = %6lx, size = %6lx\n", (uint32)mem1, size1),20,30);
  DBUGXY ((sbuf, "VRAM = %6lx, size = %6lx\n", (uint32)mem2, size2),20,40);

  DBG ((sbuf, "Now run memory checks\n"));
  i = passmem (0xffffffffL);

  if (MACEXISTS) printf("Memtest exiting \n");
  
}

uint32 erry =0;

void incxy ()
{

if (erry == 0) erry = 60;
erry += 30;
if (erry > 180) erry = 60;
}

void
memerror (void)
{
  static uint32 oldcount = 0;

  if (oldcount!=errorcount[0]) {
    incxy();
    DBUGXY((sbuf, "ERR AT %6lx\n", errorcount[1]), 20,erry);
    DBUGXY ((sbuf, "%8lx %8lx %8lx %8lx\n",
	  errorcount[4+0], errorcount[4+1], errorcount[4+2], errorcount[4+3]),22,erry+10);
	DBUGXY ((sbuf, "%8lx %8lx %8lx %8lx\n",
	  errorcount[8+0], errorcount[8+1], errorcount[8+2], errorcount[8+3]), 22,erry+20);
 	oldcount = errorcount[0];
  }
}


int32
grabmem (void **m)
{
  int32 s, ss;

  s = ss = 8*1024*1024;
  while (ss>16) {
    ss >>= 1;
    DBG ((sbuf, "Attempt to allocate %lx bytes of memory\n", s));
    *m = malloc(s);
    if (*m) {
      free (*m);
      s += ss;
    } else {
      s -= ss;
    }
  }
  s -= ss;
  *m = malloc(s);
  if (!*m) {
    DBG ((sbuf, "Error grabbing memory???\n"));
    exit (1);
  }
  return s;
}

uint32 Paused = 0;

uint32
passmem (uint32 numpasses)
{
  uint32 i;
  static uint32 r = 0xffffffff;
  uint32 curJoy;

  i = 0;

  errorcount[0] = errorcount[1] = errorcount[2] = 0;
  DBUGXY ((sbuf, "MEMTEST:" ), 60,20 );

  for (; numpasses>0; numpasses--) {
 	curJoy = ReadControlPad( 0);
   if ((curJoy & JOYSTART) ) {return 0;}
   if ((curJoy & JOYFIREA) ) 
   	 {
	 Paused = 1- Paused;
	 if (Paused) { DBUGXY( (sbuf, "PAUSED"), 160, 20);}
      else {DBUGXY((sbuf, "      "), 160, 20); };
	  }
	if (!Paused)
		{
		memtest (r, (void *)((uint32)mem1+i*4), (void *)((uint32)mem1+size1-32), 0xffffffff);
	    memerror ();
	    memtest (r, (void *)((uint32)mem2+i*4), (void *)((uint32)mem2+size2-32), 0xffffffff);
	    memerror ();
	    memtest (0, (void *)((uint32)mem1+i*4), (void *)((uint32)mem1+size1-32), 0xffffffff);
	    memerror ();
	    memtest (0, (void *)((uint32)mem2+i*4), (void *)((uint32)mem2+size2-32), 0xffffffff);
	    memerror ();
	    memtest2 (r, (void *)((uint32)mem1+i*4), (void *)((uint32)mem1+size1-32), 0xffffffff);
	    memerror ();
	    memtest2 (r, (void *)((uint32)mem2+i*4), (void *)((uint32)mem2+size2-32), 0xffffffff);
	    memerror ();
	    memtest2 (0, (void *)((uint32)mem1+i*4), (void *)((uint32)mem1+size1-32), 0xffffffff);
	    memerror ();
 	    memtest2 (0, (void *)((uint32)mem2+i*4), (void *)((uint32)mem2+size2-32), 0xffffffff);
 	    memerror ();
   		if (++i==8) i=0;
 	    r = rand();
  	    errorcount[2] += 1;
   		DBUGXY ((sbuf, "pass %6ld errorcount %6ld\n", errorcount[2], errorcount[0]), 20,50 );
		}	
 
  }
  return errorcount[0];
}














































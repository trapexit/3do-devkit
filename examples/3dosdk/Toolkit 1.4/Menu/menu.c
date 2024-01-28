/*
        File:		menu.c

        Contains:	menu task for sublaunching 3DO applications

        Written by:	RJ Mical

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 5/09/94	crm		removed
   explicit references to GrafBase struct <8>	 7/30/93	JAY
   added IO parameter block for SetVRAMPages() and CopyVRAMPages(). Set IOInfo
   blocks to zero before assigning values. Modified MySportIO.
                 <5>	 6/25/93	dsm		Changed Hilighting
   mechanism to positional since I couldn't get the font_CCB control to work.
                 <4>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL for Cardinal2
                 <4>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL for Cardinal2
                 <3>	  4/5/93	JAY		more work on JoyPad for
   Cardinal and added ChangeInitialDirectory to support running from /CD-ROM or
                                                                        /remote
                 <2>	  4/2/93	JAY		rehosting menu to run
   with the new eventBroker under Cardinal1 and real red <1>	  4/1/93
   JAY		first checked in

        To Do:
*/

#include "debug.h"
#include "device.h"
#include "discdata.h"
#include "driver.h"
#include "event.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "graphics.h"
#include "io.h"
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "types.h"

#include "folio.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"

#include "directory.h"
#include "directoryfunctions.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "filefunctions.h"

#undef KERNEL

/*???#ifndef ARMC*/
/*???#include <stdlib.h>*/
/*???#endif*/

#include "strings.h"

long ReadAFile (char *filename, char *buf, long count);
char *AllocAndLoadFile (char *filename, ulong memtype);
int32 Init (void);
int32 OpenMenuDisplay (void);
void CloseMenuDisplay (void);
int32 GetMenuSelection (void);
void RenderMenu (int32 fillrect);
char *GetAtom (char *atom, char *line);

void FillDisplay (GrafCon *gcon, long red, long green, long blue);
void RandomFillDisplay (GrafCon *gcon, long maxred, long maxgreen,
                        long maxblue);
int32 InitBackPic (char *filename);
void CopyBackPic (Bitmap *bitmap);
void ClearBitmaps (Bitmap *bitmap0, Bitmap *bitmap1);
void ClearBitmap (Bitmap *bitmap);
int32 myOpenSPORT (void);

long GetJoystick (void);

ulong Random (ulong);

void InitFont (void);
void CloseAFont (void);

void LaunchProgram (int32 program);

#define VERSION "0.2"

/*???#define MENU_LINE_COUNT (240/(11+3))*/
#define MENU_LINE_COUNT 240 / 8

#define SCREEN_WIDTH 320
#define DISPLAY_WIDTH 320
#define SCREEN_HEIGHT 240
#define DISPLAY_HEIGHT 240

#define RED_MASK 0x7C00
#define GREEN_MASK 0x03E0
#define BLUE_MASK 0x001F
#define RED_SHIFT 10
#define GREEN_SHIFT 5
#define BLUE_SHIFT 0
#define ONE_RED (1 << REDSHIFT)
#define ONE_GREEN (1 << GREENSHIFT)
#define ONE_BLUE (1 << BLUESHIFT)
#define MAX_RED (RED_MASK >> RED_SHIFT)
#define MAX_GREEN (GREEN_MASK >> GREEN_SHIFT)
#define MAX_BLUE (BLUE_MASK >> BLUE_SHIFT)

/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20 (1 << 20)
#define ONE_16_16 (1 << 16)

/* The JOYCONTINUOUS definition specifies those joystick flags that we
 * want to know about all the time, whenever the associated switches
 * are depressed, not just when they make the transition from undepressed
 * to depressed (which is the default).
 */
#define JOYCONTINUOUS (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT)
/*???#define JOYCONTINUOUS  (JOYSTART)*/

#define printf kprintf

/* === RJ's Idiosyncracies === */
#define NOT !
#define FOREVER for (;;)
#define SetFlag(v, f) ((v) |= (f))
#define ClearFlag(v, f) ((v) &= ~(f))
#define FlagIsSet(v, f) ((bool)(((v) & (f)) != 0))
#define FlagIsClear(v, f) ((bool)(((v) & (f)) == 0))
#define Error(s) kprintf ("ERROR:  %s\n", s)
#define Error2(s, arg) kprintf ("ERROR:  %s %s\n", s, arg)
#define Warning(s) kprintf ("WARNING:  %s\n", s)

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Data Declarations    ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

int32 MaxCharacterCount;

int32 myFontPLUT[] = {
  0x18C66318,
  0x63186318,
  0x63186318,
  0x63186318,
};

CCB MenuFontCCB = {
  /* ulong ccb_Flags; */
  CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS
      | CCB_LDPPMP | CCB_LDPLUT | CCB_YOXY | CCB_ACW | CCB_ACCW | PMODE_ZERO,
  /*???CCB_PACKED*/

  /* struct CCB *ccb_NextPtr; CelData *ccb_CelData; void *ccb_PLUTPtr; */
  &MenuFontCCB,
  NULL,
  myFontPLUT,

  /* Coord ccb_X; Coord ccb_Y; */
  /* long ccb_HDX, ccb_HDY, ccb_VDX, ccb_VDY, ccb_DDX, ccb_DDY; */
  0,
  0,
  ONE_12_20,
  0,
  0,
  ONE_16_16,
  0,
  0,

  /* ulong ccb_PPMPC; */
  (PPMP_MODE_NORMAL << PPMP_0_SHIFT) | (PPMP_MODE_AVERAGE << PPMP_1_SHIFT),
};

Font MenuFont = {
  /* uint8     font_Height; */
  8,
  /* uint8     font_Flags; */
  FONT_ASCII | FONT_ASCII_UPPERCASE,
  /* CCB      *font_CCB; */
  &MenuFontCCB,
  /* FontChar *font_FontCharArray[]; */
  NULL,
};

Font *SystemFont;

int32 MenuSelect = 0;

int32 RawNewJoy;

Rect WorkRect;

ubyte *BackPic = NULL;

int32 ScreenSelect = 0;

char ProgramLines[MENU_LINE_COUNT][256];
char MenuAtoms[MENU_LINE_COUNT][256];
int32 MenuLineCount;

void *BitmapBufPtr;

TagArg ScreenTags[] = { CSG_TAG_BITMAPBUF_ARRAY,
                        (void *)(&BitmapBufPtr),
                        CSG_TAG_SCREENCOUNT,
                        (void *)1,
                        CSG_TAG_DONE,
                        0 };

Item ScreenGroupItem = 0;
Item ScreenItems[2] = { 0 };
Bitmap *Bitmaps[2];
Item BitmapItems[2];
Item VRAMIOReqItem;

CelData TestData[] = {
  /* Cel first preamble word bits */
  ((14 - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) | PRE0_LINEAR | PRE0_BPP_16,

  /* Cel second preamble word bits */
  ((6 - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT)
      | ((12 - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT),

  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFF0000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x0000FFFF,
  0xFFFF0000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x0000FFFF,
  0xFFFF0000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x00000000,
  0x0000FFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF,
};

struct CCB TestCCB = {
  /* ulong ccb_Flags; */
  CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | PMODE_ONE
      | CCB_LDPRS | CCB_LDPPMP | CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_BGND,

  /* struct CCB *ccb_NextPtr;Cel *ccb_CelData;void *ccb_PLUTPtr; */
  NULL, TestData, NULL,

  /* Coord ccb_X, ccb_Y; */
  /* long ccb_hdx, ccb_hdy, ccb_vdx, ccb_vdy, ccb_ddx, ccb_ddy; */
  0, 0, ONE_12_20, 0, 0, ONE_16_16, 0, 0,

  /* ulong ccb_PPMPC; */
  (PPMP_MODE_NORMAL << PPMP_0_SHIFT) | (PPMP_MODE_AVERAGE << PPMP_1_SHIFT),

  /* long ccb_Width, ccb_Height; */
  0, 0
};

ulong ScreenPageCount = 0;
long ScreenByteCount = 0;
Item mySPORTIO[2] = { -1 };

int32 AllocAndLoadLen;
char RootName[256];
int32 oldbits = 0;
int32 upcount = 0;
int32 downcount = 0;

extern bool EventUtilityInited;
Item VRAMIOReq;

int
main (int32 argc, char **argv)
{
  int32 i, i2;
  char *progname;
  char *scriptname;
  char *scriptbuffer, *nextline, *lastchar;
  int32 line, program;

  progname = *argv++;
  scriptname = "menu.script";

  if (argc == 2)
    {
      scriptname = *argv++;
    }
  if (argc > 2)
    {
      printf ("Usage:  %s <scriptfile>\n", progname);
      goto DONE;
    }

  if (Init () < 0)
    goto DONE;

  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_NODBGR))
    strcpy (RootName, "/cd-rom/");
  else
    strcpy (RootName, "/remote/");

  VRAMIOReqItem = GetVRAMIOReq ();

  scriptbuffer = AllocAndLoadFile (scriptname, MEMTYPE_FILL | 0x00);
  if (scriptbuffer == NULL)
    {
      printf ("Error:  Couldn't allocate memory for script file\n");
      goto DONE;
    }
  nextline = scriptbuffer;
  lastchar = scriptbuffer + AllocAndLoadLen - 1;
  while (nextline <= lastchar)
    {
      if (*nextline < 0x20)
        *nextline = '\0';
      nextline++;
    }

  nextline = scriptbuffer;
  line = 0;
  MenuLineCount = 0;
  MaxCharacterCount = 0;
  FOREVER
  {
    if (*nextline && (*nextline != ';'))
      {
        nextline = GetAtom (MenuAtoms[line], nextline);
        while (*nextline == ' ')
          nextline++;
        strcpy (ProgramLines[line], nextline);
        i = strlen (MenuAtoms[line]);
        if (MaxCharacterCount < i)
          MaxCharacterCount = i;
        line++;
        MenuLineCount++;
        if (line >= MENU_LINE_COUNT)
          goto MENU_NAMES_GOTTEN;
      }

    while (*nextline++ && (nextline <= lastchar))
      ;
    if (nextline > lastchar)
      goto MENU_NAMES_GOTTEN;
  }
MENU_NAMES_GOTTEN:

  while (strcmp (MenuAtoms[0], "STARTUP:") == 0)
    {
      LaunchProgram (0);
      for (i = 1; i < MenuLineCount; i++)
        {
          strcpy (MenuAtoms[i - 1], MenuAtoms[i]);
          strcpy (ProgramLines[i - 1], ProgramLines[i]);
        }
      MenuLineCount--;
    }

  for (i = 0; i < MenuLineCount; i++)
    {
      for (i2 = strlen (MenuAtoms[i]); i2 < MaxCharacterCount; i2++)
        strcat (MenuAtoms[i], " ");
    }

  FOREVER
  {
    if (OpenMenuDisplay () < 0)
      goto DONE;
    RenderMenu (1);
    program = GetMenuSelection ();
    ClearBitmap (Bitmaps[ScreenSelect]);
    CloseMenuDisplay ();
    KillEventUtility ();
    oldbits = 0;
    upcount = 0;
    downcount = 0;
    EventUtilityInited = false;
    if (program < 0)
      goto DONE;
    if ((program >= 0) && (program < MENU_LINE_COUNT))
      LaunchProgram (program);
  }

DONE:
  CloseAFont ();
  return (0);
}

void
LaunchProgram (int32 program)
{
  Task *t;
  Item launchedItem;

  launchedItem = LoadProgram (ProgramLines[program]);
  if (launchedItem < 0)
    {
      kprintf ("Error launching %s: ", ProgramLines[program]);
      // PrintfSysErr( (uint32)launchedItem );
    }
  else
    {
      kprintf ("Launched %s as item 0x%lx\n", ProgramLines[program],
               launchedItem);
      while (1)
        {
          if ((t = (Task *)CheckItem (launchedItem, KERNELNODE, TASKNODE))
              == 0)
            break;
          if (t->t.n_Owner != KernelBase->kb_CurrentTask->t.n_Item)
            break;
          WaitSignal (SIGF_DEADTASK);
          // Wait( SIGF_DEADTASK );
        }
      kprintf ("Task 0x%lx has exited\n", launchedItem);
    }
}

long
ReadAFile (char *filename, char *buf, long count)
/* Reads count bytes from the filename file into
 * the specified buffer.  Returns the actual length of
 * the read,
 */
{
  Stream *stream;
  char name[256];

  strcpy (name, "^/");
  strcat (name, filename);

  stream = OpenDiskStream (name, 0);
  if (stream != NULL)
    {
      if ((count = ReadDiskStream (stream, buf, count)) < 0)
        Error2 ("Couldn't read file:", filename);
      CloseDiskStream (stream);
    }
  else
    {
      Error2 ("file doesn't exist:", filename);
      count = -1;
    }
  return (count);
}

char *
AllocAndLoadFile (char *filename, ulong memtype)
/* This handy little routine allocates a buffer that's large enough for
 * the named file, then loads that file into the buffer, and
 */
{
  long len;
  char *buf;
  Stream *stream;
  char name[256];

  strcpy (name, "^/");
  strcat (name, filename);

  buf = NULL;
  stream = OpenDiskStream (name, 0);
  if (stream != NULL)
    {
      len = stream->st_FileLength;
      AllocAndLoadLen = len;
      if (len > 0)
        {
          len = (len + 3) & ~3L;
          buf = (char *)ALLOCMEM ((int)len, memtype);
          if (buf)
            {
              if (ReadDiskStream (stream, buf, len) < 0)
                Error2 ("Couldn't read file:", filename);
            }
          else
            Error2 ("Out of memory loading file:", filename);
        }
      else
        Error2 ("file is empty:", filename);
      CloseDiskStream (stream);
    }
  else
    Error2 ("file doesn't exist:", filename);
  return (buf);
}

int32
Init ()
{
  char screenSelect;
  int32 retvalue;

  retvalue = -1;

  screenSelect = 0;

  VRAMIOReq = GetVRAMIOReq ();

  retvalue = OpenGraphicsFolio ();
  if (retvalue < 0)
    {
      printf ("Error:  OpenGraphicsFolio() returned %ld\n", retvalue);
      // PrintfSysErr( retvalue );
      goto DONE;
    }

  if (myOpenSPORT () < 0)
    {
      retvalue = -1;
      goto DONE;
    }

  InitFont ();

  retvalue = 0;

  ChangeInitialDirectory ("/CD-ROM", "/remote", false);

DONE:
  return (retvalue);
}

char *
GetAtom (char *atom, char *line)
{
  char terminator;

  if (*line == '"')
    {
      line++;
      terminator = '"';
    }
  else
    terminator = ' ';
  while ((*atom++ = *line++) != terminator)
    ;
  *(atom - 1) = '\0';
  if (terminator != ' ')
    line++;
  return (line);
}

int32
GetMenuSelection ()
{
  int32 newbits, newmenu;

  // while ( GetJoystick()
  //	& (JOYFIREA|JOYFIREB|JOYFIREC|JOYSTART) ) ;

  FOREVER
  {
    // newbits = GetJoystick();
    newbits = ReadControlPad (JOYCONTINUOUS);
    newmenu = 0;
    if (newbits & JOYFIREA)
      return (MenuSelect);
    if (newbits & JOYFIREB)
      return (MenuSelect);
    if (newbits & JOYFIREC)
      return (MenuSelect);
    if (newbits & JOYSTART)
      return (-1);
    if (newbits & JOYUP)
      {
        upcount++;
        if (((oldbits & JOYUP) == 0)
            || ((upcount > 7500) && ((upcount % 1500) == 0)))
          {
            if (MenuSelect > 0)
              {
                MenuSelect--;
                newmenu = 1;
              }
          }
      }
    else
      upcount = 0;
    if (newbits & JOYDOWN)
      {
        downcount++;
        if (((oldbits & JOYDOWN) == 0)
            || ((downcount > 7500) && ((downcount % 1500) == 0)))
          {
            if (MenuSelect < MenuLineCount - 1)
              {
                MenuSelect++;
                newmenu = 1;
              }
          }
      }
    else
      downcount = 0;
    if (newmenu)
      RenderMenu (0);
    oldbits = newbits;
  }
  return (-1);
}

int32
OpenMenuDisplay ()
{
  int32 retvalue;
  char *ptr;
  Screen *screen;
  int32 width, height, bigsize, i;
  int32 vramPageSize;

  retvalue = -1;

  width = SCREEN_WIDTH;
  height = SCREEN_HEIGHT;
  bigsize = width * height * 2;

  vramPageSize = GetPageSize (MEMTYPE_VRAM);
  ScreenPageCount = (bigsize + vramPageSize - 1) / vramPageSize;
  ScreenByteCount = ScreenPageCount * vramPageSize;

  /*??? need better type */
  ptr = (char *)ALLOCMEM (ScreenByteCount, MEMTYPE_VRAM);
  if (ptr == 0)
    {
      printf ("Error:  Couldn't allocate menu buffer\n");
      goto DONE;
    }

  BitmapBufPtr = (void *)ptr;

  ScreenGroupItem = CreateScreenGroup (ScreenItems, ScreenTags);
  if (ScreenGroupItem < 0)
    {
      printf ("Error:  CreateScreen() returned %ld\n", ScreenGroupItem);
      goto DONE;
    }
  AddScreenGroup (ScreenGroupItem, NULL);

  for (i = 0; i <= 0; i++)
    {
      screen = (Screen *)LookupItem (ScreenItems[i]);
      BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      Bitmaps[i] = screen->scr_TempBitmap;
      ClearBitmap (Bitmaps[i]);
    }

  retvalue = 0;

DONE:
  return (retvalue);
}

void
CloseMenuDisplay ()
{
  RemoveScreenGroup (ScreenGroupItem);
  FREEMEM (BitmapBufPtr, ScreenByteCount);
}

/* *************************************************************************
 * ***                                        ******************************
 * ***  Rendering and SPORT Support Routines  ******************************
 * ***                                        ******************************
 * *************************************************************************
 */

void
FillDisplay (GrafCon *gcon, long red, long green, long blue)
/* Fills the gcon display buffer by rendering a rectangle using the
 * RGB values specified by the args.  Expects the RGB values to
 * be in range (if not, they will overstrike one another's values).
 */
{
  WorkRect.rect_XLeft = 0;
  WorkRect.rect_XRight = Bitmaps[0]->bm_Width - 1;
  WorkRect.rect_YTop = 0;
  WorkRect.rect_YBottom = Bitmaps[0]->bm_Height - 1;
  SetFGPen (gcon, MakeRGB15 (red, green, blue));
  FillRect (BitmapItems[ScreenSelect], gcon, &WorkRect);
}

void
RandomFillDisplay (GrafCon *gcon, long maxred, long maxgreen, long maxblue)
/* Fills the gcon display buffer by rendering a rectangle using a Random()
 * color bounded by the maximum RGB values specified by the args.
 *
 * Pass an arg <= 0 to automatically get the full range for that
 * color component
 */
{
  ulong red, green, blue;

  if (maxred <= 0)
    maxred = MAX_RED;
  if (maxgreen <= 0)
    maxgreen = MAX_GREEN;
  if (maxblue <= 0)
    maxblue = MAX_BLUE;
  red = Random (maxred + 1);
  green = Random (maxgreen + 1);
  blue = Random (maxblue + 1);
  FillDisplay (gcon, red, green, blue);
}

int32
InitBackPic (char *filename)
/* Allocates the BackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the BackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
  int32 retvalue;
  ubyte *ptr, *ptr2, *ptr3;
  long i, j;
  int32 width;

  retvalue = -1;

  BackPic = (ubyte *)ALLOCMEM ((int)(ScreenByteCount),
                               MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

  if (NOT BackPic)
    {
      Warning ("unable to allocate BackPic");
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, BackPic, 0, ScreenPageCount, -1);

  width = Bitmaps[0]->bm_Width * 2;

  if (filename)
    {
      if (ReadAFile (filename, BackPic, ScreenByteCount) < 0)
        goto DONE;
      ptr = BackPic;
      ptr2 = Bitmaps[0]->bm_Buffer;
      ptr3 = ptr2 + 2;
      for (i = 0; i < DISPLAY_HEIGHT; i += 2)
        {
          for (j = 0; j < DISPLAY_WIDTH; j++)
            {
              *ptr3++ = *ptr++;
              *ptr3 = *ptr++;
              ptr3 += 3;
              *ptr2++ = *ptr++;
              *ptr2 = *ptr++;
              ptr2 += 3;
            }
        }
    }

  CopyVRAMPages (VRAMIOReq, BackPic, Bitmaps[0]->bm_Buffer, ScreenPageCount,
                 -1);

  retvalue = 0;

DONE:
  return (retvalue);
}

void
CopyBackPic (Bitmap *bitmap)
{
  if (BackPic)
    CopyVRAMPages (VRAMIOReq, bitmap->bm_Buffer, BackPic, ScreenPageCount, -1);
}

void
ClearBitmap (Bitmap *bitmap)
/* Uses flash write to clear to zero the bitmap */
{
  SetVRAMPages (VRAMIOReqItem, bitmap->bm_Buffer, 0, ScreenPageCount,
                0xFFFFFFFF);
}

void
ClearBitmaps (Bitmap *bitmap0, Bitmap *bitmap1)
/* Uses flash write to clear to zero the two bitmaps */
{
  ClearBitmap (bitmap0);
  ClearBitmap (bitmap1);
}

Item SPORTDevice;

int32
myOpenSPORT (void)
/* Open the SPORT device for blasting the background picture and doing
 * other SPORT operations
 *
 * NOTE:  this routine will be replaced when we lose the SPORT operations
 * when we switch to the new RAM architecture
 */
{
  register int32 i;
  int32 retvalue;
  TagArg targs[2];

  retvalue = -1;

  if ((SPORTDevice = OpenItem (
           FindNamedItem (MKNODEID (KERNELNODE, DEVICENODE), "SPORT"), 0))
      < 0)
    {
      Error ("SPORT open failed");
      goto DONE;
    }

  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)SPORTDevice;
  targs[1].ta_Tag = TAG_END;

  for (i = 1; i >= 0; i--)
    if ((mySPORTIO[i] = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), targs))
        < 0)
      {
        Error ("Can't create SPORT I/O");
        goto DONE;
      }

  retvalue = 0;

DONE:
  return (retvalue);
}

long
GetJoystick (void)
/* Gets the current state of the joystick switches.  The default is
 * to return the switches that have made a transition from off to on,
 * and any of the JOYCONTINUOUS switches that are currently on.
 *
 * NOTE:  routine doesn't debounce yet.  Maybe someone could hack in a
 * little debounce code for me?
 *
 * Something like this routine should end up somewhere in a library,
 * or perhaps system code.
 */
{
  static long oldjoy;
  long newjoy, returnjoy;

  /* Which joystick switches have changed since last test? */
  /*??? This routine perhaps shouldn't be reaching directly
   *??? into the graphics folio's GrafBase like this, and someday
   *??? this routine should reach to a more appropriate place
   *??? to get the state of the joystick switches.
   */
  /*???	newjoy = GrafBase->gf_JoySave;*/
  newjoy = ReadControlPad (JOYCONTINUOUS);
  RawNewJoy = newjoy;

  returnjoy = newjoy ^ oldjoy;

  /* The default is to return only positive transitions */
  returnjoy &= newjoy;

  /* Also return any of the continuous-signal switches that are
   * currently joy-depressed
   */
  returnjoy |= (newjoy & JOYCONTINUOUS);
  oldjoy = newjoy;
  return (returnjoy);
}

ulong
Random (ulong n)
/* Return a random number from 0 to n-1
 * The return value has 16 bits of significance, and ought to be unsigned.
 * Is the above true?
 */
{
  ulong i, j, k;

  i = (ulong)rand () << 1;
  j = i & 0x0000FFFF;
  k = i >> 16;
  return ((((j * n) >> 16) + k * n) >> 16);
}

void
InitFont ()
{
  ResetCurrentFont ();
  SystemFont = GetCurrentFont ();
  //	MenuFont.font_FontEntries = SystemFont->font_FontEntries;
  //	SetCurrentFont( &MenuFont );
  MenuFontCCB.ccb_SourcePtr = SystemFont->font_CCB->ccb_SourcePtr;
  //	OpenRAMFont(&MenuFont);
  SetCurrentFontCCB (&MenuFontCCB);
}

void
CloseAFont (void)
{
  ResetCurrentFont ();
}

void
RenderMenu (int32 fillrect)
{
  int32 i;
  GrafCon gcon;
  int32 height, top, width, left, right, bottom;
  char *ptr;
  int32 lineheight;

  if (MenuLineCount > 19)
    lineheight = 8;
  else if (MenuLineCount > 17)
    lineheight = 9;
  else if (MenuLineCount > 14)
    lineheight = 10;
  else
    lineheight = 11;

  height = MenuLineCount * lineheight + 4 - 6 + 6 + 4;
  top = (SCREEN_HEIGHT - height) / 2 + (((lineheight * 2) + 4) / 2);
  width = MaxCharacterCount * 8 + 4 - 2 + 8 + 4 + 8;
  left = (SCREEN_WIDTH - width) / 2;
  right = left + width - 1;
  bottom = top + height - 1;

  if (fillrect)
    {
      SetFGPen (&gcon, 0x2C02);
      WorkRect.rect_XLeft = left;
      WorkRect.rect_XRight = right;
      WorkRect.rect_YTop = top;
      WorkRect.rect_YBottom = bottom;
      FillRect (BitmapItems[ScreenSelect], &gcon, &WorkRect);

      SetFGPen (&gcon, 0x6318);
      MoveTo (&gcon, left + 3, top + 2);
      DrawTo (BitmapItems[ScreenSelect], &gcon, right - 3, top + 2);
      DrawTo (BitmapItems[ScreenSelect], &gcon, right - 3, bottom - 2);
      DrawTo (BitmapItems[ScreenSelect], &gcon, left + 3, bottom - 2);
      DrawTo (BitmapItems[ScreenSelect], &gcon, left + 3, top + 2);

      myFontPLUT[0] = 0x00006318;
      ptr = "Select a 3DO Application";
      i = strlen (ptr) * 8;
      gcon.gc_PenX = (SCREEN_WIDTH - i) / 2;
      gcon.gc_PenY = top - (lineheight * 2) - 4;
      DrawText8 (&gcon, BitmapItems[ScreenSelect], ptr);

      ptr = "Press Any Button to Begin";
      i = strlen (ptr) * 8;
      gcon.gc_PenX = (SCREEN_WIDTH - i) / 2;
      gcon.gc_PenY = top - lineheight - 4;
      // gcon.gc_PenY = top - (lineheight * 2) - 4;
      DrawText8 (&gcon, BitmapItems[ScreenSelect], ptr);

      /*ptr = "Press Start to exit this program";
      i = strlen ( ptr ) * 8;
      gcon.gc_PenX = (SCREEN_WIDTH - i) / 2;
      gcon.gc_PenY = top - lineheight - 4;
      DrawText8( &gcon, BitmapItems[ScreenSelect], ptr );*/
    }
  for (i = 0; i < MenuLineCount; i++)
    {
      SetFGPen (&gcon, 0x2C02);
      WorkRect.rect_XLeft = left + 2 + 2 + 4;
      ;
      WorkRect.rect_XRight = right - 4;
      WorkRect.rect_YTop = top + 2 + 2 + lineheight * i + 2;
      WorkRect.rect_YBottom = WorkRect.rect_YTop + 8;
      FillRect (BitmapItems[ScreenSelect], &gcon, &WorkRect);
      gcon.gc_PenX = left + 2 + 2 + 4;
      gcon.gc_PenY = top + 2 + 2 + lineheight * i + 2;
      if (i != MenuSelect)
        gcon.gc_PenX += 8;
      //		if ( i == MenuSelect ) myFontPLUT[0] =  0x18C66318;
      //		else myFontPLUT[0] = 0x2C026318;
      DrawText8 (&gcon, BitmapItems[ScreenSelect], MenuAtoms[i]);
    }
  DisplayScreen (ScreenItems[ScreenSelect], 0);
  //	ScreenSelect = 1 - ScreenSelect;
}

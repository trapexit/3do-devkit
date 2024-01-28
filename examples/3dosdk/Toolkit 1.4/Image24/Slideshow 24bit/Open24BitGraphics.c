/*******************************************************************************************
 *	File:			Open24BitGraphics.c
 *
 *	Contains:		Screen init for 24-bit color.
 *
 *	Written by:		Stephen Landrum, George Mitsuoka, Neil Day,
 *Steve Hayes and friends.
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	4/29/94			jhm		Commented out #define
 *CEL_OVER_VIDEO, so it won't come on even #if DEBUG.
 *  3/2/94			jhm		Added "#define NFW 0" so the
 *compiler won't generate warnings while regenerating make file dependencies.
 *	2/24/94			bck		Fixed text overlay for 24 bit
 *display. 2/24/94			seh		Add text overlay.
 *	2/20/94			seh		Double buffering.
 *	2/14/94			nmd		Added 24 bit frame buffer
 *support for single buffer
 *	?/?/??			gm		created from Landrum's example
 *
 *******************************************************************************************/

#include "Form3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "graphics.h" /* for CCB */
#include "stdlib.h"

//#include "world.h"
#include "Open24BitGraphics.h"

#define MAXVDLS 2
#define NVDLS 2
#define NBUFFERS (NVDLS * 2)

#define prt(x) printf x
#define catch_nil(v, x)                                                       \
  if (!v)                                                                     \
    {                                                                         \
      prt (x);                                                                \
      goto cleanup;                                                           \
    }
#define catch_neg(v, x)                                                       \
  if (v < 0)                                                                  \
    {                                                                         \
      prt (x);                                                                \
      PrintfSysErr ((Item)v);                                                 \
      goto cleanup;                                                           \
    }
#define dbg(x) prt (x)

///************************************************************************************
///	GLOBALS
///************************************************************************************

/* Displaying a screen within a task is a three level process
        1) create a screen group
        2) add the screen groups to the graphics folios
        3) display a screen from a group
*/

#if 0
//Item 	ScreenItems[NBUFFERS];
Item	ScreenGroupItem;
//Item	BitmapItems[NBUFFERS];
//Bitmap	*Bitmaps[NBUFFERS];
#endif
/* Stuff we need 'cause this is not well thought out */
uint32 bmw[NBUFFERS] = { 320, 320, 320, 320 }; // ,320,320};
uint32 bmh[NBUFFERS] = { 240, 240, 240, 240 }; // ,240,240};
ubyte *bmbufs[NBUFFERS];
ubyte *bmbuffer[NVDLS];

int32 *vdlarray[NBUFFERS];
int32 *vdl[NVDLS];

TagArg taScreenTags[] = {
  CSG_TAG_SPORTBITS,
  (void *)0,
  //			  CSG_TAG_VDLPTR_ARRAY,	(void*)0,
  CSG_TAG_DISPLAYHEIGHT,
  (void *)240,
  CSG_TAG_SCREENCOUNT,
  (void *)NBUFFERS,
  CSG_TAG_SCREENHEIGHT,
  (void *)240,
  CSG_TAG_BITMAPCOUNT,
  (void *)1,
  CSG_TAG_BITMAPWIDTH_ARRAY,
  (void *)bmw,
  CSG_TAG_BITMAPHEIGHT_ARRAY,
  (void *)bmh,
  CSG_TAG_BITMAPBUF_ARRAY,
  (void *)bmbufs,
  CSG_TAG_DONE,
  (void *)0,
};

///************************************************************************************
///	PROTOTYPES
///************************************************************************************

void initVDL (int32 *vdl, int32 bitmapindex);

/*************************************************************************************
        OpenGL

        initialize and
*************************************************************************************/
// uint32 OpenGL(char **buffer)
int32
Open24BitGraphics (ScreenContext *sc, int nScreens)
{
  long i, v;
  int32 ret;
  Item ScreenGroupItem;
  int32 *tmpvdl;
  Item vramioreq;

  sc->sc_nScreens = nScreens;

  if (nScreens != NVDLS)
    printf ("ERROR: I only know how to open two 24bit screens!\n");

  if (OpenGraphicsFolio ())
    {
      DIAGNOSTIC ("Cannot open graphics folio");
      return FALSE;
    }

  taScreenTags[0].ta_Arg = (void *)GETBANKBITS (GrafBase->gf_ZeroPage);

  for (v = 0; v < NVDLS; v++)
    {
      vdl[v] = (int32 *)AllocMem (6 * 40 * 4, MEMTYPE_DMA | MEMTYPE_VRAM
                                                  | MEMTYPE_STARTPAGE);
      catch_nil (vdl[v], ("failed to allocate vdl %d\n", v));
      vdlarray[v * 2] = vdl[v];
      vdlarray[(v * 2) + 1] = vdl[v] + 120;
    }

  sc->sc_nFrameBufferPages
      = (bmw[0] * bmh[0] * 4 + GrafBase->gf_VRAMPageSize - 1)
        / GrafBase->gf_VRAMPageSize;

  for (v = 0; v < NVDLS; v++)
    {
      bmbuffer[v] = (ubyte *)ALLOCMEM (
          (int)(sc->sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize),
          MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);
      catch_nil (bmbuffer[v], ("failed to allocate screen memory\n"));
      bmbufs[v * 2] = bmbuffer[v];
      bmbufs[(v * 2) + 1] = bmbuffer[v];
    }

  //	taScreenTags[1].ta_Arg = (void *)vdlarray;

  ScreenGroupItem = CreateScreenGroup (&(sc->sc_Screens[0]), taScreenTags);
  catch_neg (ScreenGroupItem, ("Failed to CreateScreenGroup\n"));

  AddScreenGroup (ScreenGroupItem, NULL);

  for (i = 0; i < NBUFFERS; i++)
    {
      Screen *screen;

      screen = (Screen *)LookupItem (sc->sc_Screens[i]);
      catch_nil (screen, ("Failed to Lookup ScreenItem\n"));
      sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
    }

  for (v = 0; v < NVDLS; v++)
    {
      tmpvdl = vdl[v];
      initVDL (tmpvdl, v * 2);
      initVDL (tmpvdl + 120, (v * 2) + 1);
      tmpvdl[83] = (int32)(tmpvdl + 120);
      tmpvdl[123] = (int32)GrafBase->gf_VDLPostDisplay;

      i = SubmitVDL ((VDLEntry *)tmpvdl, 30 * 4, VDLTYPE_FULL);
      if (i < 0)
        {
          dbg (("Error in SubmitVDL  (%lx)\n", i));
          return (-1);
        }
      SetVDL (sc->sc_Screens[v * 2], i);

      i = SubmitVDL ((VDLEntry *)tmpvdl + 30 * 4, 30 * 4, VDLTYPE_FULL);
      if (i < 0)
        {
          dbg (("Error in SubmitVDL  (%lx)\n", i));
          return (-1);
        }
      SetVDL (sc->sc_Screens[(v * 2) + 1], i);
    }

  vramioreq = GetVRAMIOReq ();

  SetVRAMPages (vramioreq, sc->sc_Bitmaps[0]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, ~0L);
  SetVRAMPages (vramioreq, sc->sc_Bitmaps[2]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, ~0L);

  // TBD don't I have to free the vramioreq?

  ret = DisplayScreen (sc->sc_Screens[0], sc->sc_Screens[1]);
  catch_neg (ret, ("displayscreen failed\n"));

  for (v = 0; v < NVDLS; v++)
    {
      prt (("VideoBuffer[%d] $%x\n", v,
            (uint32)sc->sc_Bitmaps[v * 2]->bm_Buffer));
    }

  sc->sc_nFrameByteCount
      = sc->sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize;
  sc->sc_curScreen = 0;

  return ScreenGroupItem;
cleanup:
  return -1;
}

/*************************************************************************************
        initVDL
*************************************************************************************/

void
initVDL (int32 *vdl, int32 bitmapindex)
{
  int32 j;

  vdl[0] = VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV | (34 << VDL_LEN_SHIFT)
           | VDL_PREVSEL | (240 << VDL_LINE_SHIFT) | VDL_DISPMOD_320
           | VDL_480RES;
  vdl[1] = ((uint32)bmbufs[bitmapindex]);
  vdl[2] = ((uint32)bmbufs[bitmapindex]) + 2;
  vdl[3] = (int32)(vdl + 40);

  for (j = 0; j < 8; j++)
    {
      vdl[4 + j] = (j << 24) | (j * 2 * 0x00010101);
    }
  for (j = 8; j < 16; j++)
    {
      vdl[4 + j] = (j << 24) | ((j + j + 225) * 0x00010101);
    }
  for (j = 16; j < 32; j++)
    {
      vdl[4 + j] = (j << 24) | (((31 - j) * 16) * 0x00010101);
    }
  vdl[4 + 32] = VDL_DISPCTRL | VDL_BACKGROUND | 0;
  vdl[4 + 33] = VDL_DISPCTRL | VDL_VINTEN | VDL_HINTEN | VDL_HSUB_ZERO
                | VDL_BLSB_BLUE
                | (int32)((bitmapindex & 1) ? VDL_VSUB_ONE : VDL_VSUB_ZERO);
  for (j = 4 + 34; j < 40; j++)
    {
      vdl[j] = VDL_NULLVDL;
    }

  vdl[40] = (34 << VDL_LEN_SHIFT) | (0 << VDL_LINE_SHIFT) | VDL_DISPMOD_320
            | VDL_480RES;
  vdl[41] = ((uint32)bmbufs[bitmapindex]);
  vdl[42] = ((uint32)bmbufs[bitmapindex]) + 2;
  vdl[43] = (int32)(vdl + 80);
  for (j = 0; j < 8; j++)
    {
      vdl[44 + j] = (j << 24) | (j * 2 * 0x00010101);
    }
  for (j = 8; j < 16; j++)
    {
      vdl[44 + j] = (j << 24) | ((j + j + 225) * 0x00010101);
    }
  for (j = 16; j < 32; j++)
    {
      vdl[44 + j] = (j << 24) | (((31 - j) * 16) * 0x00010101);
    }
  vdl[44 + 32] = VDL_DISPCTRL | VDL_BACKGROUND | 0;
  vdl[44 + 33] = VDL_DISPCTRL | VDL_VINTEN | VDL_HINTEN | VDL_HSUB_ZERO
                 | VDL_BLSB_BLUE
                 | (int32)((bitmapindex & 1) ? VDL_VSUB_ONE : VDL_VSUB_ZERO);
  for (j = 44 + 34; j < 40 + 40; j++)
    {
      vdl[j] = VDL_NULLVDL;
    }

  vdl[80] = (34 << VDL_LEN_SHIFT) | (17 << VDL_LINE_SHIFT) | VDL_DISPMOD_320
            | VDL_480RES;
  vdl[81] = ((uint32)bmbufs[bitmapindex]);
  vdl[82] = ((uint32)bmbufs[bitmapindex]) + 2;
  vdl[83] = (int32)GrafBase->gf_VDLPostDisplay;
  for (j = 0; j < 8; j++)
    {
      vdl[84 + j] = (j << 24) | (j * 2 * 0x00010101);
    }
  for (j = 8; j < 16; j++)
    {
      vdl[84 + j] = (j << 24) | ((j + j + 225) * 0x00010101);
    }
  for (j = 16; j < 32; j++)
    {
      vdl[84 + j] = (j << 24) | (((31 - j) * 16) * 0x00010101);
    }
  vdl[84 + 32] = VDL_DISPCTRL | VDL_BACKGROUND | 0;
  vdl[84 + 33] = VDL_DISPCTRL | VDL_VINTEN | VDL_HINTEN | VDL_HSUB_ZERO
                 | VDL_BLSB_BLUE
                 | (int32)((bitmapindex & 1) ? VDL_VSUB_ONE : VDL_VSUB_ZERO);
  for (j = 84 + 34; j < 80 + 40; j++)
    {
      vdl[j] = VDL_NULLVDL;
    }
}

#define NFW 0 // [TBD] SHayes?
#if NFW

/*************************************************************************************
        GLGetBitmapItem
*************************************************************************************/
Item
GLGetBitmapItem (char *buffer)
{
  int32 v;

  for (v = NVDLS - 1; v >= 0; v--)
    {
      if (buffer == sc->sc_Bitmaps[v * 2]->bm_Buffer)
        return (sc->sc_BitmapItems[v * 2]);
    }

  return (-1);
}

static int32 curindex = 0;

/*************************************************************************************
        GLDisplayDoubleBuffer
*************************************************************************************/
void
GLDisplayDoubleBuffer (int32 index)
{
  int32 ret;

  curindex = index;
  if (index < NBUFFERS)
    {
      ret = DisplayScreen (sc->sc_Screens[index * 2],
                           sc->sc_Screens[index * 2 + 1]);
      catch_neg (ret, ("displayscreen %d failed\n", index));
    }

cleanup:
  return;
}
#endif

#if DEBUG
// static  GrafCon gc;

// #define CEL_OVER_VIDEO
#ifdef CEL_OVER_VIDEO
CCB *ccb = NULL;
static Item bitmapItem;
GrafCon grafCon;
#define LockedPrintf printf
#endif

/*************************************************************************************
        GLDisplayText
*************************************************************************************/

void
GLDisplayText (ScreenContext *sc, char *s, long num, int y)
{
  char outstr[256];

#ifdef CEL_OVER_VIDEO
  if (ccb == NULL)
    {
      Point quad[4];
      int32 width;
      int32 height;
      int32 byteCount;

      width = 160;
      height = 12;
      byteCount = width * height * 4;

      /* Allocate a block the size of a CCB */
      if ((ccb = (CCB *)ALLOCMEM (sizeof (CCB),
                                  MEMTYPE_CEL | (MEMTYPE_FILL | 0x00)))
          == NULL)
        {
          LockedPrintf (
              ("MoviePlayer>CreatePerformer: Cannot allocate CCB memory.\n"));
          return;
        }

      /* Allocate a block the size of the celData */
      if ((ccb->ccb_SourcePtr = (CelData *)ALLOCMEM (
               byteCount, MEMTYPE_CEL | (MEMTYPE_FILL | 0x00)))
          == NULL)
        {
          LockedPrintf (("MoviePlayer>CreatePerformer: Cannot allocate CCB "
                         "source memory.\n"));
          return;
        }
      memset (ccb->ccb_SourcePtr, 0x00, byteCount);

      ccb->ccb_Width = width;
      ccb->ccb_Height = height;
      ccb->ccb_Flags = CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_LDSIZE
                       | CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE | /* CCB_BGND | */
                       CCB_YOXY | CCB_ACW | CCB_ACCW;
      ccb->ccb_PRE0
          = ((height / 2 - 1) << PRE0_VCNT_SHIFT)
            +             /* number of vertical data lines in sprite data -1 */
            PRE0_LINEAR + /* use PIN for IPN (0x10) */
            PRE0_BPP_16;  /* set bits/pixel to 16 (0x6) see HDWR spec 3.6.3 */

      ccb->ccb_PRE1
          = (((width * 4 / 4) - 2) << PRE1_WOFFSET10_SHIFT)
                + /* offset (in words) from line start to next line start. */
                (width - 1)
            | PRE1_LRFORM; /* number of horizontal pixels to render -1 */
      ccb->ccb_PIXC = 0x1F001F00;

      quad[0].pt_X = 0;
      quad[0].pt_Y = 0;
      quad[1].pt_X = quad[0].pt_X + width;
      quad[1].pt_Y = quad[0].pt_Y;
      quad[2].pt_X = quad[0].pt_X + width;
      quad[2].pt_Y = quad[0].pt_Y + height;
      quad[3].pt_X = quad[0].pt_X;
      quad[3].pt_Y = quad[0].pt_Y + height;

      MapCel (ccb, quad);
      ccb->ccb_VDY *= 2;
      {
        TagArg tagArg[4];

        memset (&grafCon, 0, sizeof (GrafCon));

        SetFGPen (&grafCon, MakeRGB15 (16, 20, 32));
        SetBGPen (&grafCon, MakeRGB15 (0, 0, 0));

        tagArg[0].ta_Tag = CBM_TAG_WIDTH;
        tagArg[0].ta_Arg = (void *)width;
        tagArg[1].ta_Tag = CBM_TAG_HEIGHT;
        tagArg[1].ta_Arg = (void *)height;
        tagArg[2].ta_Tag = CBM_TAG_BUFFER;
        tagArg[2].ta_Arg = (void *)ccb->ccb_SourcePtr;
        tagArg[3].ta_Tag = CBM_TAG_DONE;

        bitmapItem = CreateBitmap (tagArg);
        if (bitmapItem < 0)
          PrintfSysErr (bitmapItem);
      }
    }
  sprintf (outstr, "%s %d          ", s,
           num); /* NOTE: the terminating spaces
                  * will clear out the rest of the cel, otherwise we get extra
                  * digits on the display, and the number looks huge! */
  MoveTo (&grafCon, 0, 0);
  DrawText8 (&grafCon, bitmapItem, outstr);
  ccb->ccb_XPos = 48 << 15;
  ccb->ccb_YPos = (long)(y * 2) << 15;
  DrawCels (sc->sc_BitmapItems[sc->sc_curScreen * 2], ccb);
//	DrawCels(sc->sc_BitmapItems[sc->sc_curScreen*2 + 1], ccb);
#else
  MoveTo (&gc, 32, y);
  sprintf (outstr, "%s %d", s, num);
  DrawText8 (&gc, sc->sc_BitmapItems[sc->sc_curScreen * 2], outstr);
#endif // #ifdef CEL_OVER_VIDEO
}
#endif // #if DEBUG

#if NFW
/*************************************************************************************
        GLGetIndBitMapItem
*************************************************************************************/

Item
GLGetIndBitMapItem (int32 ix)
{
  return sc->sc_BitmapItems[ix * 2];
}
#endif

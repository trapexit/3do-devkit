/* $Id: pd_faders.c,v 1.14 1994/05/05 22:48:16 peabody Exp $ */
/****************************************************************
**
**  patchdemo.arm gui module.
**
**  By:  Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
*****************************************************************
**  940223 WJB  Created from patchtest.c.
**  940224 WJB  Added New/Delete/ShowPatchPage().
**  940225 WJB  Moved DrawFaderBlock() to faders.c.
**  940225 WJB  Added New/DeletePatchPageList().
**  940225 WJB  Added trap for empty FaderBlock in NewPatchPageList().
**  940228 WJB  Added myalloc(), myfree().
**  940228 WJB  Added PAGE_IncludeEmptyPages.
**  940228 WJB  Removed nfaders==0 safety code - now depends on
*InitFaderBlock() to do the right thing.
**  940301 WJB  Renamed ppage_List to ppage_PageList.
**  940304 WJB  Fixed includes.
**  940408 WJB  Updated for PatchSymbol usage.
**  940422 WJB  Converted C++ comments to C comments.
**  940505 WJB  Removed newline character from page text - rendered junk.
****************************************************************/

/* local */
#include "patchdemo.h"

/* audiodemo */
#include "audiodemo.h" /* macros */
#include "faders.h"
#include "graphic_tools.h" /* rendering */

/* portfolio */
#include <stdio.h> /* sprintf() */

/* -------------------- Debugging */

#define DEBUG_Entry 0  /* entry/exit */
#define DEBUG_Events 0 /* FaderHandler() */

/* -------------------- Conditional */

#define PAGE_IncludeEmptyPages                                                \
  0 /* when enabled, includes empty pages in the patch display */

/* -------------------- Local functions */

static int32 FaderHandler (int32 ifader, int32 newvalue, FaderBlock *);

/* -------------------- New/DeletePatchPageList() */

/*
    Create PatchPageList for Patch.

    Can return w/ an empty List.
    Currently doesn't add pages w/o any faders.  Thus can return fewer pages
   than instruments.
*/
PatchPageList *
NewPatchPageList (Patch *patch, Err *errbuf)
{
  PatchPageList *pagelist = NULL;
  Err errcode = 0;

#if DEBUG_Entry
  printf ("NewPatchPageList() patch=$%p ", patch);
  printavail ();
  printf ("\n");
#endif

  /* Allocate and initialize PatchPageList */
  if ((pagelist = (PatchPageList *)myalloc (sizeof *pagelist)) == NULL)
    {
      errcode = -1; /* !!! real error code */
      goto clean;
    }
  pagelist->ppage_Patch = patch;
  InitList (&pagelist->ppage_PageList, "Instrument Pages");

  /* Create a PatchPage for every PatchInstrument in Patch, and add them to
   * PatchPageList */
  {
    PatchInstrument *inst;

    for (inst = (PatchInstrument *)FirstNode (&patch->patch_InstrumentList);
         IsNode (&patch->patch_InstrumentList, inst);
         inst = (PatchInstrument *)NextNode (inst))
      {
        PatchPage *page;

        if ((page = NewPatchPage (inst, &errcode)) == NULL)
          goto clean;

          /* add page to pagelist if there are some knobs on it, otherwise,
             delete page (depending on PAGE_IncludeEmptyPages setting) */
#if !PAGE_IncludeEmptyPages
        if (!page->ppage_FaderBlock.fdbl_NumFaders)
          DeletePatchPage (page);
        else
#endif
          AddTail (&pagelist->ppage_PageList, (Node *)page);
      }
  }

clean:
  if (errcode < 0)
    {
      CloseRes (DeletePatchPageList, pagelist);
      if (errbuf)
        *errbuf = errcode;
    }

#if DEBUG_Entry
  printf ("  pagelist=$%p err=%ld ", pagelist, errcode);
  printavail ();
  printf ("\n");
#endif

  return pagelist;
}

/*
    Delete PatchPageList created by NewPatchPageList()
*/
Err
DeletePatchPageList (PatchPageList *pagelist)
{
#if DEBUG_Entry
  printf ("DeletePatchPageList() pagelist=$%p ", pagelist);
  printavail ();
  printf ("\n");
#endif

  if (pagelist)
    {

      /* delete attached PatchPages */
      {
        struct PatchPage *page;

        while ((page = (PatchPage *)RemTail (&pagelist->ppage_PageList))
               != NULL)
          DeletePatchPage (page);
      }

      /* free PatchPageList */
      myfree (pagelist);
    }

#if DEBUG_Entry
  printf ("  ");
  printavail ();
  printf ("\n");
#endif

  return 0;
}

/* -------------------- New/DeletePatchPage() */

/*
    Create a new PatchPage for PatchInstrument.

    Sets fdr_UserItem of each Fader to the Knob item # of the associated knob.
*/
PatchPage *
NewPatchPage (PatchInstrument *inst, Err *errbuf)
{
  PatchPage *page = NULL;
  Err errcode = 0;

#if DEBUG_Entry
  printf ("NewPatchPage() inst=\"%s\" ", inst->pinst_Symbol.psym_Node.n_Name);
  printavail ();
  printf ("\n");
#endif

  /* Allocate and initialize PatchPage and Fader array */
  {
    const int32 nfaders = GetNGrabbedPatchKnobs (inst);

#if DEBUG_Entry
    printf ("  nfaders=%ld\n", nfaders);
#endif
    if ((page = (PatchPage *)myalloc (sizeof *page + nfaders * sizeof (Fader)))
        == NULL)
      {
        errcode = -1; /* !!! real error code */
        goto clean;
      }
    page->ppage_Node.n_Size = sizeof *page;
    page->ppage_Instrument = inst;
    InitFaderBlock (&page->ppage_FaderBlock, (Fader *)(page + 1), nfaders,
                    FaderHandler);
  }

  /* Bind faders to knobs */
  {
    PatchKnob *knob = inst->pinst_KnobTable;
    int32 nknobs = inst->pinst_NKnobs;
    Fader *fader = page->ppage_FaderBlock.fdbl_Faders;

    for (; nknobs--; knob++)
      if (IsPatchKnobGrabbed (knob))
        {
          TagArg getknobtags[] = { /* @@@ order is assumed below */
                                   { AF_TAG_MIN },
                                   { AF_TAG_MAX },
                                   { AF_TAG_CURRENT },
                                   { TAG_END }
          };

          /* get knob attributes */
          if ((errcode = GetAudioItemInfo (knob->pknob_Knob, getknobtags)) < 0)
            goto clean;

          /* fill out Fader from PatchKnob */
          fader->fdr_Name = knob->pknob_Name;
          fader->fdr_UserItem = knob->pknob_Knob;
          fader->fdr_VMin = (int32)getknobtags[0].ta_Arg;
          fader->fdr_VMax = (int32)getknobtags[1].ta_Arg;
          fader->fdr_Value = (int32)getknobtags[2].ta_Arg;
          fader->fdr_Increment
              = MAX (uceil (fader->fdr_VMax - fader->fdr_VMin, 100), 1);

          /* recompute fader position (!!! inherited from ta_faders.c - do this
           * more gracefully) */
          fader->fdr_YMin
              = FADER_YMIN
                + ((int32)(fader - page->ppage_FaderBlock.fdbl_Faders)
                   * FADER_SPACING)
                + 15;
          fader->fdr_YMax = fader->fdr_YMin + FADER_HEIGHT;

          /* increment fader pointer */
          fader++;
        }
  }

clean:
  if (errcode < 0)
    {
      CloseRes (DeletePatchPage, page);
      if (errbuf)
        *errbuf = errcode;
    }

#if DEBUG_Entry
  printf ("  page=$%p err=%ld ", page, errcode);
  printavail ();
  printf ("\n");
#endif

  return page;
}

/*
    Delete PatchPage created by NewPatchPage().
*/
Err
DeletePatchPage (PatchPage *page)
{
#if DEBUG_Entry
  printf ("DeletePatchPage() page=$%p ", page);
  printavail ();
  printf ("\n");
#endif

  myfree (page);

#if DEBUG_Entry
  printf ("  ");
  printavail ();
  printf ("\n");
#endif

  return 0;
}

/* -------------------- ShowPatchPage() */

/*
    Display PatchPage on screen.
*/
Err
ShowPatchPage (PatchPage *page)
{
  int32 nscreens = NumScreens;
  char b[64];

#if DEBUG_Entry
  printf ("ShowPatchPage() page=$%p inst=\"%s\"\n", page,
          page->ppage_Instrument->pinst_Symbol.psym_Node.n_Name);
  printf ("  NumScreens=%ld\n", NumScreens);
#endif

  /* draw everything in each screen buffer */
  while (nscreens--)
    {

      /* clear screen */
      ClearScreen ();

      /* draw text (!!! make this better) */
      MoveTo (&GCon[0], 20, TOP_VISIBLE_EDGE + 5);
      DrawText8 (&GCon[0], CURBITMAPITEM, "PatchDemo");

      sprintf (b, "%s %s",
               page->ppage_Instrument->pinst_Symbol.psym_Node.n_Name,
               page->ppage_Instrument->pinst_TemplateName);
      MoveTo (&GCon[0], 20, TOP_VISIBLE_EDGE + 20);
      DrawText8 (&GCon[0], CURBITMAPITEM, b);

      /* !!! more */

      /* draw FaderBlock */
      DrawFaderBlock (&page->ppage_FaderBlock);

      SwitchScreens ();
    }

#if DEBUG_Entry
  printf ("  ScreenSelect=%ld\n", ScreenSelect);
#endif

  return DisplayScreen (ScreenItems[ScreenSelect],
                        0); /* !!! should SwitchScreens() even if there's no
                               real switch to be done? */
}

/* -------------------- FaderEventHandler() */

/*
    Tweak knob associated with Fader on value changes.
*/
static int32
FaderHandler (int32 ifader, int32 newvalue, FaderBlock *fdbl)
{
  const Fader *fader = &fdbl->fdbl_Faders[ifader];

#if DEBUG_Events
  printf ("tweak \"%s\" %ld\n", fader->fdr_Name, newvalue);
#endif

  return TweakRawKnob (fader->fdr_UserItem, newvalue);
}

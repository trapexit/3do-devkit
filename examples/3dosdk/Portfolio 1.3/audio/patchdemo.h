/* $Id: patchdemo.h,v 1.13 1994/04/22 23:04:41 peabody Exp $ */
/****************************************************************
**
**  patchdemo.arm central include file.
**
**  By:  Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
*****************************************************************
**  940223 WJB  Created from patchtest.c.
**  940224 WJB  Added PatchPage.
**  940224 WJB  Added FindPatchKnob(), GetNGrabbedPatchKnobs().
**  940225 WJB  Added PatchPageList.  Changed comment hierarchy a bit.
**  940228 WJB  Added myalloc(), myfree().
**  940228 WJB  Moved some macros to audiodemo.h.
**  940301 WJB  Renamed ppage_List to ppage_PageList.
**  940301 WJB  Added PatchSample and PatchSampleAttachment.
**  940311 WJB  Converted non-triple-bang C++ comments to C comments.
**  940407 WJB  Made a bunch of functions private to pd_patch.c
**  940407 WJB  Added PatchSymbol.
**  940414 WJB  Added note about delay line usage of PatchSample.
**  940422 WJB  Added pinst_AttrFlags and pinst_StateFlags and related defines
*to PatchInstrument.
****************************************************************/

#pragma include_only_once
#ifndef __PATCHDEMO_H
#define __PATCHDEMO_H

/* audiodemo */
#include "faders.h" /* FaderBlock */

/* portfolio */
#include <audio.h>
#include <list.h>
#include <mem.h> /* availMem() */
#include <nodes.h>
#include <stdlib.h> /* calloc(), free() */
#include <types.h>

/* -------------------- Patch structures */

/* PatchSymbol (base structure for named structures (e.g. PatchInstrument)) */
/*
    Symbol names are case insensitive and are unique within a given symbol
   list.
*/
struct PatchSymbol;
typedef struct PatchSymbol PatchSymbol;
typedef Err (*PatchSymbolDestructor) (PatchSymbol *symbol);

typedef struct PatchSymbolFunctions
{
  PatchSymbolDestructor
      psymfn_Destructor; /* destructor can assume that symbol is non-NULL */
} PatchSymbolFunctions;

struct PatchSymbol
{
  Node psym_Node; /* Link in symbol list.  n_Name is symbol name. */
  const PatchSymbolFunctions *psym_Functions;
};

/* PatchKnob (knob tracker for PatchInstrument) */
typedef struct PatchKnob
{
  char *pknob_Name; /* name of knob */
  Item pknob_Knob;  /* Item # of grabbed knob */
} PatchKnob;

#define IsPatchKnobGrabbed(knob) ((knob)->pknob_Knob != 0)

/* PatchInstrument (member of patch_InstrumentList in a Patch) */
typedef struct PatchInstrument
{
  PatchSymbol
      pinst_Symbol; /* Link in patch_InstrumentList symbol name space. */
  /* @@@ make a List in the base class for PatchInstrument and Patch? */

  /* Instrument */
  char *pinst_TemplateName; /* Instrument template file name. */
  Item pinst_Instrument;    /* Item # of audiofolio instrument. */
  uint8 pinst_AttrFlags;    /* Constant attribute flags (PIATTRF_) */
  uint8 pinst_StateFlags;   /* Variable state flags (PISTATEF_) */

  /* Knobs */
  PatchKnob *pinst_KnobTable; /* PatchKnob pinst_KnobTable[pinst_NKnobs].  NULL
                                 if pinst_NKnobs is 0. */
  int32 pinst_NKnobs;

} PatchInstrument;

/* pinst_AttrFlags */
#define PIATTRF_Triggerable                                                   \
  0x01 /* Instrument can be restarted w/ successive calls to                  \
          StartPatchInstrument() and can be released w/                                                       \
          ReleasePatchInstrument().  Without this flag set, the instrument is                                                       \
          started once by StartPatchInstrument() and ignores subsequent calls                                                     \
          to StartPatchInstrument() or ReleasePatchInstrument() until stopped                                       \
          with StopPatchInstrument(). */

/* pinst_StateFlags */
#define PISTATEF_Started                                                      \
  0x01 /* Instrument has been started.  Set by StartPatchInstrument(),        \
          cleared by StopPatchInstrument(). */

int32 GetNGrabbedPatchKnobs (const PatchInstrument *inst);

/* PatchSample (member of patch_SampleList in a Patch) */
typedef struct PatchSample
{
  PatchSymbol psamp_Symbol; /* Link in patch_SampleList symbol name space. */

  /* Sample */
  char *psamp_FileName; /* Sample file name.  NULL for delay line. */
  Item psamp_Sample;    /* Item # of audiofolio sample or delay line. */
} PatchSample;

/* PatchSampleAttachment (member of patch_SampleAttachmentList in a Patch) */
typedef struct PatchSampleAttachment
{
  PatchSymbol
      psatt_Symbol; /* Link in patch_SampleAttachmentList symbol name space. */
  Item psatt_Attachment; /* Item # of audiofolio sample attachment. */
} PatchSampleAttachment;

/* Patch */
typedef struct Patch
{
#if 0 /* @@@ future */
    PatchInstrument patch_Instrument;   /* Patch is a derived class of PatchInstrument */
#endif

  /* symbol name spaces */
  List patch_InstrumentList; /* Instrument symbols - List of PatchInstrument's
                              */
                             /* @@@ move to base class List/Node structure? */
  List patch_SampleList;     /* Sample symbols - List of PatchSample's */
  List patch_SampleAttachmentList; /* Sample attachment symbols - List of
                                      PatchSampleAttachment's */

#if 0 /* @@@ future */
    List patch_Inputs;
    List patch_Outputs;
    List patch_Attachments;     ???
#endif
} Patch;

Patch *LoadPatch (const char *filename, Err *errbuf);
Err UnloadPatch (Patch *);
Err StartPatch (Patch *, const TagArg *);
Err ReleasePatch (Patch *, const TagArg *);
Err StopPatch (Patch *, const TagArg *);

/* -------------------- Editor structures */

/* PatchPage - editor page for grabbed PatchKnobs in a PatchInstrument */
typedef struct PatchPage
{
  Node ppage_Node; /* Link in ppage_PageList.  n_Name is not used. */

  PatchInstrument *ppage_Instrument; /* instrument this page edits */
  FaderBlock
      ppage_FaderBlock; /* faders for this instrument.  Each Fader's
                           fdr_UserItem is set to associated Knob's item # */
} PatchPage;

PatchPage *NewPatchPage (PatchInstrument *, Err *errbuf);
Err DeletePatchPage (PatchPage *);
Err ShowPatchPage (PatchPage *);

/* PatchPageList - collection of PatchPages for all instruments in a Patch */
typedef struct PatchPageList
{
  Patch *ppage_Patch;  /* Patch this page list edits */
  List ppage_PageList; /* list of pages for this patch */
} PatchPageList;

PatchPageList *NewPatchPageList (Patch *, Err *errbuf);
Err DeletePatchPageList (PatchPageList *);

/*
    @@@ should ultimately merge PatchPage and PatchPageList:

    {
        List ppage_Link                     List of pages for instruments in a
   patch (or empty if instrument) Node in list of parent patch's instrument
   list

        PatchInstrumentBase *ppage_inst;    instrument base class.  can really
   be instrument or patch FaderBlock ppage_FaderBlock;        faders for
   patch's or instrument's knobs } PatchPage

    PatchPage *BuildPatchPageTree (PatchInstrumentBase *)       builds entire
   tree PatchPage *NewPatchPage (PatchInstrumentBase *)             creates a
   single page
*/

/* -------------------- misc macros (suitable for publication) */

/* return # of items in array */
#define NArrayElts(_array) (sizeof _array / sizeof _array[0])

/* close anything (e.g. CloseRes (CloseLibrary, GfxBase); ) */
#define CloseRes(func, res)                                                   \
  do                                                                          \
    {                                                                         \
      if (res)                                                                \
        {                                                                     \
          func (res);                                                         \
          res = NULL;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

#define ufloor(n, d) ((n) / (d))                 /* unsigned int floor() */
#define uceil(n, d) ufloor ((n) + (d)-1, (d))    /* unsigned int ceil() */
#define uround(n, d) ufloor ((n) + (d) / 2, (d)) /* unsigned int round() */

/* -------------------- Debugging */

#define printavail()                                                          \
  do                                                                          \
    {                                                                         \
      MemInfo meminfo;                                                        \
                                                                              \
      availMem (&meminfo, MEMTYPE_ANY);                                       \
      printf ("sys=%lu:%lu task=%lu:%lu total=%lu", meminfo,                  \
              meminfo.minfo_SysFree + meminfo.minfo_TaskFree);                \
    }                                                                         \
  while (0)

/* -------------------- Misc functions */

#define myalloc(size) calloc ((size), 1)
#define myfree(p)                                                             \
  do                                                                          \
    {                                                                         \
      if (p)                                                                  \
        free (p);                                                             \
    }                                                                         \
  while (0)

#endif /* __PATCHDEMO_H */

/* $Id: faders.h,v 1.8 1994/02/28 21:26:37 peabody Exp $ */
/****************************************************************
**
**  Fader Includes
**
**  By:  Phil Burk
**
**  Copyright (c) 1993-1994, 3DO Company.
**  This program is proprietary and confidential.
**
*****************************************************************
**  940225 WJB  Added standard comment block.
**  940225 WJB  Added DrawFaderBlock().  Added const to DrawFader().
**  940228 WJB  Cleaned up includes.
****************************************************************/

#pragma include_only_once
#ifndef __FADERS_H
#define __FADERS_H

#include <types.h>

/* -------------------- Fader structures */

typedef struct Fader
{
  int32 fdr_XMin;
  int32 fdr_XMax;
  int32 fdr_YMin;
  int32 fdr_YMax;
  int32 fdr_VMin; /* Value min and max. */
  int32 fdr_VMax;
  int32 fdr_Value;
  int32 fdr_Increment;
  int32 fdr_Highlight;
  char *fdr_Name;

  /* user data - for use by client's event function */
  Item fdr_UserItem;
  void *fdr_UserData;
} Fader;

/* forward references for FaderBlock */
struct FaderBlock;
typedef struct FaderBlock FaderBlock;

typedef int32 (*FaderEventFunctionP) (int32 FaderIndex, int32 FaderValue,
                                      FaderBlock *fdbl);

struct FaderBlock
{
  int32 fdbl_Current; /* Index of current fader. */
  int32 fdbl_Previous;
  int32 fdbl_NumFaders; /* Number of Faders pointed to by fdbl_Faders (set by
                           client). */
  int32 fdbl_OneShot;   /* Allow single click to advance faders. */
  Fader *fdbl_Faders;   /* Pointer to Fader table (set by client). */
  FaderEventFunctionP fdbl_Function; /* Called on any fader value change event,
                                        even trivial ones. (set by client). */
};

/* -------------------- Misc defines */

#define FADER_HEIGHT (10)
#define FADER_SPACING (15)
#define FADER_YMIN (40)
#define MAX_FADER_VALUE (1000)

/* -------------------- Functions */

int32 InitFader (Fader *fdr, int32 Index);
Err DrawFader (const Fader *);
int32 JoyToFader (Fader *fdr, uint32 joy);
int32 UpdateFader (Fader *fdr, int32 val);

int32 InitFaderBlock (FaderBlock *fdbl, Fader *Faders, int32 NumFaders,
                      FaderEventFunctionP eventfn);
Err DrawFaderBlock (const FaderBlock *);
int32 DriveFaders (FaderBlock *fdbl, uint32 joy);

#endif /* __FADERS_H */

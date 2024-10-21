#pragma include_only_once
/*
** Fader Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1993, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "debug.h"
#include "mem.h"
#include "nodes.h"
#include "stdarg.h"
#include "string.h"
#include "types.h"

#include "graphic_tools.h"

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
} Fader;

typedef struct FaderBlock
{
  int32 fdbl_Current; /* Index of current fader. */
  int32 fdbl_Previous;
  int32 fdbl_NumFaders;
  int32 fdbl_OneShot; /* Allow single click to advance faders. */
  Fader *fdbl_Faders;
  int32 (*fdbl_Function) (int32 FaderIndex, int32 FaderValue);
} FaderBlock;

#define FADER_HEIGHT (10)
#define FADER_SPACING (15)
#define FADER_YMIN (40)
#define MAX_FADER_VALUE (1000)

int32 DrawFader (Fader *fdr);
int32 DriveFaders (FaderBlock *fdbl, uint32 joy);
int32 InitFader (Fader *fdr, int32 Index);
int32 InitFaderBlock (FaderBlock *fdbl, Fader *Faders, int32 NumFaders,
                      int32 (*CustomFunc) ());
int32 JoyToFader (Fader *fdr, uint32 joy);
int32 UpdateFader (Fader *fdr, int32 val);

/*****************************************************************************
 *	File:		TextLib.h
 *
 *	Contains:	Header file for handling text rendered via 3DO fonts.
 *
 *	Written by: Edgar Lee and Ian Lepore
 *
 *	Copyright:	© 1993 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and
 *proprietary information of the 3DO Company and shall not be used by any
 *Person or for any purpose except as expressly authorized in writing by the
 *3DO Company.
 *
 *	Change History (most recent first):
 *
 *	  12/09/93	Ian		First release version.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#pragma include_only_once

#ifndef TEXTLIB_H
#define TEXTLIB_H

#include "FontLib.h"
#include "graphics.h"
#include "stdarg.h"

/*----------------------------------------------------------------------------
 * format flags that can be specified at TextCelCreate() time.
 *--------------------------------------------------------------------------*/

#define TC_FORMAT_1BPPCEL 0x00000001 // 1 bit per pixel cel (not yet supported)
#define TC_FORMAT_2BPPCEL 0x00000002 // 2 bit per pixel cel (not yet supported)
#define TC_FORMAT_4BPPCEL 0x00000004 // 4 bit per pixel cel (not yet supported)
#define TC_FORMAT_8BPPCEL 0x00000008 // 8 bit per pixel cel (default)
#define TC_FORMAT_WORDWRAP                                                    \
  0x00000010 // auto-word-wrap text within cel (not yet supported)

/*----------------------------------------------------------------------------
 * TextCel structure.
 *	Clients should use only the tc_CCB field; all other fields are private
 *	to the implementation and if you touch them Bad Things Will Happen.
 *--------------------------------------------------------------------------*/

typedef struct TextCel
{
  CCB *tc_CCB;
  FontDescriptor *tc_fontDesc;
  int32 tc_fontAdjustSpacing;
  int32 tc_fontAdjustLeading;
  uint32 tc_formatFlags;
  char *tc_formatBuffer;
  uint32 tc_formatBufferSize;
  int32 tc_XPosInCel;
  int32 tc_YPosInCel;
  int32 tc_leftMargin;
  int32 tc_topMargin;
  int32 tc_penNumber;
  int32 tc_celRowBytes;
  uint32 tc_bgColor;
  uint32 tc_fgColor[4];
} TextCel;

/*----------------------------------------------------------------------------
 * prototypes for text-in-a-cel routines
 *--------------------------------------------------------------------------*/

TextCel *CreateTextCel (FontDescriptor *fDesc, uint32 formatFlags, int32 width,
                        int32 height);
TextCel *CloneTextCel (TextCel *templateTextCel, Boolean clonePixels);
void DeleteTextCel (TextCel *tCel);

void SetTextCelSpacingAdjust (TextCel *tCel, int32 adjustSpacing);
void SetTextCelLeadingAdjust (TextCel *tCel, int32 adjustLeading);
void SetTextCelColor (TextCel *tCel, int32 bgColor, int32 fgColor0);
void SetTextCelColors (TextCel *tCel, int32 bgColor, int32 fgColors[4]);
void SetTextCelCoords (TextCel *tCel, Coord ccbX, Coord ccbY);
void SetTextCelMargins (TextCel *tCel, int32 leftMargin, int32 topMargin);
void SetTextCelPenNumber (TextCel *tCel, int32 penNumber);
Err SetTextCelSize (TextCel *tCel, int32 width, int32 height);
Err SetTextCelFormatBuffer (TextCel *tCel, char *buffer, uint32 bufsize);

void GetTextCelSpacingAdjust (TextCel *tCel, int32 *adjustSpacing);
void GetTextCelLeadingAdjust (TextCel *tCel, int32 *adjustLeading);
void GetTextCelColor (TextCel *tCel, int32 *bgColor, int32 *fgColor0);
void GetTextCelColors (TextCel *tCel, int32 *bgColor, int32 fgColors[4]);
void GetTextCelCoords (TextCel *tCel, Coord *ccbX, Coord *ccbY);
void GetTextCelMargins (TextCel *tCel, int32 *leftMargin, int32 *topMargin);
void GetTextCelPenNumber (TextCel *tCel, int32 *penNumber);
void GetTextCelSize (TextCel *tCel, int32 *width, int32 *height);
void GetTextCelFormatBuffer (TextCel *tCel, char **buffer, uint32 *bufsize);

void EraseTextInCel (TextCel *tCel);

Err vUpdateTextInCel (TextCel *tCel, Boolean replaceExisting, char *fmtString,
                      va_list fmtArgs);
Err UpdateTextInCel (TextCel *tCel, Boolean replaceExisting, char *fmtString,
                     ...);

char *vGetTextExtent (TextCel *tCel, int32 *pWidth, int32 *pHeight,
                      char *fmtString, va_list fmtArgs);
char *GetTextExtent (TextCel *tCel, int32 *pWidth, int32 *pHeight,
                     char *fmtString, ...);

/*----------------------------------------------------------------------------
 * prototypes for render-direct-to-screen routines
 *--------------------------------------------------------------------------*/

void DrawTextString (FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem,
                     char *text, ...);
void DrawTextChar (FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem,
                   uint32 character);

#endif

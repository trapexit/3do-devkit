#pragma once

/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: textlib.h,v 1.3 1994/10/05 17:34:41 vertex Exp $
 **
 **  Lib3DO header file for handling text rendered via 3DO fonts.
 **
 ******************************************************************************/

#include "extern_c.h"

#include "fontlib.h"
#include "graphics.h"
#include "stdarg.h"

/*----------------------------------------------------------------------------
 * format flags that can be specified at TextCelCreate() time.
 *--------------------------------------------------------------------------*/

#define TC_FORMAT_LEFT_JUSTIFY		0x00000000	/* left justify text within cel */
#define TC_FORMAT_RIGHT_JUSTIFY		0x00000001	/* right justify text within cel */
#define TC_FORMAT_CENTER_JUSTIFY	0x00000002	/* center justify text within cel */
#define TC_FORMAT_FILL_JUSTIFY		0x00000003	/* fill justify within cel (not yet supported)  */
#define TC_FORMAT_WORDWRAP		0x00000008	/* auto-word-wrap text within cel */

#define	TC_FORMAT_JUSTIFY_MASK		0x00000007	/* mask off all flags, leaving just justification flags */

/*----------------------------------------------------------------------------
 * TextCel structure.
 *	Clients should use only the tc_CCB field; all other fields are private
 *	to the implementation and if you touch them Bad Things Will Happen.
 *--------------------------------------------------------------------------*/

typedef struct TextCel
{
  CCB            *tc_CCB;	/* pointer to CCB containing the text */
  void           *tc_userData;	/* client code can store a value here */
  FontDescriptor *tc_fontDesc;	/* everything from here down is internal-use-only */
  s32		  tc_fontAdjustSpacing;
  s32		  tc_fontAdjustLeading;
  u32	  tc_formatFlags;
  char           *tc_formatBuffer;
  u32	  tc_formatBufferSize;
  s32		  tc_XPosInCel;
  s32		  tc_YPosInCel;
  s32		  tc_leftMargin;
  s32		  tc_topMargin;
  s32		  tc_penNumber;
  s32		  tc_celRowBytes;
  u32	  tc_bgColor;
  u32	  tc_fgColor[4];
  u16	  tc_tabStops[16];
} TextCel;

/*----------------------------------------------------------------------------
 * TagArg interface.
 *--------------------------------------------------------------------------*/

enum {
      TCEL_TAG_FONT = 1,
      TCEL_TAG_FORMAT_FLAGS,
      TCEL_TAG_WIDTH,
      TCEL_TAG_HEIGHT,
      TCEL_TAG_SPACING_ADJUST,
      TCEL_TAG_LEADING_ADJUST,
      TCEL_TAG_BG_COLOR,
      TCEL_TAG_FG_COLOR0,
      TCEL_TAG_FG_COLOR1,
      TCEL_TAG_FG_COLOR2,
      TCEL_TAG_FG_COLOR3,
      TCEL_TAG_FG_COLORS,
      TCEL_TAG_CCB_X,
      TCEL_TAG_CCB_Y,
      TCEL_TAG_LEFT_MARGIN,
      TCEL_TAG_TOP_MARGIN,
      TCEL_TAG_PEN_NUMBER,
      TCEL_TAG_FORMAT_BUFFER,
      TCEL_TAG_FORMAT_BUFFER_SIZE,
      TCEL_TAG_TAB_STOPS,
      TCEL_TAG_REPLACE_EXISTING,
      TCEL_TAG_UPDATE_TEXT_STRING,
      TCEL_TAG_UPDATE_TEXT_ARGS
};

EXTERN_C_BEGIN

TextCel *taCreateTextCel(TagArg *args);
Err	   taModifyTextCel(TextCel *tCel, TagArg *args);

/*----------------------------------------------------------------------------
 * prototypes for text-in-a-cel routines
 *--------------------------------------------------------------------------*/

TextCel *CreateTextCel(const FontDescriptor *fDesc, u32 formatFlags, s32 width, s32 height);
TextCel *CloneTextCel(TextCel *templateTextCel, boolean clonePixels);
void	   DeleteTextCel(TextCel *tCel);
CCB     *DetachTextCelCCB(TextCel *tCel);

void SetTextCelSpacingAdjust(TextCel *tCel, s32 adjustSpacing);
void SetTextCelLeadingAdjust(TextCel *tCel, s32 adjustLeading);
void SetTextCelColor(TextCel *tCel, s32 bgColor, s32 fgColor0);
void SetTextCelColors(TextCel *tCel, s32 bgColor, s32 fgColors[4]);
void SetTextCelCoords(TextCel *tCel, Coord ccbX, Coord ccbY);
void SetTextCelMargins(TextCel *tCel, s32 leftMargin, s32 topMargin);
void SetTextCelPenNumber(TextCel *tCel, s32 penNumber);
void SetTextCelFormatFlags(TextCel *tCel, u32 formatFlags);
Err  SetTextCelSize(TextCel *tCel, s32 width, s32 height);
Err  SetTextCelFormatBuffer(TextCel *tCel, char *buffer, u32 bufsize);
void SetTextCelTabStops(TextCel *tCel, u16 tabStops[16], ...);

void   GetTextCelSpacingAdjust(TextCel *tCel, s32 *adjustSpacing);
void   GetTextCelLeadingAdjust(TextCel *tCel, s32 *adjustLeading);
void   GetTextCelColor(TextCel *tCel, s32 *bgColor, s32 *fgColor0);
void   GetTextCelColors(TextCel *tCel, s32 *bgColor, s32 fgColors[4]);
void   GetTextCelCoords(TextCel *tCel, Coord *ccbX, Coord *ccbY);
void   GetTextCelMargins(TextCel *tCel, s32 *leftMargin, s32 *topMargin);
void   GetTextCelPenNumber(TextCel *tCel, s32 *penNumber);
u32 GetTextCelFormatFlags(TextCel *tCel, u32 *formatFlags);
void   GetTextCelSize(TextCel *tCel, s32 *width, s32 *height);
void   GetTextCelFormatBuffer(TextCel *tCel, char **buffer, u32 *bufsize);
void   GetTextCelTabStops(TextCel *tCel, u16 tabStops[16]);

void EraseTextInCel(TextCel *tCel);

Err vUpdateTextInCel(TextCel *tCel, boolean replaceExisting, const char *fmtString, va_list fmtArgs);
Err UpdateTextInCel(TextCel *tCel, boolean replaceExisting, const char *fmtString, ...);

char *vGetTextExtent(TextCel *tCel, s32 *pWidth, s32 *pHeight, const char *fmtString, va_list fmtArgs);
char *GetTextExtent(TextCel *tCel, s32 *pWidth, s32 *pHeight, const char *fmtString, ...);

/*----------------------------------------------------------------------------
 * prototypes for render-direct-to-screen routines
 *--------------------------------------------------------------------------*/

void DrawTextString(FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem, const char *text, ...);
void DrawTextChar(FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem, u32 character);

EXTERN_C_END

#ifndef __TEXTLIB_H
#define __TEXTLIB_H

#pragma force_top_level
#pragma include_only_once


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
#define TC_FORMAT_WORDWRAP			0x00000008	/* auto-word-wrap text within cel */

#define	TC_FORMAT_JUSTIFY_MASK		0x00000007	/* mask off all flags, leaving just justification flags */

/*----------------------------------------------------------------------------
 * TextCel structure.
 *	Clients should use only the tc_CCB field; all other fields are private
 *	to the implementation and if you touch them Bad Things Will Happen.
 *--------------------------------------------------------------------------*/

typedef struct TextCel {
	CCB *				tc_CCB;					/* pointer to CCB containing the text */
	void *				tc_userData;			/* client code can store a value here */
	FontDescriptor *	tc_fontDesc;			/* everything from here down is internal-use-only */
	int32				tc_fontAdjustSpacing;
	int32				tc_fontAdjustLeading;
	uint32				tc_formatFlags;
	char *				tc_formatBuffer;
	uint32				tc_formatBufferSize;
	int32				tc_XPosInCel;
	int32				tc_YPosInCel;
	int32				tc_leftMargin;
	int32				tc_topMargin;
	int32				tc_penNumber;
	int32				tc_celRowBytes;
	uint32				tc_bgColor;
	uint32				tc_fgColor[4];
	uint16				tc_tabStops[16];
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

#ifdef __cplusplus
  extern "C" {
#endif

TextCel *	taCreateTextCel(TagArg *args);
Err			taModifyTextCel(TextCel *tCel, TagArg *args);

/*----------------------------------------------------------------------------
 * prototypes for text-in-a-cel routines
 *--------------------------------------------------------------------------*/

TextCel *	CreateTextCel(FontDescriptor *fDesc, uint32 formatFlags, int32 width, int32 height);
TextCel *	CloneTextCel(TextCel *templateTextCel, Boolean clonePixels);
void		DeleteTextCel(TextCel *tCel);
CCB *		DetachTextCelCCB(TextCel *tCel);

void		SetTextCelSpacingAdjust(TextCel *tCel, int32 adjustSpacing);
void		SetTextCelLeadingAdjust(TextCel *tCel, int32 adjustLeading);
void		SetTextCelColor(TextCel *tCel, int32 bgColor, int32 fgColor0);
void		SetTextCelColors(TextCel *tCel, int32 bgColor, int32 fgColors[4]);
void		SetTextCelCoords(TextCel *tCel, Coord ccbX, Coord ccbY);
void		SetTextCelMargins(TextCel *tCel, int32 leftMargin, int32 topMargin);
void		SetTextCelPenNumber(TextCel *tCel, int32 penNumber);
void		SetTextCelFormatFlags(TextCel *tCel, uint32 formatFlags);
Err			SetTextCelSize(TextCel *tCel, int32 width, int32 height);
Err			SetTextCelFormatBuffer(TextCel *tCel, char *buffer, uint32 bufsize);
void		SetTextCelTabStops(TextCel *tCel, uint16 tabStops[16], ...);

void		GetTextCelSpacingAdjust(TextCel *tCel, int32 *adjustSpacing);
void		GetTextCelLeadingAdjust(TextCel *tCel, int32 *adjustLeading);
void		GetTextCelColor(TextCel *tCel, int32 *bgColor, int32 *fgColor0);
void		GetTextCelColors(TextCel *tCel, int32 *bgColor, int32 fgColors[4]);
void		GetTextCelCoords(TextCel *tCel, Coord *ccbX, Coord *ccbY);
void		GetTextCelMargins(TextCel *tCel, int32 *leftMargin, int32 *topMargin);
void		GetTextCelPenNumber(TextCel *tCel, int32 *penNumber);
uint32		GetTextCelFormatFlags(TextCel *tCel, uint32 *formatFlags);
void		GetTextCelSize(TextCel *tCel, int32 *width, int32 *height);
void		GetTextCelFormatBuffer(TextCel *tCel, char **buffer, uint32 *bufsize);
void		GetTextCelTabStops(TextCel *tCel, uint16 tabStops[16]);

void		EraseTextInCel(TextCel *tCel);

Err			vUpdateTextInCel(TextCel *tCel, Boolean replaceExisting, char *fmtString, va_list fmtArgs);
Err		 	 UpdateTextInCel(TextCel *tCel, Boolean replaceExisting, char *fmtString, ...);

char *		vGetTextExtent(TextCel *tCel, int32 *pWidth, int32 *pHeight, char *fmtString, va_list fmtArgs);
char *	 	 GetTextExtent(TextCel *tCel, int32 *pWidth, int32 *pHeight, char *fmtString, ...);

/*----------------------------------------------------------------------------
 * prototypes for render-direct-to-screen routines
 *--------------------------------------------------------------------------*/

void	DrawTextString(FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem, char *text, ...);
void	DrawTextChar(FontDescriptor *fDesc, GrafCon *gcon, Item bitmapItem, uint32 character);

#ifdef __cplusplus
  }
#endif

#endif	/* __TEXTLIB_H */

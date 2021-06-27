/*****************************************************************************
 *	File:		taTextLib.c
 *
 *	Contains:	The TagArgs interface to the TextLib library.
 *
 *	Written by:  Edgar Lee and Ian Lepore
 *
 *	Copyright:	© 1994 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *	Change History (most recent first):
 *
 *	  05/08/94	Ian		New today.
 *
 *	Implementation notes:
 *	
 *	DeleteCel() compatible.
 *
 ****************************************************************************/

#include "TextLib.h"
#include "Debug3DO.h"
#include "operamath.h"
#include "string.h"

/*----------------------------------------------------------------------------
 * misc internal stuff...
 *--------------------------------------------------------------------------*/

typedef struct ParsedTags {
	uint32			valid;
	FontDescriptor *font;
	int32			formatFlags;
	int32			width;
	int32			height;
	int32			spacing;
	int32			leading;
	int32			bgColor;
	int32			fgColor0;
	int32			fgColor1;
	int32			fgColor2;
	int32			fgColor3;
	int32 *			fgColors;
	frac16			ccbX;
	frac16			ccbY;
	int32			leftMargin;
	int32			topMargin;
	int32			penNumber;
	char *			formatBuffer;
	int32			formatBufferSize;
	uint16 *		tabStops;
	Boolean			replaceExisting;
	char *			textFormat;
	va_list			textArgs;
} ParsedTags;

typedef union ArgValue {
	void *				vptr;
	char *				cptr;
	int32 *				iptr;
	uint16 *			usptr;
	FontDescriptor *	fdptr;
	int32				inum;
	uint32				unum;
	frac16				fnum;
} ArgValue;

#define SetValid(s, tag)	((s)->valid |= (1 << (tag)))
#define ResetValid(s, tag)	((s)->valid &= ~(1 << (tag)))
#define IsValid(s, tag)		((s)->valid & (1 << (tag)))

#define FOR_MODIFY		0
#define FOR_CREATE		1

/*----------------------------------------------------------------------------
 * code...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * 
 ****************************************************************************/

static void init_tags(ParsedTags *pt, int32 init_for)
{
	memset(pt, 0, sizeof(*pt));
	
	pt->leftMargin	= -1;
	pt->topMargin	= -1;
	pt->bgColor		= -1;
	pt->fgColor0	= -1;
	pt->fgColor1	= -1;
	pt->fgColor2	= -1;
	pt->fgColor3	= -1;
	pt->fgColors	= &pt->fgColor0;

	if (init_for == FOR_MODIFY) {
		pt->width	= -1;
		pt->height	= -1;
	}
}

/*****************************************************************************
 * 
 ****************************************************************************/

Err	parse_tags(ParsedTags *pt, TagArg *args, int32 parse_for)
{
	Err			err = -1;
	ArgValue	val;
	
	if (args == NULL) {
		DIAGNOSE(("Invalid NULL TagArg pointer\n"));
		goto ERROR_EXIT;
	}

	do	{
		val.vptr = args->ta_Arg;
		switch (args->ta_Tag) {
		  case TAG_END:
		  	break;
		  case TCEL_TAG_FONT:
		  	pt->font = val.fdptr;
			SetValid(pt, TCEL_TAG_FONT);
		  	break;
		  case TCEL_TAG_FORMAT_FLAGS:
		  	pt->formatFlags = val.inum;
			SetValid(pt, TCEL_TAG_FORMAT_FLAGS);
		  	break;
		  case TCEL_TAG_WIDTH:
		  	pt->width = val.inum;
			SetValid(pt, TCEL_TAG_WIDTH);
		  	break;
		  case TCEL_TAG_HEIGHT:
		  	pt->height = val.inum;
			SetValid(pt, TCEL_TAG_HEIGHT);
		  	break;
		  case TCEL_TAG_SPACING_ADJUST:
		  	pt->spacing = val.inum;
			SetValid(pt, TCEL_TAG_SPACING_ADJUST);
		  	break;
		  case TCEL_TAG_LEADING_ADJUST:
		  	pt->leading = val.inum;
			SetValid(pt, TCEL_TAG_LEADING_ADJUST);
		  	break;
		  case TCEL_TAG_BG_COLOR:
		  	pt->bgColor = val.inum;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_FG_COLOR0:
		  	pt->fgColor0 = val.inum;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_FG_COLOR1:
		  	pt->fgColor1 = val.inum;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_FG_COLOR2:
		  	pt->fgColor2 = val.inum;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_FG_COLOR3:
		  	pt->fgColor3 = val.inum;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_FG_COLORS:
		  	pt->fgColors = val.iptr;
			SetValid(pt, TCEL_TAG_FG_COLORS);
		  	break;
		  case TCEL_TAG_CCB_X:
		  	pt->ccbX = val.fnum;
			SetValid(pt, TCEL_TAG_CCB_X);
		  	break;
		  case TCEL_TAG_CCB_Y:
		  	pt->ccbY = val.fnum;
			SetValid(pt, TCEL_TAG_CCB_Y);
		  	break;
		  case TCEL_TAG_LEFT_MARGIN:
		  	pt->leftMargin = val.inum;
			SetValid(pt, TCEL_TAG_LEFT_MARGIN);
		  	break;
		  case TCEL_TAG_TOP_MARGIN:
		  	pt->topMargin = val.inum;
			SetValid(pt, TCEL_TAG_TOP_MARGIN);
		  	break;
		  case TCEL_TAG_PEN_NUMBER:
		  	pt->penNumber = val.inum;
			SetValid(pt, TCEL_TAG_PEN_NUMBER);
		  	break;
		  case TCEL_TAG_FORMAT_BUFFER:
		  	pt->formatBuffer = val.cptr;
			SetValid(pt, TCEL_TAG_FORMAT_BUFFER);
		  	break;
		  case TCEL_TAG_FORMAT_BUFFER_SIZE:
		  	pt->formatBufferSize = val.inum;
			SetValid(pt, TCEL_TAG_FORMAT_BUFFER_SIZE);
		  	break;
		  case TCEL_TAG_TAB_STOPS:
		  	pt->tabStops = val.usptr;
			SetValid(pt, TCEL_TAG_TAB_STOPS);
		  	break;
		  case TCEL_TAG_REPLACE_EXISTING:
		  	pt->replaceExisting = (Boolean)val.inum;
			SetValid(pt, TCEL_TAG_REPLACE_EXISTING);
		  	break;
		  case TCEL_TAG_UPDATE_TEXT_STRING:
		  	pt->textFormat = val.cptr;
			SetValid(pt, TCEL_TAG_UPDATE_TEXT_STRING);
		  	break;
		  case TCEL_TAG_UPDATE_TEXT_ARGS:
		  	pt->textArgs = val.cptr;
			SetValid(pt, TCEL_TAG_UPDATE_TEXT_ARGS);
		  	break;
		  default:
		  	DIAGNOSE(("Unknown TextCel TagArgs tag %ld\n", args->ta_Tag));
			goto ERROR_EXIT;
		}
	} while ((args++)->ta_Tag != TAG_END);

	if (IsValid(pt, TCEL_TAG_UPDATE_TEXT_ARGS) && !IsValid(pt, TCEL_TAG_UPDATE_TEXT_STRING)) {
		DIAGNOSE(("TCEL_TAG_UPDATE_TEXT_STRING must be specified when TCEL_TAG_UPDATE_TEXT_ARGS is present\n"));
		goto ERROR_EXIT;
	}
	
	if (IsValid(pt, TCEL_TAG_FORMAT_BUFFER) && !IsValid(pt, TCEL_TAG_FORMAT_BUFFER_SIZE) && pt->formatBuffer != NULL) {
		DIAGNOSE(("TCEL_TAG_FORMAT_BUFFER_SIZE must be specified when TCEL_TAG_FORMAT_BUFFER is non-NULL\n"));
		goto ERROR_EXIT;
	}

	if (parse_for == FOR_MODIFY) {
		if (IsValid(pt, TCEL_TAG_FONT)) {
			DIAGNOSE(("Cannot change TextCel font\n"));
			goto ERROR_EXIT;
		}
	} else {
		if (!IsValid(pt, TCEL_TAG_FONT)) {
			DIAGNOSE(("Must specify TCEL_TAG_FONT when creating a TextCel\n"));
			goto ERROR_EXIT;
		}
	}

	err = 0;

ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * 
 ****************************************************************************/

static Err apply_tags(TextCel *tCel, ParsedTags *pt)
{
	Err	err = 0;

	if (IsValid(pt, TCEL_TAG_CCB_X)) {
		if (pt->ccbX < 1024 && pt->ccbX > -1024) {
			tCel->tc_CCB->ccb_XPos = Convert32_F16(pt->ccbX);
		} else {
			tCel->tc_CCB->ccb_XPos = pt->ccbX;
		}
	}
	
	if (IsValid(pt, TCEL_TAG_CCB_Y)) {
		if (pt->ccbY < 1024 && pt->ccbY > -1024) {
			tCel->tc_CCB->ccb_YPos = Convert32_F16(pt->ccbY);
		} else {
			tCel->tc_CCB->ccb_YPos = pt->ccbY;
		}
	}
	
	if (IsValid(pt, TCEL_TAG_FORMAT_FLAGS)) {
		SetTextCelFormatFlags(tCel, pt->formatFlags);
	}
	
	if (IsValid(pt, TCEL_TAG_SPACING_ADJUST)) {
		SetTextCelSpacingAdjust(tCel, pt->spacing);
	}
	
	if (IsValid(pt, TCEL_TAG_LEADING_ADJUST)) {
		SetTextCelLeadingAdjust(tCel, pt->leading);
	}
	
	if (IsValid(pt, TCEL_TAG_FG_COLORS)) {
		SetTextCelColors(tCel, pt->bgColor, pt->fgColors);
	}
	
	if (IsValid(pt, TCEL_TAG_LEFT_MARGIN) || IsValid(pt, TCEL_TAG_TOP_MARGIN)) {
		SetTextCelMargins(tCel, pt->leftMargin, pt->topMargin);
	}
	
	if (IsValid(pt, TCEL_TAG_PEN_NUMBER)) {
		SetTextCelPenNumber(tCel, pt->penNumber);
	}
	
	if (IsValid(pt, TCEL_TAG_TAB_STOPS)) {
		SetTextCelTabStops(tCel, pt->tabStops);
	}
	
	if (IsValid(pt, TCEL_TAG_WIDTH) || IsValid(pt, TCEL_TAG_HEIGHT)) {
		if ((err = SetTextCelSize(tCel, pt->width, pt->height)) < 0) {
			goto ERROR_EXIT;
		}
	}
	
	if (IsValid(pt, TCEL_TAG_FORMAT_BUFFER) || IsValid(pt, TCEL_TAG_FORMAT_BUFFER_SIZE)) {
		if ((err = SetTextCelFormatBuffer(tCel, pt->formatBuffer, pt->formatBufferSize)) < 0) {
			goto ERROR_EXIT;
		}
	}
	
	if (IsValid(pt, TCEL_TAG_UPDATE_TEXT_STRING)) {
		err = vUpdateTextInCel(tCel, pt->replaceExisting, pt->textFormat, pt->textArgs);
	}

ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * 
 ****************************************************************************/

TextCel * taCreateTextCel(TagArg *args)
{
	Err			err;
	ParsedTags	pt;
	TextCel *	tCel = NULL;
	
	init_tags(&pt, FOR_CREATE);
	
	if ((err = parse_tags(&pt, args, FOR_CREATE)) < 0) {
		goto ERROR_EXIT;
	}

	if ((tCel = CreateTextCel(pt.font, pt.formatFlags, pt.width, pt.height)) == NULL) {
		err = -1;
		goto ERROR_EXIT;
	}
	ResetValid(&pt, TCEL_TAG_FONT);
	ResetValid(&pt, TCEL_TAG_FORMAT_FLAGS);
	ResetValid(&pt, TCEL_TAG_WIDTH);
	ResetValid(&pt, TCEL_TAG_HEIGHT);
	
	err = apply_tags(tCel, &pt);
	
ERROR_EXIT:

	if (err < 0) {
		DeleteTextCel(tCel);
		tCel = NULL;
	}
	
	return tCel;
}

/*****************************************************************************
 * 
 ****************************************************************************/

Err	taModifyTextCel(TextCel *tCel, TagArg *args)
{
	Err			err;
	ParsedTags	pt;
	
	init_tags(&pt, FOR_MODIFY);
	
	if ((err = parse_tags(&pt, args, FOR_MODIFY)) < 0) {
		goto ERROR_EXIT;
	}
	
	err = apply_tags(tCel, &pt);
	
ERROR_EXIT:
	
	return err;
}


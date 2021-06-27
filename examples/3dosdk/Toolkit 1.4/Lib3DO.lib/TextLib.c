/*****************************************************************************
 *	File:		TextLib.c
 *
 *	Contains:	Implementation file for handling text rendered via 3DO fonts.
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
 *	  05/12/94	Ian		Fixed a bug in CloneTextCel():  added a call to
 *						recalc_colors() so that the PLUT in the new cel
 *						gets populated after copying the bg/fg colors from
 *						the template TextCel to the new TextCel.
 *						Changed behavior when width/height are zero on 
 *						CreateTextCel().  Instead of allocating a 1x1 pixel 
 *						cel as a placeholder until some text gets rendered 
 *						to trigger autosizing, we now use the font width and
 *						height as the placeholder values.  The point to this
 *						is that you can create an autosize TextCel, and even
 *						before rendering into it get a little bit of useful
 *						info from GetTextCelSize() (the useful info being
 *						the width and height of characters in the font).  The
 *						main reason for this is that it simplifies some logic
 *						in the TextList handler.
 *
 *    05/12/94	Edgar	Added check for '\r' = $0d to go along with '\n' = $0a.
 *						In recalc_and_render_text_in_cel(), made check to see if
 *						charWidth > 0 beforing rendering. 
 *
 *	  05/11/94	Ian		Added a check in SetTextCelPenColor() that warns when you
 *						try to change pens in a TWOCOLOR cel, since that isn't 
 *						supported.  Also, in SetTextCelCoords(), if the new X and Y
 *						parms are in the range -1024<coord<1024 they are assumed to
 *						be integer (not frac16) values, and are adjusted to frac16
 *						as they are stored into the CCB.  (frac16)1024 is almost
 *						(zero and if folks are setting cel coords with that sort
 *						of accuracy they're almost surely using MapCel() instead
 *						of SetTextCelCoords().
 *
 *	  05/08/94	Ian		Added DetachTextCelCCB().
 *						Fixed a bug in SetTextCelMargins(): it wouldn't let you 
 *						set the margins to zero if they were currently non-zero.
 *						Added a feature to SetTextCelColor[s](): for any color
 *						values that are less than zero, the corresponding value 
 *						in the TextCel is left unchanged.  This lets you change
 *						background color without knowing or changing the foreground
 *						color, and helps support the new TagArgs interface too.
 *						Added a feature in SetTextCelSize(): if the width or height
 *						values passed in are negative, the corresponding values 
 *						are not changed.
 *
 *	  05/07/94	Ian		Added support for outlined/shadowed fonts.  On the FontLib
 *						side of the world these are 5 bit per pixel fonts where the
 *						3 low-order bits select anti-aliasing level (like regular
 *						3bpp fonts) and the 2 high-order bits are preset pen index
 *						values to select the right type of anti-aliasing (fg to existing,
 *						outline to existing, or fg to outline).  When a TextCel is
 *						created using a font that has the TWO_IMAGES flag set, we set
 *						TC_INTERNAL_TWOCOLOR in the TextCel flags, and the recalc_colors()
 *						routine behaves a bit differently.  Other than the special 
 *						handling of the PLUT calcs, TWOCOLOR TextCels are just like
 *						regular TextCels.
 *
 *	  04/08/94  Ian		Fixed two bugs related to tabs.  CloneTextCel() was
 *						not copying the template cel's tabstops table to the new
 *						cel.  Also, SetTextCelTabStops() was not plugging the 
 *						terminating zero into the table when the stops were
 *						specified via the variable args list.
 *
 *	  03/27/94  Ian		Removed the ArgsByValue hack that was working around the
 *						lack of a vsprintf() function.  It turns out that vsprintf()
 *						is present in clib.lib, but its prototype is missing from
 *						stdio.h.  This also removes the old 10-args limitation.
 *
 *	  03/24/94  Ian		Fixed a bug in CloneTextCel().  When the clone pixels 
 *						option was used, only the first 4 bytes of the pixel data
 *						area was getting copied.  (I, ummm, sorta used sizeof()
 *						where I *meant* MemBlockSize().)
 *
 *	  03/20/94  Ian		Added DeleteCel() support, active only if DELETECELMAGIC_SUPPORT
 *						is defined by the makefile.  
 *
 *	  03/16/94  Ian		Minor bugfix:  Don't try to render characters that aren't
 *						present in the font (ie, chars with a width of zero).
 *
 *	  03/05/94	Ian		Added support for tab stops.  Not sure I did it the right or
 *						best way, but it seems to work.
 *
 *	  02/28/94  Edgar	Changed the celWidth calculation in get_line_chars(),
 *						the celRightEdge calculation in recalc_and_render_text_in_cel(),
 *						and the pWidth and pHeight calculations in vGetTextExtent()
 *						in order to support the SetTextCelMargins() routine.  The routine
 *						will now define a margin that completely surrounds the text by
 *						creating a TextCel that's (2*leftMargin + textWidth) wide and
 *						(2*topMargin + textHeight) high.  Also made sure that the justi-
 *						fication would never allow the text to flow anywhere within the
 *						margin areas.
 *
 *	  02/26/94  Ian		Added logic to recalc_colors() to improve handling of 
 *						near-black text.  The old logic was calc'ing scaled 
 *						colors of 0 when the foreground color was near-black,
 *						and the zero PLUT entries were taken as transparent 
 *						pixels, causing really ugly output.  Now, a color that comes
 *						out of the scaling calc as zero is stored in the PLUT as 001 
 *						(near black) so that pixels near the edges of characters
 *						don't get skipped just because the anti-alias level for them 
 *						is essentially black.  All of this was only an issue when
 *						the background color was 0 (transparent); a non-transparent
 *						background goes through different PIXC logic, and
 *						transparency is turned off via the CCB flags.  Also, for
 *						future reference, my earlier attempts to apply this exact
 *						fix had failed because of a documentation error:  apparently
 *						the preamble word 1 UNCLSB value is NOT just for uncoded
 *						source data (as the docs state) but applies just as equally
 *						to the blue LSB coming from the PLUT in a coded cel.
 *
 *	  02/11/94  Edgar	Added SetTextCelFormatFlags() and GetTextCelFormatFlags()
 *						for setting and retrieving the format flags for a 
 *						text cel.
 *
 *						Created justification flags and changed recalc_and
 *						_render_text_in_cel() and get_line_chars() to support
 *						text justification.  Now text can be specified as left,
 *						right, and center justified.
 *
 *						Changed the BPP flags to be internal and updated all the
 *						routines that depend on the bpp stuff.
 *
 *	  02/10/94  Edgar	In vGetTextExtent(), removed the code for calculating
 *						the width and height needed to display text in a cel.
 *						That's now done in recalc_and_render_text_in_cel().
 *						When the doRendering parameter is TRUE, recalc_and_render
 *						_text_in_cel() renders the characters in the cel.  But
 *						when doRendering is FALSE, the routine only calculates
 *						the width and height needed to contain the formatted
 *						text.  Note that word-wrapping is taken into account if
 *						the TC_FORMAT_WORDWRAP flag is enabled and the cel width
 *						is no being autosized.
 *
 *						Separated the internal flag TC_INTERNAL_AUTOSIZE into
 *						two flags, TC_INTERNAL_AUTOSIZE_WIDTH and TC_INTERNAL
 *						_AUTOSIZE_HEIGHT.  Now a cel can be created with only
 *						its width or its height autosized; however, if the width
 *						is set to autosize, word-wrapping is ignored.
 *					
 *						Changed get_line_chars() to handle character clipping
 *						and not just word clipping.  Now get_line_chars() works
 *						the same when word-wrapping is enabled or disabled.
 *
 *						Renamed getLineChars() to get_line_chars().
 *
 *	  02/09/94  Edgar	Combined render_text_in_cel() and
 *						render_wrapped_text_in_cel() into one routine
 *						called recalc_and_render_text_in_cel().
 *
 *	  02/08/94  Edgar	In getLineChars(), added an extra charSpacing to the
 *						boxWidth and celWidth to handle the extra spacing
 *						after the last character in one line of text.
 *
 *	  02/07/94  Edgar	Added render_wrapped_text_in_cel() and getLineChars()
 * 						for handling word-wrapping when rendering text in a
 *						cel.  If the TC_FORMAT_WORDWRAP format flag is set
 *						when creating a text cel, words falling outside the
 *						specified cel width boundaries are wrapped to
 *						the next row of text in the cel.  If a single word
 *						width is greater than the entire cel width, that word
 *						is broken and the remaining characters for that word
 *						are wrapped to the next row.
 *						
 *						The call to render_wrapped_text_in_cel() is 
 *						made within vUpdateTextInCel(). 
 *
 *	  02/02/94  Edgar	Added a check in alloc_text_celdata() for text cel
 *						widths exceding the cel engine limitation of 2047 pixels.
 *						Now if a text cel is defined with a width greater than
 *						2047, the width is truncated to 2047.
 *
 *	  02/01/94  Ian		Added a check for negative width/height requested in
 *						alloc_text_celdata(); that's now an error.
 *						Also, removed the logic from alloc_text_celdata() that
 *						would try to reallocate the data buffer only if it
 *						needed to be bigger than the current one.  Also removed
 *						the logic in SetTextCelSize() that would Free() and NULL
 *						the celdata buffer to work around the logic in the alloc
 *						routine.  It turns out that the workaround was forcing
 *						the alloc routine to always realloc the buffer anyway,
 *						but the combo of the workaround and the realloc logic
 *						was making it so that a failed allocation would leave
 *						a CCB with a NULL SourcePtr field (always bad news).
 * 
 *	  12/09/93	Ian		First release version.
 * 
 *	Implementation notes:
 *	
 *	DeleteCel() compatible.
 *
 *	Naming standard:  if it's mixed case, it's exported to the outside 
 *	world; if it's lowercase with underbars, it's private to this module.
 *
 *	For a description of how the anti-aliased text logic works (more or less)
 *	see the comment block for recalc_colors(), below.
 ****************************************************************************/

#define DELETECELMAGIC_SUPPORT 1
#define VARIABLE_DESTBPP_IMPLEMENTED  0	// we're not yet supporting varying destination pixel depths

#include "TextLib.h"
#include "UMemory.h"
#include "Debug.h"
#include "Debug3DO.h"
#include "Macros3DO.h"
#include "operamath.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"

#if DELETECELMAGIC_SUPPORT
  #include "CelUtils.h"
  #include "DeleteCelMagic.h"
#endif

/*----------------------------------------------------------------------------
 * misc internal constants...
 *--------------------------------------------------------------------------*/

#define PLUTSIZE	(32*sizeof(uint16))

#define PIXC_OPAQUE			0x1F001F00
#define PIXC_8BPP_BLEND		0x1F00bfc0
#define PIXC_4BPP_BLEND		0xE090E000

#define TC_INTERNAL_1BPPCEL			0x00010000	// 1 bit per pixel cel (not yet supported)
#define TC_INTERNAL_2BPPCEL			0x00020000	// 2 bit per pixel cel (not yet supported)
#define TC_INTERNAL_4BPPCEL			0x00040000	// 4 bit per pixel cel (not yet supported)
#define TC_INTERNAL_8BPPCEL			0x00080000	// 8 bit per pixel cel (default)

#define TC_INTERNAL_DYNBUF			0x01000000	// format buffer dynamically allocated by us
#define TC_INTERNAL_AUTOSIZE_WIDTH	0x02000000	// auto-resize width celdata as needed to fit text
#define TC_INTERNAL_AUTOSIZE_HEIGHT	0x04000000	// auto-resize height celdata as needed to fit text
#define TC_INTERNAL_TWOCOLOR		0x08000000	// 5bpp font data contains color indicies in pixels

#define TC_INTERNAL_FLAGSMASK		0xFFFF0000	// masks off client flags, leaving just internal flags
#define TC_FORMAT_FLAGSMASK			0x0000FFFF	// mask off internal flags, leaving just client flags
#define TC_FORMAT_BPPMASK			0x000F0000	// masks off non-BPP-related flags, leaving just BPP

/*----------------------------------------------------------------------------
 * internal routines...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * delete_textcel_callback()
 *	The callback routine invoked when DeleteCel() encounters a TextCel.
 ****************************************************************************/

#if DELETECELMAGIC_SUPPORT

static CCB * delete_textcel_callback(void *creatorData, CCB *cel)
{
	CCB *	next = CEL_NEXTPTR(cel);
	
	DeleteTextCel((TextCel*)creatorData);
	return next;
}

#endif
 
/*****************************************************************************
 * alloc_text_celdata()
 *	Allocate the a cel data buffer based on the width/height/bpp, and set the
 *	CCB fields related to width/height/bpp.
 ****************************************************************************/

static int32 alloc_text_celdata(CCB *pCel, int32 w, int32 h, int32 bpp)
{
	int32		rowBytes;
	int32		rowWOFFSET;
	int32		wPRE;
	int32		hPRE;
 	CelData *	oldCelData;
	CelData *	newCelData;
	int32		newCelDataSize;	
	
#if DEBUG
 	if (bpp != 8) {
		DIAGNOSE(("Currently supporting only 8 bits per pixel text cels\n"));
		return -1;
	}
	
	if (w <= 0 || h <= 0) {
		DIAGNOSE(("Width (%ld) and Height (%ld) must be greater than zero\n", w, h));
		return -1;
	}
	
	if (w > 2047) {
		DIAGNOSE(("Width (%ld) truncated to cel engine limit of 2047\n", w));
		w = 2047;
	}
#endif

	/*------------------------------------------------------------------------
	 * set up the preamble words.
	 *	if either cel width or height is zero force it to one, because zero
	 *	would lead to bogus values that confuse the cel engine.  we don't
	 *	consider zero width or height to be an error; if the caller thinks
	 *	it's valid, then we do the best we can to create a cel.
	 *	we have to set the bytes-per-row value to a a word-aligned value,
	 *	and further have to allow for the cel engine's hardwired minimum
	 *	of two words per row even when the pixels would fit in one word.
	 *----------------------------------------------------------------------*/

	rowBytes   = (((w * bpp) + 31) / 32) * 4;
	if (rowBytes < 8) {
		rowBytes = 8;
	}
	rowWOFFSET = (rowBytes / 4) - PRE1_WOFFSET_PREFETCH;

	wPRE = (w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT;
	hPRE = (h - PRE0_VCNT_PREFETCH)    << PRE0_VCNT_SHIFT;
	
	/*------------------------------------------------------------------------
	 * if the cel doesn't already have a data buffer attached to it, or if
	 * the new width/height values need a data buffer bigger than the one
	 * currently attached, allocate the new buffer and free the old one.
	 *----------------------------------------------------------------------*/

	newCelDataSize	= h * rowBytes;
	oldCelData 		= pCel->ccb_SourcePtr;
	newCelData = (CelData *)Malloc(newCelDataSize, MEMTYPE_CEL|MEMTYPE_FILL|0);
	if (newCelData == NULL) {
		DIAGNOSE(("Can't allocate memory for text cel data\n"));
		return -2;
	}
	pCel->ccb_SourcePtr = newCelData;
	if (oldCelData != NULL) {
		Free(oldCelData);
	}
	
	/*------------------------------------------------------------------------
	 * fill in the CCB width/height/preamble fields.
	 *----------------------------------------------------------------------*/

	pCel->ccb_Width		= w;
	pCel->ccb_Height	= h;
	
	wPRE |= PRE1_TLLSB_PDC0;
	
	switch (bpp) {
	  case 8:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_8;	
		pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);					
		break;
#if VARIABLE_DESTBPP_IMPLEMENTED
	  case 1:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_1;	
		pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 2:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_2;	
		pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 4:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_4;	
		pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 6:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_6;	
		pCel->ccb_PRE1 = wPRE| (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 16:	
	  	pCel->ccb_PRE0 = hPRE | PRE0_BPP_16;	
		pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);					
		break;
#endif
	}
	
	/*------------------------------------------------------------------------
	 * return the bytes-per-row.
	 *----------------------------------------------------------------------*/

	return rowBytes;
}

/*****************************************************************************
 * alloc_text_CCB()
 *	Alloc and fill in a CCB, including the cel data buffer.
 ****************************************************************************/

static CCB * alloc_text_CCB(TextCel *tCel, int32 w, int32 h, int32 bpp, int32 *pRowBytes)
{
	CCB *		pCel;
	int32		rowBytes;
	
	/*------------------------------------------------------------------------
	 * allocate a CCB with all fields pre-inited to zeroes.
	 * all our cels have PLUTs attached; for efficiency, we allocate a 
	 * single chunk of memory big enough for a full CCB and PLUT, and split
	 * it up ourselves with some pointer math below.
	 *----------------------------------------------------------------------*/

#if DELETECELMAGIC_SUPPORT
	pCel = AllocMagicCel_(PLUTSIZE, DELETECELMAGIC_CALLBACK, (void*)delete_textcel_callback, tCel);
#else
	pCel = (CCB *)Malloc(sizeof(CCB)+PLUTSIZE, MEMTYPE_CEL | MEMTYPE_FILL | 0x00);
#endif

	if (pCel == NULL) {
		DIAGNOSE(("Can't allocate CCB for text rendering.\n"));
		return NULL;
	}
 
	/*------------------------------------------------------------------------
	 * Set up the CCB fields that need non-zero values.
	 *----------------------------------------------------------------------*/

	pCel->ccb_PLUTPtr	= AddToPtr(pCel, sizeof(CCB));
	pCel->ccb_HDX		= (1 << 20);
	pCel->ccb_VDY		= (1 << 16);
	pCel->ccb_PIXC 		= PIXC_OPAQUE;
	pCel->ccb_Flags		= 	CCB_LAST 	| CCB_NPABS | CCB_SPABS  | CCB_PPABS  |
							CCB_LDSIZE 	| CCB_LDPRS | CCB_LDPPMP | CCB_LDPLUT |
							CCB_CCBPRE 	| CCB_YOXY 	| CCB_USEAV  | CCB_NOBLK  |
							CCB_ACE		| CCB_ACW 	| CCB_ACCW;

	/*------------------------------------------------------------------------
	 * Go set up the cel data buffer and preamble words based on width/height.
	 *----------------------------------------------------------------------*/

	if ((rowBytes = alloc_text_celdata(pCel, w, h, bpp)) < 0) {
#if DELETECELMAGIC_SUPPORT
		FreeMagicCel_(pCel);
#else
		Free(pCel);
#endif
		return NULL;	// error has already been reported.
	}

	if (pRowBytes) {
		*pRowBytes = rowBytes;
	}

	return pCel;
}

/*****************************************************************************
 * recalc_colors()
 *	calculate PLUT entries for anti-aliasing.  this gets weird...
 *	
 *	when the background color is 0 (transparent) it means that cel pixels
 *	are to be anti-aliased against the existing pixels in the bitmap.  pixel 
 *	values are 0-7 where 0 is transparent, 7 is full-intensity, and values
 *	1 thru 6 indicate a need to blend the character color with the existing
 *	pixel color in proportion to the pixel's value.  we're dealing with 8
 *	bit coded cels, and the AMV value in each cel pixel will be used to 
 *	scale the existing bitmap pixel in the typical AMV way.  the CCB primary
 *	source is set to scale the existing frame buffer pixel by the AMV, and 
 *	the secondary source adds in the PLUT value as indexed by the 5 low-order
 *	bits of the pixel value.  so, when a pixel has a value of 6, the AMV
 *	will be 2 (the AMV is calculated and set by the low-level blit routine
 *	as the pixels are unpacked into the cel buffer).  this means the existing 
 *	frame buffer pixel is scaled to 2/8 its original value, and we have to
 *	add 6/8 of the full intensity pixel color to get a proper blend.  the
 *	(bgColor==0) loop below calculates the PLUT entries such that each
 *	of the slots 1-6 holds the foreground color scaled to 1/8, 2/8, 3/8, etc.
 *	note that the PIXC is set to add the primary and secondary source without
 *	a divide-by-two operation you often find in blending.  this is because
 *	the combo of the AMV and the colors we put in the PLUT already represent
 *	proportional blends of existing and new pixel colors.
 *
 *	when the background color is non-0, it means that cel pixels completely
 *	replace existing pixels in the bitmap.  in this case, we aren't anti-
 *	aliasing against an arbitrary background, we know what the background
 *	color is before we draw the cel.  so, we use a straightforward PIXC
 *	that uses the cel pixels as the primary source and has no secondary
 *	source, and the PLUT values indexed by the pixel values replace existing
 *	pixels in the bitmap when drawn.  AMV is not used at all in this case.
 *	we do have to fill in the PLUT entries with pre-blended colors, however,
 *	so that the foreground/background blends for anti-aliasing are already
 *	implicit in the PLUT entries.  the (bgColor != 0) loop below calculates
 *	the static blends between the foreground and background colors.
 *
 *	pixel values of 0 and 7 are treated specially: they access PMode 1 
 *	instead of PMode 0, and no blending is done; the pixel is either opaque
 *	(for 7) or transparent (for 0).  the PMode bit comes from the high bit
 *	of the indexed PLUT entry, so we set PMode 1 by ORing in 0x8000 for
 *	PLUT entries 0 and 7.  (this really means nothing when bgColor != 0, 
 *	since PMode 0 is also opaque mode.)
 *
 *	When the cel is a TWOCOLOR type (IE, the font is a TWO_IMAGES type, 
 *	typically used for outlined and/or shadowed fonts), a slightly different
 *	logic applies:  Color 0 is the foreground, anti-aliased against existing
 *	pixels; Color 1 is the outline/shadow, anti-aliased against existing
 *	pixels; Color 2 is used when the foreground needs to be anti-aliased
 *	against the outine/shadow color.  Colors 0 and 1 work as described in
 *	the prior paragraphs in terms of transparent/opaque background logic 
 *	and the PIXC word and PLUT calcs.  Color 2 is handled like an opaque
 *	background in terms of PLUT calcs: the PLUT holds pre-calc'd blends 
 *	between colors 0 and 1.  All the PLUT entries for color 2 have the
 *	PMode 1 bit set (0x8000) so that opaque draw logic is used for pixels
 *	that index to color 2 regardless of whether colors 0 and 1 have a 
 *	transparent or opaque draw logic.
 ****************************************************************************/

static void recalc_colors(TextCel *tCel, int32 nColors)
{
	uint16 *thePlut;
	uint32 	bgColor;
	uint32 	fgColor;
	uint32	wkColor;
	uint32	colorIndex;
	uint32	fr, fg, fb;			// foreground color components
	uint32	br, bg, bb;			// background color components
	uint32	fwr, fwg, fwb;		// foreground color work components
	uint32	bwr, bwg, bwb;		// background color work components
	int		i;					// color index loop counter
	uint32	inverse;			// inverse of color index loop counter

	thePlut = (uint16 *)tCel->tc_CCB->ccb_PLUTPtr;

	for (colorIndex = 0; colorIndex < nColors; ++colorIndex) {

		if (colorIndex == 2 && (tCel->tc_formatFlags & TC_INTERNAL_TWOCOLOR)) {
			fgColor = tCel->tc_fgColor[0];	// for 2-color fonts (outlined/shadowed)
			bgColor = tCel->tc_fgColor[1];	// color 2 is anti-a between colors 0 & 1.
		} else {
			fgColor = tCel->tc_fgColor[colorIndex];
			bgColor = tCel->tc_bgColor;
		}
		
		if (bgColor == 0 && fgColor == 0) {	// when bg is transparent, force
			fgColor = MakeRGB15(0,0,1);		// fg color to near-black instead
		}									// of transparent.

		fr = (fgColor >> 10) & 0x1F;
		fg = (fgColor >>  5) & 0x1F;
		fb = (fgColor >>  0) & 0x1F;

		if (bgColor == 0) {
			for (i = 1; i < 8; ++i) {
				fwr = (fr * i) >> 3;
				fwg = (fg * i) >> 3;
				fwb = (fb * i) >> 3;
				wkColor = ((fwr << 10) | (fwg << 5) | fwb);
				thePlut[i-1]  = (uint16) ((wkColor == 0) ? MakeRGB15(0,0,1) : wkColor);
			}
		} else {
			br = (bgColor >> 10) & 0x1F;
			bg = (bgColor >>  5) & 0x1F;
			bb = (bgColor >>  0) & 0x1F;
			for (i = 1; i < 8; ++i) {
				fwr = (fr * i) >> 3;
				fwg = (fg * i) >> 3;
				fwb = (fb * i) >> 3;
				inverse = 8L-i;
				bwr = (br * inverse) >> 3;
				bwg = (bg * inverse) >> 3;
				bwb = (bb * inverse) >> 3;
				thePlut[i-1] = (uint16)( 0x8000U | ((fwr+bwr) << 10) | ((fwg+bwg) << 5) | (fwb+bwb) );						
			}
		}
		
		thePlut[0]	= (uint16)(0x8000U | bgColor);
		thePlut[7] 	= (uint16)(0x8000U | fgColor);
		thePlut += 8;
	}
	
	if (tCel->tc_bgColor == 0) {
		tCel->tc_CCB->ccb_Flags &= ~CCB_BGND;
		tCel->tc_CCB->ccb_PIXC	 = PIXC_8BPP_BLEND;
	} else {
		tCel->tc_CCB->ccb_Flags |= CCB_BGND;
		tCel->tc_CCB->ccb_PIXC	 = PIXC_OPAQUE;
	}

}

/*****************************************************************************
 * vformat_text()
 *	pass the text and related args through sprintf(), if the cel has a 
 *	format buffer attached to it.  return a pointer to the resulting text.
 ****************************************************************************/

static char * vformat_text(TextCel *tCel, char *fmtString, va_list fmtArgs)
{
	int32		formattedSize;
	
	if (tCel->tc_formatBuffer == NULL) {
		return fmtString;
	}
	
	formattedSize = vsprintf(tCel->tc_formatBuffer, fmtString, fmtArgs);
	
#if DEBUG
	if (formattedSize > tCel->tc_formatBufferSize) {
		DIAGNOSE(("Formatted text exceeded buffer size, memory is now corrupted!\n"));
	}
#endif

	return tCel->tc_formatBuffer;
	
}

/*****************************************************************************
 * offset_to_tabstop()
 *	returns the offset from the current X position to the next tabstop.  if
 *	there are no tabstops, or the current X is greater than the last tabstop,
 *	returns 0.
 ****************************************************************************/

static int32 offset_to_tabstop(TextCel *tCel, int32 curX)
{
	uint16 * curStop;
	
	curStop = tCel->tc_tabStops;
	while (*curStop && curX >= *curStop) {
		++curStop;
	}

	if (*curStop) {
		return (*curStop - curX);
	} else {
		return 0;
	}
}

/*****************************************************************************
 * get_line_chars()
 *	calculates the maximum number of words that can fit within the boxWidth
 *	parameter.  returns the number of characters which make up the maximum
 *	number of words.
 ****************************************************************************/

static int32 get_line_chars(FontDescriptor *fd, TextCel *tCel, char **lineStart, int32 *textLeft, int32 *textWidth, Boolean *firstTimeThru, int32 boxWidth) 
{
	int32		wordChars;
	int32		wordWidth;
	int32		widthLeft;
	int32		charSpacing;
	int32		numendingword;
	int32		celWidth;
	Boolean		wrappingOn;
	Boolean		roomLeft		= TRUE;
	Boolean		startofline		= TRUE;
	int32		lineChars		= 0;
	int32		lastCharWidth	= 0;
	char *		curChar;
	
	curChar			= *lineStart;
	charSpacing 	= fd->fd_charExtra + tCel->tc_fontAdjustSpacing;
	*textWidth		= 0;
	wrappingOn		= (tCel->tc_formatFlags & TC_FORMAT_WORDWRAP) && !(tCel->tc_formatFlags & TC_INTERNAL_AUTOSIZE_WIDTH);
	
	/*------------------------------------------------------------------------
	 * loop thru all available characters or until no more space is
	 *	available for this line of text.
	 *----------------------------------------------------------------------*/

	if (wrappingOn) {
		widthLeft	= boxWidth + charSpacing;
		celWidth	= tCel->tc_CCB->ccb_Width - (2 * tCel->tc_leftMargin) + charSpacing;
	} else {
		widthLeft = celWidth = 0xffff;
	}
	
	while ((*textLeft > 0) && roomLeft && (*curChar != '\n' && *curChar != '\r')) {
		wordWidth = 0;
		wordChars = 0;
		numendingword = 0;
		
		if (wrappingOn) {

			/*------------------------------------------------------------------------
			 * check for space characters between words and at the very beginning
			 *	 of the entire block of text.
			 *----------------------------------------------------------------------*/
	
			while ((*textLeft > 0) && (*curChar == ' ')) {
				if (!startofline || *firstTimeThru) {
					wordWidth += (GetFontCharWidth(fd, ' ') + charSpacing);
					wordChars++;
					numendingword++;
				} else {
					(*lineStart)++;
				}
				
				curChar++;
				(*textLeft)--;
			}
		}
		
		/*------------------------------------------------------------------------
		 * when word-wrapping is on, calculate the width of each word and
		 *	check to see if it fits on this line.  when word-wrapping is off
		 *	do the same thing, but with each individual character.
		 *----------------------------------------------------------------------*/

		startofline = FALSE;
		*firstTimeThru = FALSE;
		
		if (*curChar != '\n' && *curChar != '\r') {
			if (*curChar == '\t') {	// a tab is a special-case 'word' all its own (is this is a good idea?)
				(*textLeft)--;
				wordWidth = lastCharWidth = offset_to_tabstop(tCel, *textWidth);
				curChar++;
				wordChars++;
				numendingword++;
			} else {
				while ((*textLeft > 0) && (wordWidth <= widthLeft) && (*curChar != '\n' && *curChar != '\r') &&
						((wrappingOn && !isspace(*curChar)) || (!wrappingOn && wordChars == 0))) {
					(*textLeft)--;
					lastCharWidth = (GetFontCharWidth(fd, *curChar) + charSpacing);
					wordWidth += lastCharWidth;
					curChar++;
					wordChars++;
					numendingword++;
				}
			}
			
			/*------------------------------------------------------------------------
			 * quit when a word width exceeds the available space, otherwise,
			 *	decrement the available space variable and move onto the next word.
			 *----------------------------------------------------------------------*/

			if ((wordWidth > widthLeft)) {
				roomLeft = FALSE;
				
				/*------------------------------------------------------------------------
				 * if the word width is larger than the max. cel width, break the
				 *	word and return only the number of characters that will fit on
				 *	this line.
				 *----------------------------------------------------------------------*/

				if (wordWidth > celWidth) {
					lineChars = wordChars - 1;
					
					if (lineChars < 0)
						lineChars = 0;
						
					if (lineChars <= 1) {
						*textWidth = wordWidth;
					} else {
						*textWidth = wordWidth - lastCharWidth;
					}
					
					(*textLeft)++;
				} else {
					(*textLeft) += numendingword;
				}
			} else {
				widthLeft -= wordWidth;
				*textWidth += wordWidth;
				
				if (widthLeft < 0) {
					roomLeft = FALSE;
					widthLeft = 0;
				}
				lineChars += wordChars;
			}
		}
	}

	if (*textWidth > 0)
		*textWidth -= charSpacing;
		
	return lineChars;
}

/*****************************************************************************
 * recalc_and_render_text_in_cel()
 *	invoke the low-level blit routine for each character in the text.
 *	handles clipping of words and characters that fall (even paritially)
 *	outside the cel width and height boundaries.
 *  
 *	when word-wrapping is enabled, words falling outside the cel width
 *	are wrapped to the next row of text.  when word-wrapping is disabled,
 *	only the characters falling outside the width are clipped.  in
 *	either case though, characters falling below the cel height are clipped.
 *
 *	if the doRendering flag is FALSE, no rendering is performed.  this is
 *	mainly used for calculating the width and height of a cel needed to
 *	contain and display the formatted text.  when maxWidth and maxHeight
 *	aren't NULL, they pass back the calculated width and height values.
 *
 *	returns TRUE if any words were clipped, FALSE if all went well.
 ****************************************************************************/

static int32 recalc_and_render_text_in_cel(TextCel *tCel, Boolean replace, char *formattedText, Boolean doRendering, int32 *maxWidth, int32 *maxHeight)
{
	int32				charWidth;
	int32				charHeight;
	int32				charSpacing;
	int32				lineSpacing;
	int32				bpp;
	int32				celRightEdge;
	int32				celBottomEdge;
	int32				celRowBytes;
	int32				dstX;
	int32				dstY;
	int32				justifyValue;
	int32				justifyType;
	int32				colorIndex;
	int32				boxWidth;
	int32				charCount;
	int32				textLeft;
	int32				lineChars;
	int32				lineCount = 0;
	uint32				theChar = 0;
	int32				currentLineWidth, maxLineWidth;
	char *				lineStart;
	char *				textEnd;
	void *				blitInfo;
	CelData *			celData;
	FontDescriptor *	fd;
	int32				anyClipping = FALSE;
	Boolean				firstTimeThru;
	Boolean				wrappingOn;
	Boolean				autosize_width;
	
	/*------------------------------------------------------------------------
	 * wrapping can only occur if the TC_FORMAT_WORDWRAP flag is set and the
	 *	width of the cel is not being autosized.
	 *----------------------------------------------------------------------*/
	
	autosize_width	= (tCel->tc_formatFlags & TC_INTERNAL_AUTOSIZE_WIDTH) ? TRUE : FALSE;
	wrappingOn		= (tCel->tc_formatFlags & TC_FORMAT_WORDWRAP) && !autosize_width;
	
	/*------------------------------------------------------------------------
	 * clean existing text out of the cel, if the caller so desires.
	 * localize some values used a lot inside the rendering loop.
	 *----------------------------------------------------------------------*/

	if (doRendering) {
		if (replace)
			EraseTextInCel(tCel);

		celRightEdge	= tCel->tc_CCB->ccb_Width - tCel->tc_leftMargin;
		celBottomEdge	= tCel->tc_CCB->ccb_Height;
		celData			= tCel->tc_CCB->ccb_SourcePtr;
		celRowBytes		= tCel->tc_celRowBytes;
		bpp				= (tCel->tc_formatFlags & TC_FORMAT_BPPMASK) >> 16;
		colorIndex		= tCel->tc_penNumber;
	} else {
		/*------------------------------------------------------------------------
		 *	when calculating the width and height of the cell:
		 * 	set the cel width to some huge value when wrapping is off or the
		 *	width is autosized.  this is to ensure that no characters get clipped.
		 *	Also, always set the cel height to some huge number for the same
		 *	reason.
		 *----------------------------------------------------------------------*/

		if (!wrappingOn || autosize_width) {
			celRightEdge = 0xffff;
		} else {
			celRightEdge = tCel->tc_CCB->ccb_Width - tCel->tc_leftMargin;
		}
		
		celBottomEdge = 0xffff;
	}
	
	fd				= tCel->tc_fontDesc;
	charHeight		= fd->fd_charHeight;
	charSpacing		= fd->fd_charExtra + tCel->tc_fontAdjustSpacing;
	lineSpacing		= fd->fd_leading + tCel->tc_fontAdjustLeading;
	justifyType		= tCel->tc_formatFlags & TC_FORMAT_JUSTIFY_MASK;

	/*------------------------------------------------------------------------
	 * calc clipping, and render characters which fall wholly within the
	 *	cel boundaries.  when rendering work off the last x and y position
	 *	of the pen.  when calculating the cel width and height, reset the
	 *	pen position and set the boxWidth to the entire cel width.
	 *----------------------------------------------------------------------*/

	if (doRendering) {
		dstY = tCel->tc_YPosInCel;
		
		if (justifyType == TC_FORMAT_LEFT_JUSTIFY) {
			dstX		= tCel->tc_XPosInCel;
			boxWidth	= celRightEdge - tCel->tc_XPosInCel;
		} else {
			dstX		= tCel->tc_leftMargin;
			boxWidth	= celRightEdge - tCel->tc_leftMargin;
		}
			
	} else {
		dstX		= tCel->tc_leftMargin;
		dstY		= tCel->tc_topMargin;
		boxWidth	= celRightEdge - tCel->tc_leftMargin;
	}
	
	lineStart	= formattedText;
	textLeft	= strlen( formattedText );
	textEnd 	= lineStart + textLeft;

	/*------------------------------------------------------------------------
	 * set the firstTimeThru flag to TRUE to handle any spaces at the
	 *	beginning of a block of text. 
	 *----------------------------------------------------------------------*/
	
	firstTimeThru = TRUE;
	maxLineWidth = 0;
	
	do {
		/*------------------------------------------------------------------------
		 * check how many characters can fit within the box width,
		 *	then render them if the doRendering flag is TRUE
		 *----------------------------------------------------------------------*/

		lineChars = get_line_chars(fd, tCel, &lineStart, &textLeft, &currentLineWidth, &firstTimeThru, boxWidth);
		
		if (lineChars != 0 || (*lineStart == '\n' || *lineStart == '\r')) {
			lineCount++;
		}
		
		if (justifyType == TC_FORMAT_RIGHT_JUSTIFY) {
			justifyValue = boxWidth - currentLineWidth;
		} else if (justifyType == TC_FORMAT_CENTER_JUSTIFY) {
			justifyValue = (boxWidth - currentLineWidth) >> 1;
		} else {
			justifyValue = 0;
		}
		
		for (charCount = 0; charCount < lineChars; ++charCount) {
			theChar = *(lineStart + charCount);
			if (theChar == '\t') {
				dstX += offset_to_tabstop(tCel, dstX); // might not work with right/center justify???
			} else {
				charWidth = GetFontCharInfo(fd, theChar, &blitInfo);
				if (doRendering && charWidth > 0) {
					if ((dstX + justifyValue + charWidth) <= celRightEdge &&
						(dstX + justifyValue) >= tCel->tc_leftMargin &&
						(dstY + charHeight) <= celBottomEdge) {
						BlitFontChar(fd, theChar, blitInfo, celData, dstX + justifyValue, dstY, celRowBytes, colorIndex, bpp);
					} else {
						anyClipping = TRUE;
					}
				}
				dstX += charWidth + charSpacing;
			}
		}	
		
		if (currentLineWidth > maxLineWidth)
			maxLineWidth = currentLineWidth;
				
		/*------------------------------------------------------------------------
		 * after rendering the first line based off the pen x-position, reset 
	 	 *	the box width to the max. cel width.
		 *----------------------------------------------------------------------*/

		boxWidth = celRightEdge - tCel->tc_leftMargin;
		lineStart += lineChars;
			
		if (*lineStart == '\n' || *lineStart == '\r') {
			lineStart++;
			textLeft--;
			firstTimeThru = TRUE;
			
			/*------------------------------------------------------------------------
			 * update the  pen's x and y position if a newLine is encountered when
			 *	(word-wrapping is disabled) or when (word-wrapping is enabled and
			 *	the last character in the text is a newLine.
			 *----------------------------------------------------------------------*/
		
			if ((!wrappingOn) || (wrappingOn && lineStart == textEnd)) {
				dstX = tCel->tc_leftMargin;
				dstY += (charHeight + lineSpacing);
			}
		}
		
		/*------------------------------------------------------------------------
		 * update the pen's position after rendering a line of text when
		 *	word-wrapping is enabled.
		 *----------------------------------------------------------------------*/

		if (lineStart < textEnd && wrappingOn) {
			dstX = tCel->tc_leftMargin;
			dstY += (charHeight + lineSpacing);
		}
		
	} while ((lineStart < textEnd) && (dstY <= celBottomEdge));
	
	if (doRendering) {
		tCel->tc_XPosInCel = dstX;	// update the ending 'pen' position in case the caller
		tCel->tc_YPosInCel = dstY;	// wants to add more chars following this text.
	}
	
	/*------------------------------------------------------------------------
	 * calculate the max width and height necessary to contain the text.
	 *----------------------------------------------------------------------*/

	if (maxWidth) {
		if (maxLineWidth >= charSpacing) {
			*maxWidth = maxLineWidth;
		} else {
			*maxWidth = 0;
		}
	}
	
	if (maxHeight) {
		if (lineCount > 0) {
			*maxHeight = ((charHeight + lineSpacing) * lineCount) - lineSpacing;
		} else {
			*maxHeight = 0;
		}
	}
	
	return anyClipping;
}

/*----------------------------------------------------------------------------
 * public API begins here...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * DetachTextCelCCB()
 *	Disconnect the CCB from a TextCel.  Delete the TextCel stuff but leave
 *	the CCB and pixels intact.  Return the CCB.  Of course, once you've done
 *	this, you can draw the cel and do MapCel() and whatnot, but you can't
 *	do any TextLib-ish things to it anymore.  After doing this, you can safely
 *	unload the font used to build the TextCel.
 ****************************************************************************/

#if DELETECELMAGIC_SUPPORT // can only do this if we have a way to delete the detached resources

CCB * DetachTextCelCCB(TextCel *tCel)
{
	CCB *	cel = NULL;
	
	if (tCel != NULL) {
		cel = tCel->tc_CCB;
		if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF) {
			Free(tCel->tc_formatBuffer);
		}
		Free(tCel);
		ModifyMagicCel_(cel, DELETECELMAGIC_CCB_AND_DATA, cel->ccb_SourcePtr, NULL);
	}
	
	return cel;
}

#endif

/*****************************************************************************
 * DeleteTextCel()
 *	Delete a TextCel and any resources attached to it that we allocated.
 ****************************************************************************/

void DeleteTextCel(TextCel *tCel)
{
	if (tCel != NULL) {
		if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF) {
			Free(tCel->tc_formatBuffer);
		}
		if (tCel->tc_CCB != NULL) {
			if (tCel->tc_CCB->ccb_SourcePtr != NULL) {
				Free(tCel->tc_CCB->ccb_SourcePtr);
			}
#if DELETECELMAGIC_SUPPORT
			FreeMagicCel_(tCel->tc_CCB);
#else
			Free(tCel->tc_CCB);
#endif
		}
		Free(tCel);
	}
}

/*****************************************************************************
 * CreateTextCel()
 *	Create a TextCel and the basic resources it needs.
 *	Set the TC_FORMAT_WORDWRAP formatFlags to enable word-wrapping.
 ****************************************************************************/

TextCel * CreateTextCel(FontDescriptor *fDesc, uint32 formatFlags,
							int32 width, int32 height)
{
	int32		bpp;
	int32		rowBytes;
	TextCel *	tCel = NULL;
	
	/*------------------------------------------------------------------------
	 * validate parms.
	 *----------------------------------------------------------------------*/

	formatFlags &= TC_FORMAT_FLAGSMASK;	// make sure only client flags are present

	if (fDesc == NULL) {
		DIAGNOSE(("Invalid NULL FontDescriptor parm\n"));
		goto ERROR_EXIT;
	}
	
	if (width == 0) {
		width = fDesc->fd_charWidth;
		formatFlags |= TC_INTERNAL_AUTOSIZE_WIDTH;
	}
	
	if (height == 0) {
		height = fDesc->fd_charHeight;
		formatFlags |= TC_INTERNAL_AUTOSIZE_HEIGHT;
	}

	if (fDesc->fd_fontFlags & FFLAG_TWOCOLOR) {
		formatFlags |= TC_INTERNAL_TWOCOLOR;
	}

	bpp = ((formatFlags |= TC_INTERNAL_8BPPCEL) & TC_FORMAT_BPPMASK) >> 16;

	/*------------------------------------------------------------------------
	 * create the TextCel structure, the CCB and CelData, and fill in the 
	 * rest of the TextCel fields with default values.
	 *----------------------------------------------------------------------*/

	tCel = (TextCel *)Malloc(sizeof(TextCel), MEMTYPE_ANY|MEMTYPE_FILL|0);
	if (tCel == NULL) {
		DIAGNOSE(("Can't allocate TextCel\n"));
		goto ERROR_EXIT;
	}
	
	tCel->tc_CCB = alloc_text_CCB(tCel, width, height, bpp, &rowBytes);
	if (tCel->tc_CCB == NULL) {
		goto ERROR_EXIT; // error already reported by alloc_text_CCB
	}

	tCel->tc_fontDesc		= fDesc;
	tCel->tc_formatFlags	= formatFlags;
	tCel->tc_celRowBytes	= rowBytes;
	tCel->tc_fgColor[0]		= MakeRGB15(31,31,31);
	
	if (tCel->tc_formatFlags & TC_INTERNAL_TWOCOLOR) {
		tCel->tc_fgColor[1]	= MakeRGB15( 0, 0, 1);
	}
	
	recalc_colors(tCel, (tCel->tc_formatFlags & TC_INTERNAL_TWOCOLOR) ? 3 : 1);
	
	return tCel;

ERROR_EXIT:

	DeleteTextCel(tCel);
	return NULL;
}

/*****************************************************************************
 * CloneTextCel()
 *	Create a new TextCel using an existing one as a template.
 ****************************************************************************/

TextCel * CloneTextCel(TextCel *tCel, Boolean clonePixels)
{
	TextCel	*	newCel;
	char *		formatBuffer;

	/*------------------------------------------------------------------------
	 * go create a new TextCel using the width/height/flags of the template.
	 *----------------------------------------------------------------------*/
	
	newCel = CreateTextCel(tCel->tc_fontDesc, tCel->tc_formatFlags,
				tCel->tc_CCB->ccb_Width, tCel->tc_CCB->ccb_Height);
	if (newCel == NULL) {
		return NULL;
	}

	/*------------------------------------------------------------------------
	 * copy from the template to the new cel those things which CreateTextCel()
	 * gave zero/default values to.
	 *----------------------------------------------------------------------*/

	newCel->tc_formatFlags		 = tCel->tc_formatFlags;
	newCel->tc_fontAdjustSpacing = tCel->tc_fontAdjustSpacing;
	newCel->tc_fontAdjustLeading = tCel->tc_fontAdjustLeading;
	newCel->tc_leftMargin		 = tCel->tc_leftMargin;
	newCel->tc_topMargin		 = tCel->tc_topMargin;
	newCel->tc_bgColor 			 = tCel->tc_bgColor;
	memcpy(newCel->tc_fgColor, tCel->tc_fgColor, sizeof(newCel->tc_fgColor));
	memcpy(newCel->tc_tabStops, tCel->tc_tabStops, sizeof(newCel->tc_tabStops));
	
	recalc_colors(newCel, 4);
	
	/*------------------------------------------------------------------------
	 * if the template cel has a dynamic format buffer, allocate one for the
	 * new cel.  if the template has a static buffer the new cel inherits it.
	 *----------------------------------------------------------------------*/
	
	if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF) {
		formatBuffer = NULL;					// ask SetBuffer to allocate one
	} else {
		formatBuffer = tCel->tc_formatBuffer;	// attach same buffer to new cel
	}

	if (SetTextCelFormatBuffer(newCel, formatBuffer, tCel->tc_formatBufferSize) < 0) {
		DeleteTextCel(newCel);
		return NULL;
	}
	
	/*------------------------------------------------------------------------
	 * if asked to clone the pixels, do that now.
	 *----------------------------------------------------------------------*/

	if (clonePixels) {
		memcpy(newCel->tc_CCB->ccb_SourcePtr, tCel->tc_CCB->ccb_SourcePtr, 
				MemBlockSize(newCel->tc_CCB->ccb_SourcePtr));
	}

	return newCel;

}

/*****************************************************************************
 * GetTextCelSpacingAdjust()
 *	Retrieve the spacing delta for the cel.
 ****************************************************************************/

void GetTextCelSpacingAdjust(TextCel *tCel, int32 *adjustSpacing)
{
	if (adjustSpacing) {
		*adjustSpacing = tCel->tc_fontAdjustSpacing;
	}
}

/*****************************************************************************
 * GetTextCelLeadingAdjust()
 *	Retrieve the leading delta for the cel.
 ****************************************************************************/

void GetTextCelLeadingAdjust(TextCel *tCel, int32 *adjustLeading)
{
	if (adjustLeading) {
		*adjustLeading = tCel->tc_fontAdjustLeading;
	}
}

/*****************************************************************************
 * GetTextCelColor()
 *	Retrieve the background/foreground colors for the cel.
 ****************************************************************************/

void GetTextCelColor(TextCel *tCel, int32 *bgColor, int32 *fgColor0)
{
	if (bgColor) {
		*bgColor = tCel->tc_bgColor;
	}
	
	if (fgColor0) {
		*fgColor0 = tCel->tc_fgColor[0];
	}
}

/*****************************************************************************
 * GetTextCelColors()
 *	Retrieve the background and all foreground colors for the cel.
 ****************************************************************************/

void GetTextCelColors(TextCel *tCel, int32 *bgColor, int32 fgColors[4])
{
	if (bgColor) {
		*bgColor = tCel->tc_bgColor;
	}
	
	if (fgColors) {
		memcpy(fgColors, tCel->tc_fgColor, sizeof(tCel->tc_fgColor));
	}
}

/*****************************************************************************
 * GetTextCelCoords()
 *	Retrieve the cel's CCB X/Y coords.
 ****************************************************************************/

void GetTextCelCoords(TextCel *tCel, Coord *ccbX, Coord *ccbY)
{
	if (ccbX) {
		*ccbX = tCel->tc_CCB->ccb_XPos;
	}
	
	if (ccbY) {
		*ccbY = tCel->tc_CCB->ccb_YPos;
	}
}

/*****************************************************************************
 * GetTextCelMargins()
 *	Retrieve the margins for the cel.
 ****************************************************************************/

void GetTextCelMargins(TextCel *tCel, int32 *leftMargin, int32 *topMargin)
{
	if (leftMargin) {
		*leftMargin = tCel->tc_leftMargin;
	}
	
	if (topMargin) {
		*topMargin = tCel->tc_topMargin;
	}
}

/*****************************************************************************
 * GetTextCelPenNumber()
 *	Retrieve the current pen number for the cel.
 ****************************************************************************/

void GetTextCelPenNumber(TextCel *tCel, int32 *penNumber)
{
	if (penNumber) {
		*penNumber = tCel->tc_penNumber;
	}
}

/*****************************************************************************
 * GetTextCelFormatFlags()
 *	Retrieve the client format flags for the cel.
 *	returns the format flags as well.
 ****************************************************************************/

uint32 GetTextCelFormatFlags(TextCel *tCel, uint32 *formatFlags)
{
	uint32	flags;
	
	flags = tCel->tc_formatFlags & TC_FORMAT_FLAGSMASK;
	
	if (formatFlags) {
		*formatFlags = flags;
	}
	
	return flags;
}

/*****************************************************************************
 * GetTextCelSize()
 *	Retrieve the width and/or height for the cel.
 ****************************************************************************/

void GetTextCelSize(TextCel *tCel, int32 *width, int32 *height)
{	
	if (width) {
		*width = tCel->tc_CCB->ccb_Width;
	}
	
	if (height) {
		*height = tCel->tc_CCB->ccb_Height;
	}
}

/*****************************************************************************
 * GetTextCelFormatBuffer()
 *	Retrieve the format buffer information for the cel.
 ****************************************************************************/

void GetTextCelFormatBuffer(TextCel *tCel, char **buffer, uint32 *bufsize)
{
	if (buffer) {
		*buffer = tCel->tc_formatBuffer;
	}
	
	if (bufsize) {
		*bufsize = tCel->tc_formatBufferSize;
	}
}

/*****************************************************************************
 * GetTextCelTabStops()
 *	Retrieve the set of tab stops for the cel.
 ****************************************************************************/

void GetTextCelTabStops(TextCel *tCel, uint16 tabStops[16])
{
	if (tabStops) {
		memcpy(tabStops, tCel->tc_tabStops, sizeof(tabStops));
	}
}

/*****************************************************************************
 * SetTextCelSpacingAdjust()
 *	Store the provided spacing delta in our private field.
 ****************************************************************************/

void SetTextCelSpacingAdjust(TextCel *tCel, int32 adjustSpacing)
{
	tCel->tc_fontAdjustSpacing = adjustSpacing;
}

/*****************************************************************************
 * SetTextCelLeadingAdjust()
 *	Store the provided leading delta in our private field.
 ****************************************************************************/

void SetTextCelLeadingAdjust(TextCel *tCel, int32 adjustLeading)
{
	tCel->tc_fontAdjustLeading = adjustLeading;
}

/*****************************************************************************
 * SetTextCelColor()
 *	Store the provided background/foreground colors in our private fields.
 *	This one sets just the pen0 color.  If the cel is a TWOCOLOR type, we
 *	have to recalc 3 colors to get all the blending combos even though only
 *	1 color is being changed.
 ****************************************************************************/

void SetTextCelColor(TextCel *tCel, int32 bgColor, int32 fgColor0)
{
	if (bgColor >= 0) {	
		tCel->tc_bgColor = bgColor;
	}
	
	if (fgColor0 >= 0) {
		tCel->tc_fgColor[0] = fgColor0;
	}
	
	recalc_colors(tCel, (tCel->tc_formatFlags & TC_INTERNAL_TWOCOLOR) ? 3 : 1);
}

/*****************************************************************************
 * SetTextCelColors()
 *	Store the provided background/foreground colors in our private fields.
 *	This one allows specification of all four possible foreground colors,
 *	instead of just the pen0 color.
 ****************************************************************************/

void SetTextCelColors(TextCel *tCel, int32 bgColor, int32 fgColors[4])
{
	int i;
	
	if (bgColor >= 0) {	
		tCel->tc_bgColor = bgColor;
	}
	
	for (i = 0; i < 4; ++i) {
		if (fgColors[i] >= 0) {
			tCel->tc_fgColor[i] = fgColors[i];
		}
	}

	recalc_colors(tCel, 4);
}

/*****************************************************************************
 * SetTextCelCoords()
 *	Store the provided coords in the cel's CCB.
 ****************************************************************************/

void SetTextCelCoords(TextCel *tCel, Coord x, Coord y)
{
	if (x < 1024 && x > -1024 && y < 1024 && y > -1024) {	
		tCel->tc_CCB->ccb_XPos = Convert32_F16(x);
		tCel->tc_CCB->ccb_YPos = Convert32_F16(y);
	} else {
		tCel->tc_CCB->ccb_XPos = x;
		tCel->tc_CCB->ccb_YPos = y;
	}
}

/*****************************************************************************
 * SetTextCelFormatBuffer()
 *	Attach/detach a format buffer to a TextCel.
 *	If buffer size is zero, we detach any existing buffer.
 *	If the cel already has a buffer, and we allocated it, we first free it.
 *	If the buffer pointer is NULL, we allocate a new buffer for the caller;
 *	if non-NULL, we attach the provided buffer to the cel.
 *	Returns zero on success, negative on error.
 ****************************************************************************/

Err	 SetTextCelFormatBuffer(TextCel *tCel, char *buffer, uint32 bufsize)
{
	if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF) {
		tCel->tc_formatBuffer = (char *)Free(tCel->tc_formatBuffer);
		tCel->tc_formatBufferSize = 0;
		tCel->tc_formatFlags &= ~TC_INTERNAL_DYNBUF;
	}

	if (buffer == NULL && bufsize != 0) {
		buffer = (char *)Malloc(bufsize, MEMTYPE_ANY);
		if (buffer == NULL) {
			DIAGNOSE(("Can't allocate text format buffer\n"));
			return -1;
		}
		tCel->tc_formatFlags |= TC_INTERNAL_DYNBUF;
	}
	
	tCel->tc_formatBuffer = buffer;
	tCel->tc_formatBufferSize = bufsize;

	return 0;
}

/*****************************************************************************
 * SetTextCelMargins()
 *	Store the provided margins in our private fields.
 ****************************************************************************/

void SetTextCelMargins(TextCel *tCel, int32 leftMargin, int32 topMargin)
{
	if (leftMargin >= 0) {
		tCel->tc_leftMargin = leftMargin;
	}
	
	if (topMargin >= 0) {
		tCel->tc_topMargin = topMargin;
	}
}

/*****************************************************************************
 * SetTextCelPenNumber()
 *	Change the current pen number to the provided penNumber.
 ****************************************************************************/

void SetTextCelPenNumber(TextCel *tCel, int32 penNumber)
{
	if ((tCel->tc_formatFlags & TC_INTERNAL_TWOCOLOR) && penNumber != 0) {
		DIAGNOSE(("Can't change pen number when TWOCOLOR fonts are in use\n"));
	} else {
		tCel->tc_penNumber = penNumber & 0x03;
	}
}

/*****************************************************************************
 * SetTextCelFormatFlags()
 *	Store the provided client format flags in our private field.
 ****************************************************************************/

void SetTextCelFormatFlags(TextCel *tCel, uint32 formatFlags)
{
	formatFlags &= TC_FORMAT_FLAGSMASK;		// make sure only client flags are present
	tCel->tc_formatFlags = (tCel->tc_formatFlags & TC_INTERNAL_FLAGSMASK) | formatFlags;
}

/*****************************************************************************
 * SetTextCelSize()
 *	Change the size of an existing TextCel.
 *	If the width or height is zero, the cel is set to AUTOSIZE and its size
 *	will automatically change with each chunk of text rendered into it.
 *	Changing the size results in clearing any existing pixels from the cel
 *	and setting the 'pen' position back to the text cel's margins.
 *	Returns positive on success, negative on error.
 ****************************************************************************/

Err	 SetTextCelSize(TextCel *tCel, int32 width, int32 height)
{
	int32	rowBytes;
	
	int32 bpp = (tCel->tc_formatFlags & TC_FORMAT_BPPMASK) >> 16;
	
	if (width < 0) {
		width = tCel->tc_CCB->ccb_Width;
	} else if (width == 0) {
		width = tCel->tc_fontDesc->fd_charWidth;
		tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE_WIDTH;
	} else {
		tCel->tc_formatFlags &= ~TC_INTERNAL_AUTOSIZE_WIDTH;
	}
	
	if (height < 0) {
		height = tCel->tc_CCB->ccb_Height;
	} else if (height == 0) {
		height = tCel->tc_fontDesc->fd_charHeight;
		tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE_HEIGHT;
	} else {
		tCel->tc_formatFlags &= ~TC_INTERNAL_AUTOSIZE_HEIGHT;
	}
	
	rowBytes = alloc_text_celdata(tCel->tc_CCB, width, height, bpp);	
	if (rowBytes > 0) {
		tCel->tc_celRowBytes = rowBytes;
		EraseTextInCel(tCel);
	}

	return rowBytes;
}

/*****************************************************************************
 * SetTextCelTabStops()
 *	Specify a new set of tabstop X positions for the text cel.
 *	The caller has the option of passing either a pointer to an array of 16
 *	stop locations (of which only 15 count; the last must be zero), or 
 *	passing a NULL array pointer followed by a comma-delimited list of stop
 *	locations (of which at most 15 are used) with a zero indicating the end
 *	of the list.  (EG, SetTextCelTabStops(myCel, NULL, 5, 10, 15, 20, 0);)
 ****************************************************************************/

void SetTextCelTabStops(TextCel *tCel, uint16 tabStops[16], ...)
{
	int		i;
	int		thisStop;	// this must be int (not int32) due to va_arg rules!
	va_list args;
	
	if (tabStops != NULL) {
		memcpy(tCel->tc_tabStops, tabStops, sizeof(tCel->tc_tabStops));
	} else {
		va_start(args, tabStops);
		for (i = 0; i < ArrayElements(tCel->tc_tabStops); ++i) {
			thisStop = va_arg(args, int);
			tCel->tc_tabStops[i] = (uint16)thisStop;
			if (thisStop == 0) {
				break;
			}
		}
		va_end(args);
	}

	tCel->tc_tabStops[15] = 0;
}

/*****************************************************************************
 * EraseTextInCel()
 *	Clear existing pixels from a cel, and set the 'pen' back to 0,0.
 ****************************************************************************/

void EraseTextInCel(TextCel *tCel)
{
	memset(tCel->tc_CCB->ccb_SourcePtr, 0, MemBlockSize(tCel->tc_CCB->ccb_SourcePtr));
	tCel->tc_XPosInCel = tCel->tc_leftMargin;
	tCel->tc_YPosInCel = tCel->tc_topMargin;
}

/*****************************************************************************
 * vUpdateTextInCel()
 *	Render text into cel, optionally clearing existing pixels first.
 ****************************************************************************/

Err	 vUpdateTextInCel(TextCel *tCel, Boolean replaceExisting, 
						char *fmtString, va_list fmtArgs)
{
	char *		formattedText;
	int32		bpp;
	int32		celRowBytes;
	int32		width, height;
	int32		calcWidth, calcHeight;
	Boolean		autosize_width, autosize_height;

	autosize_width = (tCel->tc_formatFlags & TC_INTERNAL_AUTOSIZE_WIDTH) ? TRUE : FALSE;
	autosize_height = (tCel->tc_formatFlags & TC_INTERNAL_AUTOSIZE_HEIGHT) ? TRUE : FALSE;
				
	if (replaceExisting && (autosize_width || autosize_height)) {
		formattedText = vGetTextExtent(tCel, &calcWidth, &calcHeight, fmtString, fmtArgs);
		bpp = (tCel->tc_formatFlags & TC_FORMAT_BPPMASK) >> 16;
		
		width = autosize_width ? calcWidth : tCel->tc_CCB->ccb_Width;
		height = autosize_height ? calcHeight : tCel->tc_CCB->ccb_Height;

		if ((celRowBytes = SetTextCelSize(tCel, width, height)) < 0) {
			return celRowBytes; // error already reported
		}
		
		if (autosize_width)
			tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE_WIDTH; // force this back on
		if (autosize_height)
			tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE_HEIGHT; // force this back on
	} else {
		formattedText = vformat_text(tCel, fmtString, fmtArgs);
	}
	
	return recalc_and_render_text_in_cel(tCel, replaceExisting, formattedText, TRUE, NULL, NULL);
}

/*****************************************************************************
 * UpdateTextInCel()
 *	Render text into cel, optionally clearing existing pixels first.
 ****************************************************************************/

Err	 UpdateTextInCel(TextCel *tCel, Boolean replaceExisting, char *fmtString, ...)
{
	int32	rv;
	va_list	fmtArgs;
	
	va_start(fmtArgs, fmtString);
	rv = vUpdateTextInCel(tCel, replaceExisting, fmtString, fmtArgs);
	va_end(fmtArgs);
	return rv;
}

/*****************************************************************************
 * vGetTextExtent()
 *	Calculate and return the width and height needed to display some text.
 *	Returns width and/or height via the provided pointers.
 *	Returns pointer to the formatted text (results of optional sprintf 
 *	processing).
 ****************************************************************************/

char * vGetTextExtent(TextCel *tCel, int32 *pWidth, int32 *pHeight, 
						char *fmtString, va_list fmtArgs)
{
	char *	formattedText;

	/*------------------------------------------------------------------------
	 * format the text, and calc the adjusted char and line spacing.
	 *----------------------------------------------------------------------*/
	
	formattedText = vformat_text(tCel, fmtString, fmtArgs);
	recalc_and_render_text_in_cel(tCel, TRUE, formattedText, FALSE, pWidth, pHeight);
	
	*pWidth += (2 * tCel->tc_leftMargin);
	*pHeight += (2 * tCel->tc_topMargin);
	
	return formattedText;
}

/*****************************************************************************
 * GetTextExtent()
 *	Calculate and return the width and height needed to display some text.
 *	Returns width and/or height via the provided pointers.
 *	Returns pointer to the formatted text (results of optional sprintf 
 *	processing).
 ****************************************************************************/

char * GetTextExtent(TextCel *tCel, int32 *pWidth, int32 *pHeight, char *fmtString, ...)
{
	char *	rv;
	va_list fmtArgs;
	
	va_start(fmtArgs, fmtString);
	rv = vGetTextExtent(tCel, pWidth, pHeight, fmtString, fmtArgs);
	va_end(fmtArgs);
	return rv;
}

/*****************************************************************************
 * DrawTextString()
 *	Render formatted text directly to the specified bitmap.
 ****************************************************************************/
 
void DrawTextString(FontDescriptor *fd, GrafCon *gcon, Item bitmapItem, char *text, ... )
{
	TextCel * 	tCel;
	va_list		args;	

	va_start(args, text);

	tCel = CreateTextCel(fd, 0, 0, 0);
	if (SetTextCelFormatBuffer(tCel, NULL, 1024) < 0) {
		return; // error has already been reported
	}
	if (vUpdateTextInCel(tCel, TRUE, text, args) < 0) {
		return; // error has already been reported
	}
	SetTextCelColor(tCel, gcon->gc_BGPen, gcon->gc_FGPen);
	SetTextCelCoords(tCel, Convert32_F16(gcon->gc_PenX), Convert32_F16(gcon->gc_PenY));
	
	DrawCels(bitmapItem, tCel->tc_CCB);

	/*------------------------------------------------------------------------
	 * adjust the coords in the GrafCon based on where the text 'pen'
	 *	was left after rendering the characters in the the cel buffer.
	 *----------------------------------------------------------------------*/

	gcon->gc_PenX += tCel->tc_XPosInCel;
	gcon->gc_PenY += tCel->tc_YPosInCel;

	DeleteTextCel(tCel);

	va_end(args);
}

/*****************************************************************************
 * DrawTextChar()
 *	Render a character directly to the specified bitmap.
 ****************************************************************************/

void DrawTextChar(FontDescriptor *fd, GrafCon *gcon, Item bitmapItem, uint32 character)
{
	char	fmtString[2];

	fmtString[0] = (char)character;
	fmtString[1] = 0;
	DrawTextString(fd, gcon, bitmapItem, fmtString);
}





/////////////////////////////////////////////////////////////////////////////////////////////


/*----------------------------------------------------------------------------
 * the following junk would be used to process 4-bit coded cels with 2-pass
 * rendering and 16 levels of anti-aliasing, and also potentially 2-bit
 * coded cels with 4 levels of aa (a concept which has never been tested).
 * there isn't currently any low-level FontLib support for these formats,
 * but I'm not yet ready to throw away this cel-related support code forever.
 *--------------------------------------------------------------------------*/

#if VARIABLE_DESTBPP_IMPLEMENTED
 
#define PMV(x)	   	((x-1) << 2)
#define PDV16		0x0000
#define PDV8		0x0003
#define PDV4		0x0002
#define PDV2		0x0001

#define SCALE16(x)  (((PMV(x)|1) << 10) | ((PMV(x)|1) << 5) | (PMV(x)|1))
#define SCALE8(x)	(((PMV(x)|PDV8) << 10) | ((PMV(x)|PDV8) << 5) | (PMV(x)|PDV8))
#define SCALE4(x)	(((PMV(x)|PDV4) << 10) | ((PMV(x)|PDV4) << 5) | (PMV(x)|PDV4))

static uint16 gScale16PLUT[16] = {
	0, 
	SCALE16(6)|0x8000U,	
	SCALE16(5)|0x8000U,	
	SCALE16(4)|0x8000U,	
	SCALE16(3)|0x8000U,	
	SCALE16(2)|0x8000U,	
	SCALE16(1)|0x8000U,	
	SCALE16(8),			
	SCALE16(7),			
	SCALE16(6),			
	SCALE16(5),			
	SCALE16(4),			
	SCALE16(3),			
	SCALE16(2),			
	SCALE16(1),			
	0
};

static uint16 gScale4PLUT[4] = {
	0, 
	SCALE8(5),			
	SCALE8(3),			
	0
};

	if (nlevels == 4) {		// 4-level special case: scale to 3/8 and 5/8
		wr = (r * 3) >> 3;
		wg = (g * 3) >> 3;
		wb = (b * 3) >> 3;
		thePlut[1] = (uint16)((wr << 10) | (wg << 5) | wb);
		wr = (r * 5) >> 3;
		wg = (g * 5) >> 3;
		wb = (b * 5) >> 3;
		thePlut[2] = (uint16)((wr << 10) | (wg << 5) | wb);
	}



/*****************************************************************************
 * DrawAACel()
 *	no longer needed, but demonstrates PIXCs and stuff for 16-level AA work...
 ****************************************************************************/

void DrawAACel(GrafCon *gcon, Item bitmapItem, uint32 width, uint32 height, CCB *ccb)
{
	uint16		work_plut[32];
	GrafCon		lgcon;
	Rect		bounds;

	/**************************************************************/
	/* Paint a solid color background if the drawMode == srcCopy. */
	/**************************************************************/

	if ((gcon->gc_Flags & f_drawModeMask) == f_srcCopyDrawMode)
	{
		SetFGPen(&lgcon, gcon->gc_BGPen);
		
		bounds.rect_XLeft	= gcon->gc_PenX;
		bounds.rect_YTop	= gcon->gc_PenY;
		bounds.rect_XRight	= bounds.rect_XLeft + width;
		bounds.rect_YBottom	= bounds.rect_YTop + height;
		
		FillRect(bitmapItem, &lgcon, &bounds);
	}

	/**********************************************************/
	/* Pre-scale the existing pixels using the scaling table. */
	/**********************************************************/

	ccb->ccb_PLUTPtr = gScale16PLUT;	// scaling table is static 
	ccb->ccb_PIXC = 0xE090E000;			// scale CFB pixels using mul/div factors from PLUT	
	DrawCels(bitmapItem, ccb);
  
	/*******************************************************/
	/* Now draw the anti-aliased portion of the character. */
	/*******************************************************/
  
	ccb->ccb_PLUTPtr = calc_scaled_PLUT(work_plut, 16, gcon->gc_FGPen);
	ccb->ccb_PIXC = 0x1F001F80;			// pmode0 = (PDC+CFB)/1, pmode1 = (PDC+0)/1 (replace mode)
	DrawCels(bitmapItem, ccb);
}

/*****************************************************************************
 * recalc_colors()
 *	calculate PLUT entries for anti-aliasing.
 ****************************************************************************/

void recalc_colors(TextCel *tCel, int nColors)
{
	int		i;
	int		shiftDivide;
	uint16 *thePlut;
	int32 	nlevels;
	uint32 	bgColor;
	uint32 	fgColor;
	uint32	fr, fg, fb;
	uint32	br, bg, bb;
	uint32	wr, wg, wb;
	uint32	bwr, bwg, bwb;
	uint32	reciprocal;

	(void)nColors; // not currently used

	thePlut = (uint16 *)tCel->tc_CCB->ccb_PLUTPtr;
	bgColor = tCel->tc_bgColor;
	fgColor = tCel->tc_fgColor[0];

	if (tCel->tc_formatFlags & TC_FORMAT_4BPPCEL) {
		nlevels = 16;
		shiftDivide = 4;
	} else {
		nlevels = 8;
		shiftDivide = 3;
	}
	
	br = (bgColor >> 10) & 0x1F;
	bg = (bgColor >>  5) & 0x1F;
	bb = (bgColor >>  0) & 0x1F;

	fr = (fgColor >> 10) & 0x1F;
	fg = (fgColor >>  5) & 0x1F;
	fb = (fgColor >>  0) & 0x1F;

	for (i = 1; i < nlevels; ++i) {
		reciprocal = nlevels-i;
		wr =  ((fr * i) >> shiftDivide);
		wg =  ((fg * i) >> shiftDivide);
		wb =  ((fb * i) >> shiftDivide);
		bwr = ((br * reciprocal) >> shiftDivide);
		bwg = ((bg * reciprocal) >> shiftDivide);
		bwb = ((bb * reciprocal) >> shiftDivide);
		thePlut[i-1] = (uint16)(((wr+bwr) << 10) | ((wg+bwg) << 5) | (wb+bwb));						
	}
	
	if (bgColor == 0) {
		tCel->tc_CCB->ccb_Flags &= ~CCB_BGND;
		tCel->tc_CCB->ccb_PIXC	 = (nlevels == 8) ? PIXC_8BPP_BLEND : PIXC_4BPP_BLEND;
	} else {
		tCel->tc_CCB->ccb_Flags |= CCB_BGND;
		tCel->tc_CCB->ccb_PIXC	 = PIXC_OPAQUE;
	}
	
	thePlut[0]			= (uint16)(0x8000U | bgColor);
	thePlut[nlevels-1] 	= (uint16)(0x8000U | fgColor);

}

#endif	// end of unused varying-bit-depth support junk

/*****************************************************************************
 *	File:		FontLib.c
 *
 *	Contains:	Implementation for low-level handling of 3DO font files.
 *				This file, along with some assembler code, implements low-level font 
 *				handling.  The implementation allows you to load and unload font files,
 *				parse a font file that has been pre-loaded into memory by some other
 *				means, obtain information about the font file and the characters within
 *				it, and unpack/blit individual characters from the font into another
 *				memory location.
 *
 *	Written by:  Edgar Lee and Ian Lepore
 *
 *	Copyright:	© 1993 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *	Change History (most recent first):
 *
 *	  05/08/94	Ian		Added GetFontCharWidest() for finding the widest
 *						character from a set of characters you specify.
 *						Also added GetFontStringWidth() to get the width
 *						of a string as rendered in a given font.
 *
 *	  05/07/94	Ian		Added support for 5bpp TWCOLOR fonts.  This new
 *						type of font is used for outlined and shadowed font
 *						effects.
 *
 *	  03/16/94  Ian		Changed %d to %ld in an error message to clean up a
 *						minor compiler warning.
 *
 *	  12/09/93	Ian		First release version.
 * 
 *	Implementation notes:
 *	
 *	Parts of the low-level font handling are implemented in assembler for
 *	performance.  To 'hide' the global functions names which live in other
 *	modules but shouldn't be directly invoked by the client applications,
 *	we use a trailing underbar on the function names.  The ANSI C standard
 *	reserves names beginning with an underbar for the compiler, so we use
 *	a trailing underbar instead for hiding our own internals.
 *
 *	In an ideal world, a variety of font formats might be supported, with 
 *	varying bit depths, compression schemes, and whatnot.  Any such format-
 *	specific stuff would be hidden by the API this module presents.  
 *	
 *	Right now, we handle a single font-processing concept:  3 bit-per-pixel
 *	font data, compressed in a custom format, and unpacked as needed into a 
 *	buffer which is presumed to be formatted as an 8-bit-coded cel.  If and
 *	when other font storage schemes or cel buffer formats are added, the
 *	support will be hidden from the client by changing the BlitFontChar() 
 *	routine below to recognize the formats and dispatch handling to the
 *	appropriate unpacker/blitter routine.
 ****************************************************************************/

#include "FontLib.h"
#include "BlockFile.h"
#include "UMemory.h"
#include "Debug3DO.h"
#include "Macros3DO.h"

/*----------------------------------------------------------------------------
 * Parameter structures for interfacing to low level pixel blitter functions.
 *	These structures are loaded with values then passed to the assembler
 *	functions that do low-level blitting.  You can't add or change the order
 *	of the fields in these structures without going to the assembler code
 *	and making (perhaps non-trivial) corresponding changes.  The somewhat
 *	arbitrary order of the fields now is purely for the convenience of the
 *	assembler code, which tends to use LDM (load multiple registers) to 
 *	pull all the parameters from the structure into the right registers in
 *	a single operation.
 *--------------------------------------------------------------------------*/

typedef struct FontBlitSrcParms {
	void *				fbs_charPtr;
	int32				fbs_charWidth;
	int32				fbs_charHeight;
	FontDescriptor *	fbs_fontDesc;
	int32				fbs_extraData;
} FontBlitSrcParms;

typedef struct FontBlitDstParms {
	void *				fbd_pixelBuffer;
	int32				fbd_bytesPerRow;
	int32				fbd_X;
	int32				fbd_Y;
	int32				fbd_colorIndex;
} FontBlitDstParms;

extern void FontBlit3To8_(FontBlitSrcParms *, FontBlitDstParms *); 
extern void FontBlit5To8_(FontBlitSrcParms *, FontBlitDstParms *); 

/*****************************************************************************
 * ParseFont()
 *	Parse a font file image that has already been loaded into memory.
 *	Creates a FontDescriptor, and does any other needed work to prepare
 *	the font for use.
 ****************************************************************************/

FontDescriptor * ParseFont(void *fontImage)
{
	FontDescriptor *	fd;
	FontHeader	*		fh;

	fd = NULL;
	fh = (FontHeader *)fontImage;			
	
	if (fh->chunk_ID != CHUNK_FONT || fh->chunk_size < sizeof(FontHeader)) {
		DIAGNOSE(("This is not a valid font file image\n"));
		goto ERROR_EXIT;
	}

	if (fh->fh_bitsPerPixel != 3 && fh->fh_bitsPerPixel != 5) {
		DIAGNOSE(("This font is %ld bits per pixel; can only handle 3 or 5 bpp\n", 
			fh->fh_bitsPerPixel));
		goto ERROR_EXIT;
	}

	if ((fd = (FontDescriptor *)Malloc(sizeof(FontDescriptor), MEMTYPE_ANY)) == NULL) {
		DIAGNOSE(("Can't allocate memory for FontDescriptor\n"));
		goto ERROR_EXIT;
	}

	// fill in the public fields in the FontDescriptor...

	fd->fd_fontFlags	= fh->fh_fontFlags;
	fd->fd_charHeight 	= fh->fh_charHeight;
	fd->fd_charWidth 	= fh->fh_charWidth;
	fd->fd_bitsPerPixel = fh->fh_bitsPerPixel;
	fd->fd_firstChar 	= fh->fh_firstChar;
	fd->fd_lastChar 	= fh->fh_lastChar;
	fd->fd_charExtra 	= fh->fh_charExtra;
	fd->fd_ascent 		= fh->fh_ascent;
	fd->fd_descent 		= fh->fh_descent;
	fd->fd_leading 		= fh->fh_leading;

	// fill in the private fields in the FontDescriptor...

	fd->fd_fontHeader	= fh;
	fd->fd_charInfo		= AddToPtr(fh, fh->fh_charInfoOffset);
	fd->fd_charData		= AddToPtr(fh, fh->fh_charDataOffset);
	
	return fd;
	
ERROR_EXIT:

	if (fd != NULL) {
		Free(fd);
	}
	
	return NULL;
}

/*****************************************************************************
 * UnloadFont()
 *	Release resources acquired during LoadFont() and/or ParseFont().
 *	If we loaded the font file, the FFLAG_DYNLOADED flag will be set, and
 *	we'll unload the file here.
 ****************************************************************************/

void UnloadFont(FontDescriptor *fd)
{
	if (fd != NULL) {
		if ((fd->fd_fontFlags & FFLAG_DYNLOADED) && fd->fd_fontHeader != NULL) {
			UnloadFile(fd->fd_fontHeader);
		}
		Free(fd);
	}
}

/*****************************************************************************
 * LoadFont()
 *	Load a font file and parse it, creating a FontDescriptor for it.
 ****************************************************************************/

FontDescriptor * LoadFont(char *fontFileName, uint32 memTypeBits)
{
	void *				fontFileImage;
	long				fontFileSize;
	FontDescriptor *	fd;
	
	fontFileImage = LoadFile(fontFileName, &fontFileSize, memTypeBits);	
	if (fontFileImage == NULL) {
		DIAGNOSE(("LoadFile(%s) failed for font file\n", fontFileName));
		goto ERROR_EXIT;
 	}
	
	fd = ParseFont(fontFileImage);
	if (fd == NULL) {
		DIAGNOSE(("ParseFont(%s) failed\n", fontFileName));
		goto ERROR_EXIT;
	}
	fd->fd_fontFlags |= FFLAG_DYNLOADED;
	
	return fd;
	
ERROR_EXIT:

	if (fontFileImage != NULL) {
		UnloadFile(fontFileImage);
	}
	
	return NULL;
}

/*****************************************************************************
 * GetFontCharWidest()
 *	Return the width of the widest character in the specified string.  The
 *	return value does NOT include the charExtra (horizontal spacing) value.
 ****************************************************************************/

int32 GetFontCharWidest(FontDescriptor *fd, char *string)
{
	FontCharInfo *	fci;
	uint32			firstChar;
	uint32			lastChar;
	uint32			theChar;
	int32			width;
	int32			widest = 0;
	
	fci			= (FontCharInfo *)fd->fd_charInfo;
	firstChar	= fd->fd_firstChar;
	lastChar 	= fd->fd_lastChar;
	
	while ((theChar = *string++) != 0) {
		if (theChar >= firstChar && theChar <= lastChar) {
			width  = fci[theChar - firstChar].fci_charWidth;
			if (widest < width) {
				widest = width;
			}
		}
	}
	
	return widest;
}

/*****************************************************************************
 * GetFontStringWidth()
 *	Return the pixel width of the specified string as rendered in the
 *	specified font.  The width includes charExtra spacing between each 
 *	character, but not following the last character.  Newlines, tabs, and
 *	other meta-characters are NOT taken into account; they are counted just
 *	like any other character, if a character exists at that location in 
 *	the font. 
 ****************************************************************************/

int32 GetFontStringWidth(FontDescriptor *fd, char *string)
{
	FontCharInfo *	fci;
	uint32			firstChar;
	uint32			lastChar;
	uint32			theChar;
	uint32			charCount = 0;
	int32			width = 0;
	
	fci			= (FontCharInfo *)fd->fd_charInfo;
	firstChar	= fd->fd_firstChar;
	lastChar 	= fd->fd_lastChar;
	
	while ((theChar = *string++) != 0) {
		++charCount;
		if (theChar >= firstChar && theChar <= lastChar) {
			width += fci[theChar - firstChar].fci_charWidth;
		}
	}
	
	if (charCount) {
		width += (charCount - 1) * fd->fd_charExtra;
	}
	
	return width;
}

/*****************************************************************************
 * GetFontCharInfo()
 *	Get width and (optionally) blit-specific information about a character.
 *	The blitinfo is data needed by the blit routine.  A higher-level caller
 *	is likely to obtain width information before blitting a character, so 
 *	this routine can also return info that the blit routine will need.  This
 *	can eliminate needless double calls to this routine (one by the high
 *	level to get the char width, and one by the blit internals to get the
 *	blit info).  
 *	Right now, the blitinfo we return is a pointer to the per-char data for
 *	the char; this lets the blit routine lookup the char width and offset
 *	quickly.  This could change to format-specific data if we start supporting
 *	other font formats; the caller doesn't look at this data at all.
 ****************************************************************************/

int32 GetFontCharInfo(FontDescriptor *fd, int32 theChar, void **blitInfo)
{
	FontCharInfo *	fci;
	uint32			firstChar;
	int32			width = 0;
	
	fci = (FontCharInfo *)fd->fd_charInfo;
	firstChar = fd->fd_firstChar;
	
	if (theChar >= firstChar && theChar <= fd->fd_lastChar) {
		fci   += theChar - firstChar;
		width  = fci->fci_charWidth;
	}

	if (blitInfo) {
		*blitInfo = (void *)fci;
	}

	return width;
}

/*****************************************************************************
 * GetFontCharWidth()
 *	Just what the name says.
 ****************************************************************************/

int32 GetFontCharWidth(FontDescriptor *fd, int32 theChar)
{
	return GetFontCharInfo(fd, theChar, NULL);
}

/*****************************************************************************
 * BlitFontChar()
 *	Blit a character's pixels from the font data area into a memory buffer
 *	specified by the caller.
 *	Right now, we support only 3bpp font data and 8bpp destination buffers.
 *	If we ever support other source/dest formats, this function will serve
 *	as a dispatching routine, calling the proper assembler code to handle
 *	the source-to-dest conversion.
 ****************************************************************************/

int32 BlitFontChar(FontDescriptor *fd, uint32 theChar, void *blitInfo,
					void *dstBuf, int32 dstX, int32 dstY, 
					int32 dstBPR, int32 dstColor, int32 dstBPP)
{
	int32				charWidth;
	FontCharInfo *		fci;
	FontBlitSrcParms	srcParms;
	FontBlitDstParms	dstParms;
	
#if DEBUG
	if (dstBPP != 8 || (fd->fd_bitsPerPixel != 3 && fd->fd_bitsPerPixel != 5)) {
		DIAGNOSE(("Unsupported bits-per-pixel\n"));
		return 0;
	}
#endif
	
	if (blitInfo == NULL) {
		charWidth = GetFontCharInfo(fd, theChar, (void **)&fci);
	} else {
		fci = (FontCharInfo *)blitInfo;
		charWidth = fci->fci_charWidth;
	}
	
	if (charWidth != 0) {
		srcParms.fbs_charPtr	 = AddToPtr(fd->fd_charData, fci->fci_charOffset);
		srcParms.fbs_charWidth	 = charWidth;
		srcParms.fbs_charHeight	 = fd->fd_charHeight;
		dstParms.fbd_pixelBuffer = dstBuf;
		dstParms.fbd_X			 = dstX;
		dstParms.fbd_Y			 = dstY;
		dstParms.fbd_bytesPerRow = dstBPR;
		dstParms.fbd_colorIndex	 = dstColor;
		if (fd->fd_bitsPerPixel == 3) {
			FontBlit3To8_(&srcParms, &dstParms);
		} else {
			FontBlit5To8_(&srcParms, &dstParms);
		}
	}

	return charWidth;
}

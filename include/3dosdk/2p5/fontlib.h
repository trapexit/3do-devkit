#ifndef __FONTLIB_H
#define __FONTLIB_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: fontlib.h,v 1.3 1994/10/05 17:31:13 vertex Exp $
**
**  Lib3DO header file for low-level handling of 3DO font files
**
**  This header file is used with both ARMCC and MPWC; the MPWC use is
**  for the 3DOFontWriter application  If you make changes or additions
**  to this header file, you must recompile 3DOFontWriter as well as any
**  3DO-side libraries and applications using fonts
**
******************************************************************************/


#ifdef __CC_NORCROFT				/* MPWC has its own types.h that conflicts with */
  #include "types.h"				/* the 3DO types.h, so on the 3DO side we use the */
#else								/* header file, and on the MPW side, we just supply */
  typedef long				int32;	/* a few crucial typedefs right here */
  typedef unsigned long		uint32;
  typedef short				int16;
  typedef unsigned short	uint16;
#endif

/*----------------------------------------------------------------------------
 * Font internal datatypes
 *	Client code should not count on these things remaining as they are now.
 *--------------------------------------------------------------------------*/

#ifndef CHAR4LITERAL
  #define CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)
#endif

#define CHUNK_FONT			CHAR4LITERAL('F','O','N','T')

typedef struct FontHeader {
	int32			chunk_ID;					/* Standard 3DO file header fields; */
	int32			chunk_size; 				/* font file is one huge chunk */
	uint32			fh_version;					/* Font header/date version number */
	uint32			fh_fontFlags;				/* Font flags */
	uint32			fh_charHeight;				/* Height of character (ascent+descent) */
	uint32			fh_charWidth;				/* Max width of character (pixels) */
	uint32			fh_bitsPerPixel;			/* Pixel depth of each character, as stored in file */
	uint32			fh_firstChar;				/* First char defined in character set */
	uint32			fh_lastChar;				/* Last char defined in character set */
	uint32			fh_charExtra;				/* Spacing between characters */
	uint32			fh_ascent;					/* Distance from baseline to ascentline */
	uint32			fh_descent;					/* Distance from baseline to descentline */
	uint32			fh_leading;					/* Distance from descent line to next ascent line */
	uint32			fh_charInfoOffset;			/* Offset from file beginning to offset/width table */
	uint32			fh_charInfoSize;			/* Size of offset/width table in bytes */
	uint32			fh_charDataOffset;			/* Offset from file beginning to char data */
	uint32			fh_charDataSize;			/* Size of all character data in bytes */
	uint32			fh_reserved[4];				/* Typical reserved-for-future-expansion */
} FontHeader;

typedef struct FontCharInfo {
	unsigned int	fci_charOffset : 22;		/* Offset from start of char data to this char */
	unsigned int	fci_unused     :  2;		/* two available bits */
	unsigned int	fci_charWidth  :  8;		/* Width (in pixels) of data for this char */
} FontCharInfo;

/*----------------------------------------------------------------------------
 * FontDescriptor
 *	Client code should pull any needed info about a font from this structure.
 *	the fields down thru fd_reserved are g'teed to remain the same forever;
 *	fields from fd_fontHeader down may change in the future.
 *	Client code should never modify any values in this structure.
 *--------------------------------------------------------------------------*/

#define FFLAG_DYNLOADED		0x01000000			/* File loaded via LoadFont (not via ParseFont) */
#define FFLAG_MONOSPACED	0x00000001			/* Font is monospaced (not proportional) */
#define	FFLAG_ITALIC		0x00000002			/* Font typefaces */
#define	FFLAG_BOLD			0x00000004			/* Font typefaces */
#define	FFLAG_OUTLINED		0x00000008			/* Font typefaces */
#define	FFLAG_SHADOWED		0x00000010			/* Font typefaces */
#define	FFLAG_UNDERLINED	0x00000020			/* Font typefaces */
#define	FFLAG_TWOCOLOR		0x00000100			/* Font contains two-color characters (5bpp) */

typedef struct FontDescriptor  {
	uint32			fd_fontFlags;				/* Flags describing the font */
	uint32			fd_charHeight;				/* Height of character (ascent+descent) */
	uint32			fd_charWidth;				/* Max width of character (pixels) */
	uint32			fd_bitsPerPixel;			/* Pixel depth of each character, as stored in file */
	uint32			fd_firstChar;				/* First char defined in character set */
	uint32			fd_lastChar;				/* Last char defined in character set */
	uint32			fd_charExtra;				/* Spacing between characters */
	uint32			fd_ascent;					/* Distance from baseline to ascentline */
	uint32			fd_descent;					/* Distance from baseline to descentline */
	uint32			fd_leading;					/* Distance from descent line to next ascent line */
	uint32			fd_reserved[4];				/* Reserved values from font file header. */
	void *			fd_userData;				/* Client code can store a value here. */
	void *			fd_fontHeader;				/* Font header information */
	void *			fd_charInfo;				/* Per-character data table */
	void *			fd_charData;				/* The character data */
} FontDescriptor;

/*----------------------------------------------------------------------------
 * prototypes...
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------
 * FontBlit API
 *	These things are used to blit character pixels from the font data
 *	area into a cel buffer.
 *--------------------------------------------------------------------------*/

int32	GetFontCharInfo(FontDescriptor *fDesc, int32 character, void **blitInfo);
int32	BlitFontChar(FontDescriptor *fDesc, uint32 theChar, void *blitInfo,
					void *dstBuf, int32 dstX, int32 dstY,
					int32 dstBPR, int32 dstColorIndex, int32 dstBPP);

/*----------------------------------------------------------------------------
 * Font file API
 *--------------------------------------------------------------------------*/

FontDescriptor *	ParseFont(void *fontImage);
FontDescriptor *	LoadFont(char *fontFileName, uint32 memTypeBits);
void				UnloadFont(FontDescriptor * fDesc);

int32	GetFontCharWidth(FontDescriptor *fDesc, int32 character);
int32	GetFontCharWidest(FontDescriptor *fDesc, char *string);
int32	GetFontStringWidth(FontDescriptor *fDesc, char *string);

#ifdef __cplusplus
  }
#endif

#endif	/* __FONTLIB_H */

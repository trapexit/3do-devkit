/*****************************************************************************
 *	File:		TextLib.c
 *
 *	Contains:	Implementation file for handling text rendered via 3DO
 *fonts.
 *
 *	Written by:  Edgar Lee and Ian Lepore
 *
 *	Copyright:	© 1993 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and
 *proprietary information of the 3DO Company and shall not be used by any
 *Person or for any purpose except as expressly authorized in writing by the
 *3DO Company.
 *
 *	Change History (most recent first):
 *
 *	  02/02/94  Edgar	Added a check in alloc_text_celdata() for text
 *cel widths exceding the cel engine limitation of 2047 pixels. Now if a text
 *cel is defined with a width greater than 2047, the width is truncated to
 *2047.
 *
 *	  02/01/94  Ian		Added a check for negative width/height
 *requested in alloc_text_celdata(); that's now an error. Also, removed the
 *logic from alloc_text_celdata() that would try to reallocate the data buffer
 *only if it needed to be bigger than the current one.  Also removed the logic
 *in SetTextCelSize() that would Free() and NULL the celdata buffer to work
 *around the logic in the alloc routine.  It turns out that the workaround was
 *forcing the alloc routine to always realloc the buffer anyway, but the combo
 *of the workaround and the realloc logic was making it so that a failed
 *allocation would leave a CCB with a NULL SourcePtr field (always bad news).
 *
 *	  12/09/93	Ian		First release version.
 *
 *	Implementation notes:
 *
 *	Naming standard:  if it's mixed case, it's exported to the outside
 *	world; if it's lowercase with underbars, it's private to this module.
 *
 *	For a description of how the anti-aliased text logic works (more or
 *less) see the comment block for recalc_colors(), below.
 ****************************************************************************/

#define VARIABLE_DESTBPP_IMPLEMENTED                                          \
  0 // we're not yet supporting varying destination pixel depths

#include "TextLib.h"
#include "Debug3DO.h"
#include "UMemory.h"
#include "operamath.h"
#include "stdio.h"
#include "string.h"

/*----------------------------------------------------------------------------
 * misc internal constants...
 *--------------------------------------------------------------------------*/

#define PLUTSIZE (32 * sizeof (uint16))

#ifndef AddToPtr
#define AddToPtr(ptr, val) ((void *)((((char *)(ptr)) + (val))))
#endif

#define PIXC_OPAQUE 0x1F001F00
#define PIXC_8BPP_BLEND 0x1F00bfc0
#define PIXC_4BPP_BLEND 0xE090E000

#define TC_INTERNAL_DYNBUF                                                    \
  0x01000000 // format buffer dynamically allocated by us
#define TC_INTERNAL_AUTOSIZE                                                  \
  0x02000000 // auto-resize celdata as needed to fit text
#define TC_INTERNAL_FLAGSMASK                                                 \
  0xFFFF0000 // masks off client flags, leaving just internal flags
#define TC_FORMAT_FLAGSMASK                                                   \
  0x0000FFFF // mask off internal flags, leaving just client flags
#define TC_FORMAT_BPPMASK                                                     \
  0x0000000F // masks off non-BPP-related flags, leaving just BPP

/*----------------------------------------------------------------------------
 * the following is an ugly hack to the max...
 *	Right now, we don't have a vsprintf() function but we have to behave as
 *	if we do.  To do so, we call the regular sprintf(), passing the first
 *	10 varargs we received along to it.  If less than 10 args are needed,
 *we just waste some time stacking some extra args; no harm done.  If more than
 *10 args are needed, oh well: we'll probably crash sprintf I guess. The
 *ArgsByValue structure typedef is syntactic sugar for this little scheme.  By
 *taking a regular va_list pointer, casting it to an ArgsByValue pointer, then
 *dereferencing it, we get the effect of putting 10 longwords on the stack
 *without having to name each longword.
 *--------------------------------------------------------------------------*/

typedef struct ArgsByValue
{
  unsigned long argvalues[10];
} ArgsByValue;

/*----------------------------------------------------------------------------
 * internal routines...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * alloc_text_celdata()
 *	Allocate the a cel data buffer based on the width/height/bpp, and set
 *the CCB fields related to width/height/bpp.
 ****************************************************************************/

static int32
alloc_text_celdata (CCB *pCel, int32 w, int32 h, int32 bpp)
{
  int32 rowBytes;
  int32 rowWOFFSET;
  int32 wPRE;
  int32 hPRE;
  CelData *oldCelData;
  CelData *newCelData;
  int32 newCelDataSize;

#if DEBUG
  if (bpp != 8)
    {
      DIAGNOSE (("Currently supporting only 8 bits per pixel text cels\n"));
      return -1;
    }

  if (w < 0 || h < 0)
    {
      DIAGNOSE (("Width (%ld) and Height (%ld) cannot be negative\n", w, h));
      return -1;
    }

  if (w > 2047)
    {
      DIAGNOSE (("Width (%ld) truncated to cel engine limit of 2047\n", w));
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

  if (w == 0)
    {
      w = 1;
    }

  if (h == 0)
    {
      h = 1;
    }

  rowBytes = (((w * bpp) + 31) / 32) * 4;
  if (rowBytes < 8)
    {
      rowBytes = 8;
    }
  rowWOFFSET = (rowBytes / 4) - PRE1_WOFFSET_PREFETCH;

  wPRE = (w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT;
  hPRE = (h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT;

  /*------------------------------------------------------------------------
   * if the cel doesn't already have a data buffer attached to it, or if
   * the new width/height values need a data buffer bigger than the one
   * currently attached, allocate the new buffer and free the old one.
   *----------------------------------------------------------------------*/

  newCelDataSize = h * rowBytes;
  oldCelData = pCel->ccb_SourcePtr;
  newCelData
      = (CelData *)Malloc (newCelDataSize, MEMTYPE_CEL | MEMTYPE_FILL | 0);
  if (newCelData == NULL)
    {
      DIAGNOSE (("Can't allocate memory for text cel data\n"));
      return -2;
    }
  pCel->ccb_SourcePtr = newCelData;
  if (oldCelData != NULL)
    {
      Free (oldCelData);
    }

  /*------------------------------------------------------------------------
   * fill in the CCB width/height/preamble fields.
   *----------------------------------------------------------------------*/

  pCel->ccb_Width = w;
  pCel->ccb_Height = h;

  switch (bpp)
    {
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
      pCel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
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

static CCB *
alloc_text_CCB (int32 w, int32 h, int32 bpp, int32 *pRowBytes)
{
  CCB *pCel;
  int32 rowBytes;

  /*------------------------------------------------------------------------
   * allocate a CCB with all fields pre-inited to zeroes.
   * all our cels have PLUTs attached; for efficiency, we allocate a
   * single chunk of memory big enough for a full CCB and PLUT, and split
   * it up ourselves with some pointer math below.
   *----------------------------------------------------------------------*/

  pCel = (CCB *)Malloc (sizeof (CCB) + PLUTSIZE,
                        MEMTYPE_CEL | MEMTYPE_FILL | 0x00);
  if (pCel == NULL)
    {
      DIAGNOSE (("Can't allocate CCB for text rendering.\n"));
      return NULL;
    }

  /*------------------------------------------------------------------------
   * Set up the CCB fields that need non-zero values.
   *----------------------------------------------------------------------*/

  pCel->ccb_PLUTPtr = AddToPtr (pCel, sizeof (CCB));
  pCel->ccb_HDX = (1 << 20);
  pCel->ccb_VDY = (1 << 16);
  pCel->ccb_PIXC = PIXC_OPAQUE;
  pCel->ccb_Flags = CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE
                    | CCB_LDPRS | CCB_LDPPMP | CCB_LDPLUT | CCB_CCBPRE
                    | CCB_YOXY | CCB_USEAV | CCB_NOBLK | CCB_ACE | CCB_ACW
                    | CCB_ACCW;

  /*------------------------------------------------------------------------
   * Go set up the cel data buffer and preamble words based on width/height.
   *----------------------------------------------------------------------*/

  if ((rowBytes = alloc_text_celdata (pCel, w, h, bpp)) < 0)
    {
      Free (pCel);
      return NULL; // error has already been reported.
    }

  if (pRowBytes)
    {
      *pRowBytes = rowBytes;
    }

  return pCel;
}

/*****************************************************************************
 * recalc_colors()
 *	calculate PLUT entries for anti-aliasing.  this gets weird...
 *
 *	when the background color is 0 (transparent) it means that cel pixels
 *	are to be anti-aliased against the existing pixels in the bitmap. pixel
 *	values are 0-7 where 0 is transparent, 7 is full-intensity, and values
 *	1 thru 6 indicate a need to blend the character color with the existing
 *	pixel color in proportion to the pixel's value.  we're dealing with 8
 *	bit coded cels, and the AMV value in each cel pixel will be used to
 *	scale the existing bitmap pixel in the typical AMV way.  the CCB
 *primary source is set to scale the existing frame buffer pixel by the AMV,
 *and the secondary source adds in the PLUT value as indexed by the 5 low-order
 *	bits of the pixel value.  so, when a pixel has a value of 6, the AMV
 *	will be 2 (the AMV is calculated and set by the low-level blit routine
 *	as the pixels are unpacked into the cel buffer).  this means the
 *existing frame buffer pixel is scaled to 2/8 its original value, and we have
 *to add 6/8 of the full intensity pixel color to get a proper blend.  the
 *	(bgColor==0) loop below calculates the PLUT entries such that each
 *	of the slots 1-6 holds the foreground color scaled to 1/8, 2/8, 3/8,
 *etc. note that the PIXC is set to add the primary and secondary source
 *without a divide-by-two operation you often find in blending.  this is
 *because the combo of the AMV and the colors we put in the PLUT already
 *represent proportional blends of existing and new pixel colors.
 *
 *	when the background color is non-0, it means that cel pixels completely
 *	replace existing pixels in the bitmap.  in this case, we aren't anti-
 *	aliasing against an arbitrary background, we know what the background
 *	color is before we draw the cel.  so, we use a straightforward PIXC
 *	that uses the cel pixels as the primary source and has no secondary
 *	source, and the PLUT values indexed by the pixel values replace
 *existing pixels in the bitmap when drawn.  AMV is not used at all in this
 *case. we do have to fill in the PLUT entries with pre-blended colors,
 *however, so that the foreground/background blends for anti-aliasing are
 *already implicit in the PLUT entries.  the (bgColor != 0) loop below
 *calculates the static blends between the foreground and background colors.
 *
 *	pixel values of 0 and 7 are treated specially: they access PMode 1
 *	instead of PMode 0, and no blending is done; the pixel is either opaque
 *	(for 7) or transparent (for 0).  the PMode bit comes from the high bit
 *	of the indexed PLUT entry, so we set PMode 1 by ORing in 0x8000 for
 *	PLUT entries 0 and 7.  (this really means nothing when bgColor != 0,
 *	since PMode 0 is also opaque mode.)
 ****************************************************************************/

void
recalc_colors (TextCel *tCel, int32 nColors)
{
  uint16 *thePlut;
  uint32 bgColor;
  uint32 fgColor;
  uint32 colorIndex;
  uint32 fr, fg, fb;    // foreground color components
  uint32 br, bg, bb;    // background color components
  uint32 fwr, fwg, fwb; // foreground color work components
  uint32 bwr, bwg, bwb; // background color work components
  int i;                // color index loop counter
  uint32 inverse;       // inverse of color index loop counter

  thePlut = (uint16 *)tCel->tc_CCB->ccb_PLUTPtr;

  bgColor = tCel->tc_bgColor;
  br = (bgColor >> 10) & 0x1F;
  bg = (bgColor >> 5) & 0x1F;
  bb = (bgColor >> 0) & 0x1F;

  for (colorIndex = 0; colorIndex < nColors; ++colorIndex)
    {
      fgColor = tCel->tc_fgColor[colorIndex];
      fr = (fgColor >> 10) & 0x1F;
      fg = (fgColor >> 5) & 0x1F;
      fb = (fgColor >> 0) & 0x1F;

      if (bgColor == 0)
        {
          for (i = 1; i < 8; ++i)
            {
              fwr = (fr * i) >> 3;
              fwg = (fg * i) >> 3;
              fwb = (fb * i) >> 3;
              thePlut[i - 1] = (uint16)((fwr << 10) | (fwg << 5) | fwb);
            }
        }
      else
        {
          for (i = 1; i < 8; ++i)
            {
              fwr = (fr * i) >> 3;
              fwg = (fg * i) >> 3;
              fwb = (fb * i) >> 3;
              inverse = 8L - i;
              bwr = (br * inverse) >> 3;
              bwg = (bg * inverse) >> 3;
              bwb = (bb * inverse) >> 3;
              thePlut[i - 1] = (uint16)(((fwr + bwr) << 10)
                                        | ((fwg + bwg) << 5) | (fwb + bwb));
            }
        }

      thePlut[0] = (uint16)(0x8000U | bgColor);
      thePlut[7] = (uint16)(0x8000U | fgColor);
      thePlut += 8;
    }

  if (bgColor == 0)
    {
      tCel->tc_CCB->ccb_Flags &= ~CCB_BGND;
      tCel->tc_CCB->ccb_PRE0 &= ~PRE0_BGND;
      tCel->tc_CCB->ccb_PIXC = PIXC_8BPP_BLEND;
    }
  else
    {
      tCel->tc_CCB->ccb_Flags |= CCB_BGND;
      tCel->tc_CCB->ccb_PRE0 |= PRE0_BGND;
      tCel->tc_CCB->ccb_PIXC = PIXC_OPAQUE;
    }
}

/*****************************************************************************
 * vformat_text()
 *	pass the text and related args through sprintf(), if the cel has a
 *	format buffer attached to it.  return a pointer to the resulting text.
 ****************************************************************************/

static char *
vformat_text (TextCel *tCel, char *fmtString, va_list fmtArgs)
{
  int32 formattedSize;

  if (tCel->tc_formatBuffer == NULL)
    {
      return fmtString;
    }

  formattedSize
      = sprintf (tCel->tc_formatBuffer, fmtString, *(ArgsByValue *)fmtArgs);

#if DEBUG
  if (formattedSize > tCel->tc_formatBufferSize)
    {
      DIAGNOSE (
          ("Formatted text exceeded buffer size, memory is now corrupted!\n"));
    }
#endif

  return tCel->tc_formatBuffer;
}

/*****************************************************************************
 * render_text_in_cel()
 *	invoke the low-level blit routine for each character in the text.
 *	handles clipping of characters that fall (even paritially) outside the
 *	cel boundaries.
 *	returns TRUE if any characters were clipped, FALSE if all went well.
 ****************************************************************************/

static int32
render_text_in_cel (TextCel *tCel, Boolean replace, char *formattedText)
{
  int32 charWidth;
  int32 charHeight;
  int32 charSpacing;
  int32 lineSpacing;
  int32 bpp;
  int32 celWidth;
  int32 celHeight;
  int32 celRowBytes;
  int32 dstX;
  int32 dstY;
  int32 endX;
  int32 endY;
  int32 colorIndex;
  uint32 theChar;
  void *blitInfo;
  CelData *celData;
  FontDescriptor *fd;
  int32 anyClipping = FALSE;

  /*------------------------------------------------------------------------
   * clean existing text out of the cel, if the caller so desires.
   * localize some values used a lot inside the rendering loop.
   *----------------------------------------------------------------------*/

  if (replace)
    {
      EraseTextInCel (tCel);
    }

  celWidth = tCel->tc_CCB->ccb_Width;
  celHeight = tCel->tc_CCB->ccb_Height;
  celData = tCel->tc_CCB->ccb_SourcePtr;
  celRowBytes = tCel->tc_celRowBytes;
  bpp = tCel->tc_formatFlags & TC_FORMAT_BPPMASK;
  colorIndex = tCel->tc_penNumber;

  fd = tCel->tc_fontDesc;
  charHeight = fd->fd_charHeight;
  charSpacing = fd->fd_charExtra + tCel->tc_fontAdjustSpacing;
  lineSpacing = fd->fd_leading + tCel->tc_fontAdjustLeading;

  /*------------------------------------------------------------------------
   * calc clipping, and render characters which fall wholly within the
   *	cel boundaries.
   *----------------------------------------------------------------------*/

  dstX = tCel->tc_XPosInCel;
  dstY = tCel->tc_YPosInCel;
  endY = dstY + charHeight - 1;

  while ((theChar = *formattedText++) != 0)
    {
      charWidth = GetFontCharInfo (fd, theChar, &blitInfo);
      if (theChar == '\n')
        {
          dstX = tCel->tc_leftMargin;
          dstY += charHeight + lineSpacing;
          endY = dstY + charHeight - 1;
        }
      else if (charWidth > 0)
        {
          endX = dstX + charWidth - 1;
          if (endX < celWidth && endY < celHeight)
            {
              BlitFontChar (fd, theChar, blitInfo, celData, dstX, dstY,
                            celRowBytes, colorIndex, bpp);
            }
          else
            {
              anyClipping = TRUE;
            }
          dstX += charWidth + charSpacing;
        }
    }

  tCel->tc_XPosInCel
      = dstX; // update the ending 'pen' position in case the caller
  tCel->tc_YPosInCel = dstY; // wants to add more chars following this text.

  return anyClipping;
}

/*----------------------------------------------------------------------------
 * public API begins here...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * DeleteTextCel()
 *	Delete a TextCel and any resources attached to it that we allocated.
 ****************************************************************************/

void
DeleteTextCel (TextCel *tCel)
{
  if (tCel != NULL)
    {
      if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF)
        {
          Free (tCel->tc_formatBuffer);
        }
      if (tCel->tc_CCB != NULL)
        {
          if (tCel->tc_CCB->ccb_SourcePtr != NULL)
            {
              Free (tCel->tc_CCB->ccb_SourcePtr);
            }
          Free (tCel->tc_CCB);
        }
      Free (tCel);
    }
}

/*****************************************************************************
 * CreateTextCel()
 *	Create a TextCel and the basic resources it needs.
 ****************************************************************************/

TextCel *
CreateTextCel (FontDescriptor *fDesc, uint32 formatFlags, int32 width,
               int32 height)
{
  int32 bpp;
  int32 rowBytes;
  TextCel *tCel = NULL;

  /*------------------------------------------------------------------------
   * validate parms.
   *----------------------------------------------------------------------*/

  formatFlags
      &= TC_FORMAT_FLAGSMASK; // make sure only client flags are present

  if (fDesc == NULL)
    {
      DIAGNOSE (("Invalid NULL FontDescriptor parm\n"));
      goto ERROR_EXIT;
    }

  if ((width == 0 || height == 0))
    {
      formatFlags |= TC_INTERNAL_AUTOSIZE;
    }

  bpp = formatFlags & TC_FORMAT_BPPMASK;

  switch (bpp)
    {
    case 0:
      bpp = 8;
      formatFlags |= TC_FORMAT_8BPPCEL;
      break;
    case 8:
      break;
#if VARIABLE_DESTBPP_IMPLEMENTED
    case 1:
      break;
    case 2:
      break;
    case 4:
      break;
#endif
    default:
      DIAGNOSE (("Unsuportted bits-per-pixel option %ld for text cel\n", bpp));
      goto ERROR_EXIT;
    }

  /*------------------------------------------------------------------------
   * create the TextCel structure, the CCB and CelData, and fill in the
   * rest of the TextCel fields with default values.
   *----------------------------------------------------------------------*/

  tCel = (TextCel *)Malloc (sizeof (TextCel), MEMTYPE_ANY | MEMTYPE_FILL | 0);
  if (tCel == NULL)
    {
      DIAGNOSE (("Can't allocate TextCel\n"));
      goto ERROR_EXIT;
    }

  tCel->tc_CCB = alloc_text_CCB (width, height, bpp, &rowBytes);
  if (tCel->tc_CCB == NULL)
    {
      goto ERROR_EXIT; // error already reported by alloc_text_CCB
    }

  tCel->tc_fontDesc = fDesc;
  tCel->tc_formatFlags = formatFlags;
  tCel->tc_celRowBytes = rowBytes;
  tCel->tc_fgColor[0] = MakeRGB15 (31, 31, 31);

  recalc_colors (tCel, 1);

  return tCel;

ERROR_EXIT:

  DeleteTextCel (tCel);
  return NULL;
}

/*****************************************************************************
 * CloneTextCel()
 *	Create a new TextCel using and existing one as a template.
 ****************************************************************************/

TextCel *
CloneTextCel (TextCel *tCel, Boolean clonePixels)
{
  TextCel *newCel;
  char *formatBuffer;

  /*------------------------------------------------------------------------
   * go create a new TextCel using the width/height/flags of the template.
   *----------------------------------------------------------------------*/

  newCel = CreateTextCel (tCel->tc_fontDesc, tCel->tc_formatFlags,
                          tCel->tc_CCB->ccb_Width, tCel->tc_CCB->ccb_Height);
  if (newCel == NULL)
    {
      return NULL;
    }

  /*------------------------------------------------------------------------
   * copy from the template to the new cel those things which CreateTextCel()
   * gave zero/default values to.
   *----------------------------------------------------------------------*/

  newCel->tc_formatFlags = tCel->tc_formatFlags;
  newCel->tc_fontAdjustSpacing = tCel->tc_fontAdjustSpacing;
  newCel->tc_fontAdjustLeading = tCel->tc_fontAdjustLeading;
  newCel->tc_leftMargin = tCel->tc_leftMargin;
  newCel->tc_topMargin = tCel->tc_topMargin;
  newCel->tc_bgColor = tCel->tc_bgColor;
  memcpy (newCel->tc_fgColor, tCel->tc_fgColor, sizeof (newCel->tc_fgColor));

  /*------------------------------------------------------------------------
   * if the template cel has a dynamic format buffer, allocate one for the
   * new cel.  if the template has a static buffer the new cel inherits it.
   *----------------------------------------------------------------------*/

  if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF)
    {
      formatBuffer = NULL; // ask SetBuffer to allocate one
    }
  else
    {
      formatBuffer = tCel->tc_formatBuffer; // attach same buffer to new cel
    }

  if (SetTextCelFormatBuffer (newCel, formatBuffer, tCel->tc_formatBufferSize)
      < 0)
    {
      DeleteTextCel (newCel);
      return NULL;
    }

  /*------------------------------------------------------------------------
   * if asked to clone the pixels, do that now.
   *----------------------------------------------------------------------*/

  if (clonePixels)
    {
      memcpy (newCel->tc_CCB->ccb_SourcePtr, tCel->tc_CCB->ccb_SourcePtr,
              sizeof (newCel->tc_CCB->ccb_SourcePtr));
    }

  return newCel;
}

/*****************************************************************************
 * GetTextCelSpacingAdjust()
 *	Retrieve the spacing delta for the cel.
 ****************************************************************************/

void
GetTextCelSpacingAdjust (TextCel *tCel, int32 *adjustSpacing)
{
  if (adjustSpacing)
    {
      *adjustSpacing = tCel->tc_fontAdjustSpacing;
    }
}

/*****************************************************************************
 * GetTextCelLeadingAdjust()
 *	Retrieve the leading delta for the cel.
 ****************************************************************************/

void
GetTextCelLeadingAdjust (TextCel *tCel, int32 *adjustLeading)
{
  if (adjustLeading)
    {
      *adjustLeading = tCel->tc_fontAdjustLeading;
    }
}

/*****************************************************************************
 * GetTextCelColor()
 *	Retrieve the background/foreground colors for the cel.
 ****************************************************************************/

void
GetTextCelColor (TextCel *tCel, int32 *bgColor, int32 *fgColor0)
{
  if (bgColor)
    {
      *bgColor = tCel->tc_bgColor;
    }

  if (fgColor0)
    {
      *fgColor0 = tCel->tc_fgColor[0];
    }
}

/*****************************************************************************
 * GetTextCelColors()
 *	Retrieve the background and all foreground colors for the cel.
 ****************************************************************************/

void
GetTextCelColors (TextCel *tCel, int32 *bgColor, int32 fgColors[4])
{
  if (bgColor)
    {
      *bgColor = tCel->tc_bgColor;
    }

  if (fgColors)
    {
      memcpy (fgColors, tCel->tc_fgColor, sizeof (tCel->tc_fgColor));
    }
}

/*****************************************************************************
 * GetTextCelCoords()
 *	Retrieve the cel's CCB X/Y coords.
 ****************************************************************************/

void
GetTextCelCoords (TextCel *tCel, Coord *ccbX, Coord *ccbY)
{
  if (ccbX)
    {
      *ccbX = tCel->tc_CCB->ccb_XPos;
    }

  if (ccbY)
    {
      *ccbY = tCel->tc_CCB->ccb_YPos;
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

void
GetTextCelMargins (TextCel *tCel, int32 *leftMargin, int32 *topMargin)
{
  if (leftMargin)
    {
      *leftMargin = tCel->tc_leftMargin;
    }

  if (topMargin)
    {
      *topMargin = tCel->tc_topMargin;
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

void
GetTextCelPenNumber (TextCel *tCel, int32 *penNumber)
{
  if (penNumber)
    {
      *penNumber = tCel->tc_penNumber;
    }
}

/*****************************************************************************
 * GetTextCelSize()
 *	Retrieve the width and/or height for the cel.
 ****************************************************************************/

void
GetTextCelSize (TextCel *tCel, int32 *width, int32 *height)
{
  if (width)
    {
      *width = tCel->tc_CCB->ccb_Width;
    }

  if (height)
    {
      *height = tCel->tc_CCB->ccb_Height;
    }
}

/*****************************************************************************
 * GetTextCelFormatBuffer()
 *	Retrieve the format buffer information for the cel.
 ****************************************************************************/

void
GetTextCelFormatBuffer (TextCel *tCel, char **buffer, uint32 *bufsize)
{
  if (buffer)
    {
      *buffer = tCel->tc_formatBuffer;
    }

  if (bufsize)
    {
      *bufsize = tCel->tc_formatBufferSize;
    }
}

/*****************************************************************************
 * SetTextCelSpacingAdjust()
 *	Store the provided spacing delta in our private field.
 ****************************************************************************/

void
SetTextCelSpacingAdjust (TextCel *tCel, int32 adjustSpacing)
{
  tCel->tc_fontAdjustSpacing = adjustSpacing;
}

/*****************************************************************************
 * SetTextCelLeadingAdjust()
 *	Store the provided leading delta in our private field.
 ****************************************************************************/

void
SetTextCelLeadingAdjust (TextCel *tCel, int32 adjustLeading)
{
  tCel->tc_fontAdjustLeading = adjustLeading;
}

/*****************************************************************************
 * SetTextCelColor()
 *	Store the provided background/foreground colors in our private fields.
 *	This one sets just the pen0 color.  If the background color is
 *changing, we ask recalc_colors() to recalc PLUT entries for all 4 pens, so
 *that a change between static and dynamic anti-aliasing gets calc'd based on
 *the new background color.  if background color isn't changing, there's no
 *	need to recalc the PLUT entries for all 4 pens, we can just do the one
 *	that changed.
 ****************************************************************************/

void
SetTextCelColor (TextCel *tCel, int32 bgColor, int32 fgColor0)
{
  int32 nColors;

  nColors = (tCel->tc_bgColor == bgColor) ? 1 : 4;

  tCel->tc_bgColor = bgColor;
  tCel->tc_fgColor[0] = fgColor0;

  recalc_colors (tCel, nColors);
}

/*****************************************************************************
 * SetTextCelColors()
 *	Store the provided background/foreground colors in our private fields.
 *	This one allows specification of all four possible foreground colors,
 *	instead of just the pen0 color.  We always recalc the PLUT entries for
 *	all 4 pens, because any or all of them may be changing.
 ****************************************************************************/

void
SetTextCelColors (TextCel *tCel, int32 bgColor, int32 fgColors[4])
{
  int i;

  tCel->tc_bgColor = bgColor;
  for (i = 0; i < 4; ++i)
    {
      tCel->tc_fgColor[i] = fgColors[i];
    }

  recalc_colors (tCel, 4);
}

/*****************************************************************************
 * SetTextCelCoords()
 *	Store the provided coords in the cel's CCB.
 ****************************************************************************/

void
SetTextCelCoords (TextCel *tCel, Coord x, Coord y)
{
  tCel->tc_CCB->ccb_XPos = x;
  tCel->tc_CCB->ccb_YPos = y;
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

Err
SetTextCelFormatBuffer (TextCel *tCel, char *buffer, uint32 bufsize)
{
  if (tCel->tc_formatFlags & TC_INTERNAL_DYNBUF)
    {
      tCel->tc_formatBuffer = (char *)Free (tCel->tc_formatBuffer);
      tCel->tc_formatBufferSize = 0;
      tCel->tc_formatFlags &= ~TC_INTERNAL_DYNBUF;
    }

  if (buffer == NULL && bufsize != 0)
    {
      buffer = (char *)Malloc (bufsize, MEMTYPE_ANY);
      if (buffer == NULL)
        {
          DIAGNOSE (("Can't allocate text format buffer\n"));
          return -1;
        }
      tCel->tc_formatFlags |= TC_INTERNAL_DYNBUF;
    }

  tCel->tc_formatBuffer = buffer;
  tCel->tc_formatBufferSize = bufsize;

  return 0;
}

/*****************************************************************************
 *
 ****************************************************************************/

void
SetTextCelMargins (TextCel *tCel, int32 leftMargin, int32 topMargin)
{
  if (leftMargin > 0)
    {
      tCel->tc_leftMargin = leftMargin;
    }

  if (topMargin > 0)
    {
      tCel->tc_topMargin = topMargin;
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

void
SetTextCelPenNumber (TextCel *tCel, int32 penNumber)
{
  tCel->tc_penNumber = penNumber & 0x03;
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

Err
SetTextCelSize (TextCel *tCel, int32 width, int32 height)
{
  int32 rowBytes;

  int32 bpp = tCel->tc_formatFlags & TC_FORMAT_BPPMASK;

  if (width == 0 || height == 0)
    {
      tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE;
    }
  else
    {
      tCel->tc_formatFlags &= ~TC_INTERNAL_AUTOSIZE;
    }

  rowBytes = alloc_text_celdata (tCel->tc_CCB, width, height, bpp);
  if (rowBytes > 0)
    {
      tCel->tc_celRowBytes = rowBytes;
      EraseTextInCel (tCel);
    }

  return rowBytes;
}

/*****************************************************************************
 * EraseTextInCel()
 *	Clear existing pixels from a cel, and set the 'pen' back to 0,0.
 ****************************************************************************/

void
EraseTextInCel (TextCel *tCel)
{
  memset (tCel->tc_CCB->ccb_SourcePtr, 0,
          MemBlockSize (tCel->tc_CCB->ccb_SourcePtr));
  tCel->tc_XPosInCel = tCel->tc_leftMargin;
  tCel->tc_YPosInCel = tCel->tc_topMargin;
}

/*****************************************************************************
 * vUpdateTextInCel()
 *	Render text into cel, optionally clearing existing pixels first.
 ****************************************************************************/

Err
vUpdateTextInCel (TextCel *tCel, Boolean replaceExisting, char *fmtString,
                  va_list fmtArgs)
{
  char *formattedText;
  int32 bpp;
  int32 celRowBytes;
  int32 width;
  int32 height;

  formattedText = vGetTextExtent (tCel, &width, &height, fmtString, fmtArgs);

  if (replaceExisting && (tCel->tc_formatFlags & TC_INTERNAL_AUTOSIZE))
    {
      bpp = tCel->tc_formatFlags & TC_FORMAT_BPPMASK;
      if ((celRowBytes = SetTextCelSize (tCel, width, height)) < 0)
        {
          return celRowBytes; // error already reported
        }
      tCel->tc_formatFlags |= TC_INTERNAL_AUTOSIZE; // force this back on
    }

  return render_text_in_cel (tCel, replaceExisting, formattedText);
}

/*****************************************************************************
 * UpdateTextInCel()
 *	Render text into cel, optionally clearing existing pixels first.
 ****************************************************************************/

Err
UpdateTextInCel (TextCel *tCel, Boolean replaceExisting, char *fmtString, ...)
{
  int32 rv;
  va_list fmtArgs;

  va_start (fmtArgs, fmtString);
  rv = vUpdateTextInCel (tCel, replaceExisting, fmtString, fmtArgs);
  va_end (fmtArgs);
  return rv;
}

/*****************************************************************************
 * vGetTextExtent()
 *	Calculate and return the width and height needed to display some text.
 *	Returns width and/or height via the provided pointers.
 *	Returns pointer to the formatted text (results of optional sprintf
 *	processing).
 ****************************************************************************/

char *
vGetTextExtent (TextCel *tCel, int32 *pWidth, int32 *pHeight, char *fmtString,
                va_list fmtArgs)
{
  int32 charSpacing;
  int32 lineSpacing;
  int32 charHeight;
  uint32 theChar;
  uint32 thisCharWidth;
  char *formattedText;
  char *s;
  uint32 priorChar = 0;
  int32 lineCount = 0;
  int32 thisLineWidth = 0;
  int32 maxLineWidth = 0;
  FontDescriptor *fd = tCel->tc_fontDesc;

  /*------------------------------------------------------------------------
   * format the text, and calc the adjusted char and line spacing.
   *----------------------------------------------------------------------*/

  formattedText = vformat_text (tCel, fmtString, fmtArgs);

  charHeight = fd->fd_charHeight;
  charSpacing = fd->fd_charExtra + tCel->tc_fontAdjustSpacing;
  lineSpacing = fd->fd_leading + tCel->tc_fontAdjustLeading;

  /*------------------------------------------------------------------------
   * now cruise through the text counting lines and remembering the width
   * of the longest line we find.
   *----------------------------------------------------------------------*/

  s = formattedText;
  while ((theChar = *s++) != 0)
    {
      if (theChar == '\n')
        {
          ++lineCount;
          if (maxLineWidth < thisLineWidth)
            {
              maxLineWidth = thisLineWidth;
            }
          thisLineWidth = 0;
        }
      else
        {
          thisCharWidth = GetFontCharWidth (fd, theChar);
          if (thisCharWidth > 0)
            {
              thisLineWidth += thisCharWidth + charSpacing;
            }
        }
      priorChar = theChar;
    }

  /*------------------------------------------------------------------------
   * the last line of text ends with an implied newline character, if
   * there wasn't an explict newline character there.
   *----------------------------------------------------------------------*/

  if (priorChar != '\n')
    {
      ++lineCount;
      if (maxLineWidth < thisLineWidth)
        {
          maxLineWidth = thisLineWidth;
        }
    }

  /*------------------------------------------------------------------------
   * return the width and height (in pixels) via the supplied pointers,
   * and return a pointer to the formatted text.
   *----------------------------------------------------------------------*/

  if (maxLineWidth > 0)
    {
      maxLineWidth -= charSpacing;
    }

  if (pWidth)
    {
      *pWidth = maxLineWidth;
    }

  if (pHeight)
    {
      *pHeight = ((charHeight + lineSpacing) * lineCount) - lineSpacing;
    }

  return formattedText;
}

/*****************************************************************************
 * GetTextExtent()
 *	Calculate and return the width and height needed to display some text.
 *	Returns width and/or height via the provided pointers.
 *	Returns pointer to the formatted text (results of optional sprintf
 *	processing).
 ****************************************************************************/

char *
GetTextExtent (TextCel *tCel, int32 *pWidth, int32 *pHeight, char *fmtString,
               ...)
{
  char *rv;
  va_list fmtArgs;

  va_start (fmtArgs, fmtString);
  rv = vGetTextExtent (tCel, pWidth, pHeight, fmtString, fmtArgs);
  va_end (fmtArgs);
  return rv;
}

/*****************************************************************************
 * DrawTextString()
 *	Render formatted text directly to the specified bitmap.
 ****************************************************************************/

void
DrawTextString (FontDescriptor *fd, GrafCon *gcon, Item bitmapItem, char *text,
                ...)
{
  TextCel *tCel;
  va_list args;

  va_start (args, text);

  tCel = CreateTextCel (fd, TC_FORMAT_8BPPCEL, 0, 0);
  if (SetTextCelFormatBuffer (tCel, NULL, 1024) < 0)
    {
      return; // error has already been reported
    }
  if (vUpdateTextInCel (tCel, TRUE, text, args) < 0)
    {
      return; // error has already been reported
    }
  SetTextCelColor (tCel, gcon->gc_BGPen, gcon->gc_FGPen);
  SetTextCelCoords (tCel, Convert32_F16 (gcon->gc_PenX),
                    Convert32_F16 (gcon->gc_PenY));

  DrawCels (bitmapItem, tCel->tc_CCB);

  // adjust the coords in the GrafCon based on where the text 'pen'
  // was left after rendering the characters in the the cel buffer.

  gcon->gc_PenX += tCel->tc_XPosInCel;
  gcon->gc_PenY += tCel->tc_YPosInCel;

  DeleteTextCel (tCel);

  va_end (args);
}

/*****************************************************************************
 * DrawTextChar()
 *	Render a character directly to the specified bitmap.
 ****************************************************************************/

void
DrawTextChar (FontDescriptor *fd, GrafCon *gcon, Item bitmapItem,
              uint32 character)
{
  char fmtString[2];

  fmtString[0] = (char)character;
  fmtString[1] = 0;
  DrawTextString (fd, gcon, bitmapItem, fmtString);
}

/*----------------------------------------------------------------------------
 * the following junk would be used to process 4-bit coded cels with 2-pass
 * rendering and 16 levels of anti-aliasing, and also potentially 2-bit
 * coded cels with 4 levels of aa (a concept which has never been tested).
 * there isn't currently any low-level FontLib support for these formats,
 * but I'm not yet ready to throw away this cel-related support code forever.
 *--------------------------------------------------------------------------*/

#if VARIABLE_DESTBPP_IMPLEMENTED

#define PMV(x) ((x - 1) << 2)
#define PDV16 0x0000
#define PDV8 0x0003
#define PDV4 0x0002
#define PDV2 0x0001

#define SCALE16(x)                                                            \
  (((PMV (x) | 1) << 10) | ((PMV (x) | 1) << 5) | (PMV (x) | 1))
#define SCALE8(x)                                                             \
  (((PMV (x) | PDV8) << 10) | ((PMV (x) | PDV8) << 5) | (PMV (x) | PDV8))
#define SCALE4(x)                                                             \
  (((PMV (x) | PDV4) << 10) | ((PMV (x) | PDV4) << 5) | (PMV (x) | PDV4))

static uint16 gScale16PLUT[16] = { 0,
                                   SCALE16 (6) | 0x8000U,
                                   SCALE16 (5) | 0x8000U,
                                   SCALE16 (4) | 0x8000U,
                                   SCALE16 (3) | 0x8000U,
                                   SCALE16 (2) | 0x8000U,
                                   SCALE16 (1) | 0x8000U,
                                   SCALE16 (8),
                                   SCALE16 (7),
                                   SCALE16 (6),
                                   SCALE16 (5),
                                   SCALE16 (4),
                                   SCALE16 (3),
                                   SCALE16 (2),
                                   SCALE16 (1),
                                   0 };

static uint16 gScale4PLUT[4] = { 0, SCALE8 (5), SCALE8 (3), 0 };

if (nlevels == 4)
  { // 4-level special case: scale to 3/8 and 5/8
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
 *	no longer needed, but demonstrates PIXCs and stuff for 16-level AA
 *work...
 ****************************************************************************/

void
DrawAACel (GrafCon *gcon, Item bitmapItem, uint32 width, uint32 height,
           CCB *ccb)
{
  uint16 work_plut[32];
  GrafCon lgcon;
  Rect bounds;

  /**************************************************************/
  /* Paint a solid color background if the drawMode == srcCopy. */
  /**************************************************************/

  if ((gcon->gc_Flags & f_drawModeMask) == f_srcCopyDrawMode)
    {
      SetFGPen (&lgcon, gcon->gc_BGPen);

      bounds.rect_XLeft = gcon->gc_PenX;
      bounds.rect_YTop = gcon->gc_PenY;
      bounds.rect_XRight = bounds.rect_XLeft + width;
      bounds.rect_YBottom = bounds.rect_YTop + height;

      FillRect (bitmapItem, &lgcon, &bounds);
    }

  /**********************************************************/
  /* Pre-scale the existing pixels using the scaling table. */
  /**********************************************************/

  ccb->ccb_PLUTPtr = gScale16PLUT; // scaling table is static
  ccb->ccb_PIXC
      = 0xE090E000; // scale CFB pixels using mul/div factors from PLUT
  DrawCels (bitmapItem, ccb);

  /*******************************************************/
  /* Now draw the anti-aliased portion of the character. */
  /*******************************************************/

  ccb->ccb_PLUTPtr = calc_scaled_PLUT (work_plut, 16, gcon->gc_FGPen);
  ccb->ccb_PIXC
      = 0x1F001F80; // pmode0 = (PDC+CFB)/1, pmode1 = (PDC+0)/1 (replace mode)
  DrawCels (bitmapItem, ccb);
}

/*****************************************************************************
 * recalc_colors()
 *	calculate PLUT entries for anti-aliasing.
 ****************************************************************************/

void
recalc_colors (TextCel *tCel, int nColors)
{
  int i;
  int shiftDivide;
  uint16 *thePlut;
  int32 nlevels;
  uint32 bgColor;
  uint32 fgColor;
  uint32 fr, fg, fb;
  uint32 br, bg, bb;
  uint32 wr, wg, wb;
  uint32 bwr, bwg, bwb;
  uint32 reciprocal;

  (void)nColors; // not currently used

  thePlut = (uint16 *)tCel->tc_CCB->ccb_PLUTPtr;
  bgColor = tCel->tc_bgColor;
  fgColor = tCel->tc_fgColor[0];

  if (tCel->tc_formatFlags & TC_FORMAT_4BPPCEL)
    {
      nlevels = 16;
      shiftDivide = 4;
    }
  else
    {
      nlevels = 8;
      shiftDivide = 3;
    }

  br = (bgColor >> 10) & 0x1F;
  bg = (bgColor >> 5) & 0x1F;
  bb = (bgColor >> 0) & 0x1F;

  fr = (fgColor >> 10) & 0x1F;
  fg = (fgColor >> 5) & 0x1F;
  fb = (fgColor >> 0) & 0x1F;

  for (i = 1; i < nlevels; ++i)
    {
      reciprocal = nlevels - i;
      wr = ((fr * i) >> shiftDivide);
      wg = ((fg * i) >> shiftDivide);
      wb = ((fb * i) >> shiftDivide);
      bwr = ((br * reciprocal) >> shiftDivide);
      bwg = ((bg * reciprocal) >> shiftDivide);
      bwb = ((bb * reciprocal) >> shiftDivide);
      thePlut[i - 1]
          = (uint16)(((wr + bwr) << 10) | ((wg + bwg) << 5) | (wb + bwb));
    }

  if (bgColor == 0)
    {
      tCel->tc_CCB->ccb_Flags &= ~CCB_BGND;
      tCel->tc_CCB->ccb_PIXC
          = (nlevels == 8) ? PIXC_8BPP_BLEND : PIXC_4BPP_BLEND;
    }
  else
    {
      tCel->tc_CCB->ccb_Flags |= CCB_BGND;
      tCel->tc_CCB->ccb_PIXC = PIXC_OPAQUE;
    }

  thePlut[0] = (uint16)(0x8000U | bgColor);
  thePlut[nlevels - 1] = (uint16)(0x8000U | fgColor);
}

#endif // end of unused varying-bit-depth support junk

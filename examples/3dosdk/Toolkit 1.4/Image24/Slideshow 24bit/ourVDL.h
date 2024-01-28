/*
        File:		ourVDL.h

        Contains:	Header for interim VDL routines.

        Written by:	eric carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	 8/11/93	JAY		corrected kVLDSize, was
   (kScreenWidth *sizeof(VDL_REC)), worked because screen width is greater than
   height, but we really only need (kScreenHeight *sizeof(VDL_REC)) <2> 6/23/93
   JAY		Added kVDLSize defn.
                 <1>	 3/18/93	JAY		first checked in
                                12/18/92	ec		New.

        To Do:
*/

#ifndef __Init3DO_h
#include "Init3DO.h"
#endif

// use any 32-bit variable as long
#define LONGWORD(aStruct) (*((long *)&(aStruct)))

#ifndef noErr
#define noErr 0
#endif

#define kScreenWidthNTSC 320
#define kScreenHeightNTSC 240
#define kScreenWidthPAL 384
#define kScreenHeightPAL 288

// The following line will enable PAL options

#define enablePAL

#define NTSCFieldFreq 60
#define PALFieldFreq 50

#ifdef enablePAL

#define kScreenWidth kScreenWidthPAL
#define kScreenHeight kScreenHeightPAL
#define VDL_DISPMOD VDL_DISPMOD_384

#else

#define kScreenWidth kScreenWidthNTSC
#define kScreenHeight kScreenHeightNTSC
#define VDL_DISPMOD VDL_DISPMOD_320

#endif

#define kVDLSize (myScreenHeight * sizeof (VDL_REC))

#define kControlWords 4
#define kDisplayWords 1
#define kCLUTWords 32
#define kFillerWords 2

extern int32 myScreenWidth;
extern int32 myScreenHeight;

//
// There are a minimum of 4 (maybe 6) entries in a 'color palette'.  There
//  are ALWAYS 4 control words:  The first word is always the
//  control value to be deposited in the CLUT DMA controller, the second
//  word is always the 'current' frame buffer address, the third word is
//  always the 'previous' frame buffer address, and the fourth word is
//  always the address of the next color palette to be used.  The maximum
//  number of total words in a color palette is about 50, due to the amount
//  of time available for the hardware to process it during horiz blank.  In
//  addition a color palette is not allowed to fall across a 1 megabyte
//  physical VRAM page boundary.

// the actual number of color entries in a vdl entry
#define kClutColorWords ((sizeof (VDL_REC) / sizeof (UInt32)) - kControlWords)

typedef struct
{
  unsigned int : 5;                   // [31-27] reserved; must be zero
  unsigned int sc640 : 1;             // [   26]
  unsigned int displayMode : 3;       // [25-23] display buffer mode
  unsigned int captureSlipStream : 1; // [   22] SlipStream capture enabled
  unsigned int videoDMA : 1;          // [   21] enable video DMA
  unsigned int slipStreamDMAchan : 1; // [   20] selects 'frame' or 'command'
                                      // DMA channel
  unsigned int lines480 : 1;          // [   19] 480-line frame buffer
  unsigned int
      relativeAddrNextCLUT : 1; // [   18] 'next palette' addr is relative
  unsigned int
      prevAddrCalc : 1; // [   17] how to calculate previous address value
  unsigned int
      useCurrentLineAddr : 1; // [   16] 'current' frame buff addr valid
  unsigned int
      usePreviousLineAddr : 1;    // [   15] 'previous' frame buff addr valid
  unsigned int numColorWords : 6; // [14- 9] length of CLUT - 4 (control words)
  unsigned int
      scanLines : 9; // [ 8- 0] # of scan lines to wait before loading next VDL
} DMAControlWord;

typedef struct
{
  unsigned int
      controlWord : 1; // [   31] set to one for a display control word
  unsigned int
      controlWordType : 1; // [   30] set to one for a display control word
  unsigned int backgroundCLUT : 1; // [   29] set background color registers
  unsigned int nullAMYVideoControl : 1; // [   28]
  unsigned int videoIsPAL : 1;          // [   27] PAL video
  unsigned int select640Mode : 1;       // [   26] reserved; set to zero
  unsigned int bypassCLUT : 1;          // [   25] bypass CLUT if pen MSB set
  unsigned int
      slipStreamOverlay : 1; // [   24] source of overlay is SlipStream
  unsigned int forceTransparency : 1;  // [   23]
  unsigned int backgroundDetector : 1; // [   22] enable background detector
  unsigned int swapWindowPenHV : 1; // [   21] swap H and V bits in pen numbers
                                    // in window
  unsigned int windowVSource : 2; // [20-19] selects source for V sub-positions
                                  // in window pixels
  unsigned int windowHSource : 2; // [18-17] selects source for H sub-position
                                  // in window pixels
  unsigned int windowBlueLSBSource : 2;    // [16-15] selects source for window
                                           // blue pen number's LSB
  unsigned int windowVerticalInterp : 1;   // [   14] enable vertical
                                           // interpolation in window
  unsigned int windowHorizontalInterp : 1; // [   13] enable horizontal
                                           // interpolation in window
  unsigned int bypassLSBIsRandom : 1; // [   12] enable random number generator
                                      // for low 3 of CLUT bypass
  unsigned int windowMSBReplicate : 1; // [   11]
  unsigned int swapPenVH : 1; // [   10] swap H and V bits in pen number
  unsigned int VSource : 2; // [ 9- 8] selects source of vertical sub-position
  unsigned int
      HSource : 2; // [ 7- 6] selects source of horizontal sub-position
  unsigned int
      blueLSBSource : 2; // [ 5- 4] selects source of blue pen number's LSB
  unsigned int
      verticalInterpolation : 1; // [    3] enable vertical interpolation
  unsigned int
      horizontalInterpolation : 1; // [    2] enable horizontal interpolation
  unsigned int colorsOnly : 1;     // [    1] reserved; set to zero
  unsigned int noVerticalInterpLine : 1; // [    0] suppress vertical
                                         // interpolation for this line only
} VDLDisplayCtlWord;

typedef struct
{
  unsigned int flag : 1;   // [    31] one   bits of zero to signify a color
                           // value VDL word
  unsigned int select : 2; // [ 30-29] two   bits of color select 0=>all
                           // 01=>blue 10=>green 11=>red
  unsigned int index : 5;  // [ 28-24] five  bits of color index
  unsigned int red : 8;    // [ 23-16] eight bits of red
  unsigned int green : 8;  // [ 15- 8] eight bits of green
  unsigned int blue : 8;   // [  7- 0] eight bits of blue
} VDLColorEntry;

typedef struct
{
  uint32 SVDLDMA;
  uint32 SVDLCurBuf;
  uint32 SVDLPrevBuf;
  uint32 SVDLNextVDL;
  uint32 SVDLDisplay;
  VDLColorEntry SVDLColors[32];
  uint32 backgroundEntry; //	RGB 000 will use this entry
  uint32 SVDLFiller1;
  uint32 SVDLFiller2;
} SVDL;

int32 SetDisplayCtlWord (Boolean Hinterp, Boolean Vinterp, Boolean clutBypass);
int32 SetDMACtlWord (int32 clutColorWords, int32 scanLineCount,
                     Boolean lastScanLine);
void InitVDL (VDLEntry *vdlPtr, Bitmap *destBitmap);
void CorrectVDL (VDLEntry *vdlPtr, Bitmap *destBitmap);
void MergeVDL (VDLEntry *rawVDLPtr, VDLEntry *activeVDLPtr, uint32 vdllines);

int32 AllocateVDL (VDLEntry **newVDL, Bitmap *screenBitMap);
void ShowAnotherField (VDLDisplayCtlWord dmaWord, Boolean nextField);
void SetCtlField (VDLDisplayCtlWord *dmaWord, Boolean increase);
short GetDisplayCtlValue (VDLDisplayCtlWord dmaWord, short fieldNum);
VDLDisplayCtlWord SetDisplayCtlValue (VDLDisplayCtlWord dmaWord,
                                      short newValue, short fieldNum);

void DrawImage24 (int32 *image, Bitmap *destBitmap);
void InitVDL480 (int32 *vdl, Bitmap *destBitmap, int32 bitmapindex,
                 int32 PALmode);
void InitSimpleVDL (SVDL *v, Bitmap *destBitmap, int32 PALmode);

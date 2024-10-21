
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ktextbox.c,v 1.5 1994/11/22 23:47:30 vertex Exp $
**
******************************************************************************/

#include "audio.h"
#include "debug.h"
#include "debug3do.h"
#include "filefunctions.h"
#include "folio.h"
#include "graphics.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdlib.h"
#include "string.h"
#include "task.h"
#include "types.h"

#include "form3do.h"
#include "init3do.h"
#include "ktextbox.h"
#include "parse3do.h"

/*------------------------------------------------------------------------
 * Macros used to determine if the string is Kanji (S-JIS level 1),
 *	Kanji (S-JIS level 2), Kana (Katakana), or ANK (Alphabet,
 *	Numerals, and Katakana).
 *----------------------------------------------------------------------*/

#define isKanji(c) (0x81 <= (c) && (c) <= 0x9F || 0xE0 <= (c) && (c) <= 0xFC)
#define isKanji2(c) (0x40 <= (c) && (c) <= 0xFC && (c) != 0x7F)
#define isKana(c) (0xA0 <= (c) && (c) <= 0xDF)
#define isANK(c) ((0x20 <= (c) && (c) <= 0x7E) || isKana (c))

/*------------------------------------------------------------------------
 * ASCII values for various control characters.
 *----------------------------------------------------------------------*/

#define CR 0x0D         /* 改行 */
#define LF 0x0A         /* 復帰 */
#define ESC 0x1B        /* エスケープ */
#define TAB 0x09        /* タブ */
#define SPACE 0x20      /* スペース */
#define KANA_START 0xA0 /* 半角カタカナスタート */
#define DEL 0x7F        /* デリート文字 */

#define O_VERTICAL 0x01 /* 縦書き ON */
#define O_KINSOKU 0x02  /* 禁則処理 ON */

#define ATTR_STD 0x01    /* 標準の字体 */
#define ATTR_BOLD 0x02   /* ボールド体(太字) */
#define ATTR_ITARIC 0x04 /* イタリック体(斜体) */
#define ATTR_WHITE 0x08  /* 白抜き */
#define ATTR_SHADOW 0x10 /* 影付き */

#define XXX_UNDERLINE 0x00   /* 下線 */
#define XXX_RUBI 0x00        /* ルビ */
#define XXX_SUBSCRIPT 0x00   /* 下付き */
#define XXX_SUPERSCRIPT 0x00 /* 上付き */

#define TOP_KIN_ZEN "" /* 全角行頭禁則文字 */
#define BTM_KIN_ZEN "" /* 全角行末禁則文字 */
#define TOP_KIN_HAN "" /* 半角行頭禁則文字 */
#define BTM_KIN_HAN "" /* 半角行頭禁則文字 */

#define FONT_GOTHIC 1  /* 平成ゴシック体 */
#define FONT_MINCHOU 2 /* 平成明朝体 */

#define PLUTSIZE_4BIT_CODEDCEL 32 /* 4bitセルのPLUTのサイズ */
#define PLUTSIZE_1BIT_CODEDCEL 4  /* 1bitセルのPLUTのサイズ */

#define FOURBITS_PER_PIXEL 4 /* 4bitセルのPLUTの場合 */
#define ONEBITS_PER_PIXEL 1  /* 1bitセルのPLUTのサイズ */

#define CODE_TABLE_HAN 0x01   /* 半角コードテーブルが存在する */
#define CODE_TABLE_ZEN 0x02   /* 全角コードテーブルが存在する */
#define MINMAX_TABLE_HAN 0x04 /* 半角文字範囲テーブルが存在する */
#define MINMAX_TABLE_ZEN 0x08 /* 全角文字範囲テーブルが存在する */
#define WIDTH_TABLE_HAN 0x10  /* 半角文字幅テーブルが存在する */
#define WIDTH_TABLE_ZEN 0x20  /* 全角文字幅テーブルが存在する */

typedef struct
{
  int32 Value; /* 文字コード */
} KCharRec;

typedef struct
{
  int32 chunk_ID; /* 'JFNT' Magic number to identify font data */
  int32
      chunk_size; /* Size in bytes of chunk including chunk_size & chunk_ID */
  uint32 version; /* File format version identifier.  0 for now */

  uint32 headerLength; /* Size in bytes of KFontFileHeader */
  uint32 fontType;     /* 0 = bitmap */
  uint32 fontSubType;  /* Type face of the font */
  uint32 fontFlags;    /* 32-bits of font flags.  0 for now */

  uint32 fontCharWidth; /* 文字の高さ */
                        /* Maximum Kanji character width (pixels). */
                        /* Note: ASCII characters are half this value. */

  uint32 fontCharHeight; /* 文字の幅 */
                         /* Height of all characters (pixels). */

  uint32 bitsPerPixel; /* １画素当たりのビット数 */
                       /* Bit depth of each pixel. */

  uint32 grayScaleBit; /* １画素当たりのビット数のうち階調用ビット数 */
                       /* Levels of gray for each pixel. */

  uint32 cornerWeightBit; /* Bit depth of corner-weighting (if used). */

  uint32 hmincode; /* 半角文字最小コード */
                   /* Minimum Hankaku (half-width) character code. */

  uint32 hmaxcode; /* 半角文字最大コード */
                   /* Maximum Hankaku (half-width) character code. */

  uint32 zmincode; /* 全角文字最小コード */
                   /* Minimum Zenkaku (full-width) character code. */

  uint32 zmaxcode; /* 全角文字最大コード */
                   /* Maximum Zenkaka (full-width) character code. */

  uint32 hcharNum; /* 半角文字数 */
  /* Number of Hankaku (half-width) characters in the font file. */
  /* Analagous to the number of ASCII characters. */

  uint32 zcharNum; /* 全角文字数 */
  /* Number of Zenkaka (full-width) characters in the font file. */
  /* Analogous to the number of S-JIS characters. */

  uint32 offsetHcodetbl; /* 半角縮退テーブルへのオフセット */
                         /* Offset to the Hankaku code table. */
                         /* Zero means no Hankaku code table in font file. */

  uint32 offsetZcodetbl; /* 全角縮退テーブルへのオフセット */
                         /* Offset to the Zenkaku code table. */
                         /* Zero means no Zenkaku code table in font file. */

  uint32 offsetHwidthtbl; /* 半角文字幅テーブルへのオフセット */
                          /* Offset to the Hankaku character width table. */
  /* Zero means no character width table in font file. */

  uint32 offsetZwidthtbl; /* 全角文字幅テーブルへのオフセット */
                          /* Offset to the Zenkaku character width table. */
  /* Zero means no character width table in font file. */

  uint32 offsetHbitmap; /* Offset to the Hankaku character bitmap data. */
                        /* Zero means no Hankaku bitmap data in font file. */

  uint32 offsetZbitmap; /* Offset to the Zenkaku character bitmap data. */
                        /* Zero means no Zenkaku bitmap data in font file. */

  uint32 offsetCopyright;    /* Offset to '/0' terminated Copyright string
                                (ASCII). */
  uint32 offsetTypeFaceName; /* Offset to '/0' terminated typeface name string
                                (ASCII). */
  uint32 revision;           /* Revision number of actual font bitmap data. */
  uint32 compression;        /* Compression type used for bitmap data. */
  uint32 charCode;           /* Character encoding scheme. */
  uint32 reserved1;          /* Reserved. */
  uint32 reserved2;          /* Reserved. */
  uint32 reserved3;          /* Reserved. */
  uint32 reserved4;          /* Reserved. */
} KFontFileHeader;

typedef struct
{
  KFontFileHeader *header; /* フォントファイルヘッダ */
  char *hcodeTbl;          /* 半角の文字コードテーブル */
} KFontDesc;

/*------------------------------------------------------------------------
 * Gobals.
 *----------------------------------------------------------------------*/

static uint8 MASK_TABLE[7] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

/*------------------------------------------------------------------------
 * makeElementPLUT()
 *	Calculate plut entries based upon levels of transparency.
 *----------------------------------------------------------------------*/

void
makeElementPLUT (frac16 *element, int32 fgElement, int32 bgElement)
{
  int32 diffElement, i, delta32;
  frac16 delta, bgf16, fgf16;

  diffElement = fgElement - bgElement;
  bgf16 = Convert32_F16 (bgElement);
  fgf16 = Convert32_F16 (fgElement);

  if (diffElement > 0)
    {
      delta = DivSF16 (Convert32_F16 (diffElement), Convert32_F16 (15));
      delta32 = ConvertF16_32 (delta);
      *element++ = bgf16;
      *element++ = bgf16 + delta;
      *element++ = bgf16 + Convert32_F16 (delta32 * 2);
      *element++ = bgf16 + Convert32_F16 (delta32 * 3);
      *element++ = bgf16 + Convert32_F16 (delta32 * 4);
      *element++ = bgf16 + Convert32_F16 (delta32 * 5);
      *element++ = bgf16 + Convert32_F16 (delta32 * 6);
      *element++ = bgf16 + Convert32_F16 (delta32 * 7);
      *element++ = fgf16 - Convert32_F16 (delta32 * 7);
      *element++ = fgf16 - Convert32_F16 (delta32 * 6);
      *element++ = fgf16 - Convert32_F16 (delta32 * 5);
      *element++ = fgf16 - Convert32_F16 (delta32 * 4);
      *element++ = fgf16 - Convert32_F16 (delta32 * 3);
      *element++ = fgf16 - Convert32_F16 (delta32 * 2);
      *element++ = fgf16 - delta;
      *element = fgf16;
    }
  else if (diffElement < 0)
    {
      delta
          = DivSF16 (Convert32_F16 (((-1) * diffElement)), Convert32_F16 (15));
      delta32 = ConvertF16_32 (delta);
      *element++ = bgf16;
      *element++ = bgf16 - delta;
      *element++ = bgf16 - Convert32_F16 (delta32 * 2);
      *element++ = bgf16 - Convert32_F16 (delta32 * 3);
      *element++ = bgf16 - Convert32_F16 (delta32 * 4);
      *element++ = bgf16 - Convert32_F16 (delta32 * 5);
      *element++ = bgf16 - Convert32_F16 (delta32 * 6);
      *element++ = bgf16 - Convert32_F16 (delta32 * 7);
      *element++ = fgf16 + Convert32_F16 (delta32 * 7);
      *element++ = fgf16 + Convert32_F16 (delta32 * 6);
      *element++ = fgf16 + Convert32_F16 (delta32 * 5);
      *element++ = fgf16 + Convert32_F16 (delta32 * 4);
      *element++ = fgf16 + Convert32_F16 (delta32 * 3);
      *element++ = fgf16 + Convert32_F16 (delta32 * 2);
      *element++ = fgf16 + delta;
      *element = fgf16;
    }
  else if (diffElement == 0)
    {
      for (i = 0; i < 16; i++)
        {
          *element++ = fgf16;
        }
    }
}

/*------------------------------------------------------------------------
 * KMakePLUT16()
 *	Given a foreground and background color, create a 4-bit plut
 *	with 16 levels of gray (transparency).
 *----------------------------------------------------------------------*/

int32
KMakePLUT16 (Color fgColor, Color bgColor, uint8 opaqueFlag,
             uint16 **thePLUTPtr)
{
  int32 i;
  uint16 newColor;
  int32 fgRed, fgGreen, fgBlue, bgRed, bgGreen, bgBlue;
  uint8 newRed, newGreen, newBlue;
  frac16 red[16], green[16], blue[16];

  if (!*thePLUTPtr)
    {
      *thePLUTPtr = (uint16 *)AllocMem (PLUTSIZE_4BIT_CODEDCEL, MEMTYPE_CEL);
      if (*thePLUTPtr == NULL)
        {
          DIAGNOSE (("Cannot allocate storage for PLUT of 4Bit Cel.\n"));
          return (ER_KTextBox_CannotMemAlloc);
        }
    }

  fgRed = ((fgColor & 0x00FF0000) >> 19);
  fgGreen = ((fgColor & 0xFF00) >> 11);
  fgBlue = ((fgColor & 0xFF) >> 3);
  bgRed = ((bgColor & 0x00FF0000) >> 19);
  bgGreen = ((bgColor & 0xFF00) >> 11);
  bgBlue = ((bgColor & 0xFF) >> 3);

  makeElementPLUT (red, fgRed, bgRed);
  makeElementPLUT (green, fgGreen, bgGreen);
  makeElementPLUT (blue, fgBlue, bgBlue);

  /* (CHG) 93082 */
  if (fgColor == 0x00000000)
    {
      blue[15] = Convert32_F16 (2);
    }
  /* compensation */
  for (i = 0; i < 16; i++)
    {
      if (blue[i] < Convert32_F16 (2))
        blue[i] = Convert32_F16 (2);
    }

  /* set color to PLUT */
  for (i = 0; i < 16; i++)
    {
      newRed = (uint8)ConvertF16_32 (red[i]);
      newGreen = (uint8)ConvertF16_32 (green[i]);
      newBlue = (uint8)ConvertF16_32 (blue[i]);
      newColor = (newRed << 10) | (newGreen << 5) | newBlue;
      *(*thePLUTPtr + i) = newColor & 0x7FFE;
    }
  if (!(opaqueFlag & fg_Opaque))
    *(*thePLUTPtr + 15) = 0x0000;
  if (!(opaqueFlag & bg_Opaque))
    *(*thePLUTPtr + 0) = 0x0000;

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KMakePLUT2()
 *	Given a foreground and background color, create a 1-bit
 *	black & white plut.
 *----------------------------------------------------------------------*/

int32
KMakePLUT2 (Color fgColor, Color bgColor, uint8 opaqueFlag,
            uint16 **thePLUTPtr)
{
  uint16 foreColor, backColor;
  uint8 red, green, blue, *byteptr;

  /* 1bitセルのPLUTを設定する */
  /* set foreground color */
  byteptr = (uint8 *)&fgColor;
  red = *(byteptr + 1);
  green = *(byteptr + 2);
  blue = *(byteptr + 3);
  foreColor
      = ((red & 0xf8) << 7) | ((green & 0xf8) << 2) | ((blue & 0xf8) >> 3);

  /* set background color */
  byteptr = (uint8 *)&bgColor;
  red = *(byteptr + 1);
  green = *(byteptr + 2);
  blue = *(byteptr + 3);
  backColor
      = ((red & 0xf8) << 7) | ((green & 0xf8) << 2) | ((blue & 0xf8) >> 3);

  if (!*thePLUTPtr)
    {
      *thePLUTPtr = (uint16 *)AllocMem (PLUTSIZE_1BIT_CODEDCEL, MEMTYPE_CEL);
      if (*thePLUTPtr == NULL)
        {
          DIAGNOSE (("Cannot allocate storage for PLUT of 1Bit Cel.\n"));
          return (ER_KTextBox_CannotMemAlloc);
        }
    }

  *(*thePLUTPtr + 0) = backColor;
  *(*thePLUTPtr + 1) = foreColor;
  if (!(opaqueFlag & bg_Opaque))
    *(*thePLUTPtr + 0) = 0x0000;
  if (!(opaqueFlag & fg_Opaque))
    *(*thePLUTPtr + 1) = 0x0000;

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * GetDeviceStatus()
 *	Fill out a DeviceStatus at the specified address, given an IOReq.
 *----------------------------------------------------------------------*/

Err
GetDeviceStatus (Item ioReq, DeviceStatus *aDeviceStatus)
{
  IOInfo aIOInfo;

  memset (&aIOInfo, 0, sizeof (IOInfo));
  memset (aDeviceStatus, 0, sizeof (DeviceStatus));
  aIOInfo.ioi_Command = CMD_STATUS;
  aIOInfo.ioi_Recv.iob_Buffer = aDeviceStatus;
  aIOInfo.ioi_Recv.iob_Len = sizeof (DeviceStatus);
  return DoIO (ioReq, &aIOInfo);
}

/*------------------------------------------------------------------------
 * OpenRomFile()
 *	Open the Kanji font file using the low-level IO commands.
 *	Note that this routine can't be used to access data on a CD
 *	since the CD's block size does not meet DoIO's minimum block size.
 *----------------------------------------------------------------------*/

int32
OpenRomFile (KFont3DO *theFont, char *name)
{
  static Item romfitem;
  static OpenFile *romfptr;
  uint32 romBlockSize = 0;
  DeviceStatus deviceStatus;

  romfitem = OpenDiskFile (name);
  romfptr = (OpenFile *)LookupItem (romfitem);
  theFont->romior = NULL;

  if (romfptr)
    {
      if ((theFont->romIOReqItem
           = CreateIOReq (NULL, 0, romfptr->ofi.dev.n_Item, 0))
          < 0)
        {
          DIAGNOSE_SYSERR (theFont->romIOReqItem, ("Can't create IOReq\n"));
          return -1;
        }
      GetDeviceStatus (theFont->romIOReqItem, &deviceStatus);
      romBlockSize = deviceStatus.ds_DeviceBlockSize;
      printf (" romBlockSize = %ld \n", romBlockSize);
      DIAGNOSE ((" ROM File Blocksize is %ld \n", romBlockSize));
    }
  theFont->logRomBlockSize = 0;
  while (((uint32)1 << theFont->logRomBlockSize) < romBlockSize)
    theFont->logRomBlockSize++;
  theFont->romior = (IOReq *)LookupItem (theFont->romIOReqItem);
  memset (&theFont->romioInfo, 0, sizeof (theFont->romioInfo));
  theFont->romioInfo.ioi_Command = CMD_READ;
  theFont->romioInfo.ioi_Send.iob_Buffer = NULL;
  theFont->romioInfo.ioi_Send.iob_Len = 0;
  theFont->romioInfo.ioi_Offset = 0;
  return (0);
}

/*------------------------------------------------------------------------
 * ReadRom()
 *	Read the Kanji font file using the low-level DoIO commands.
 *	Note that this routine can to be used to read data from the CD
 *	since the CD's block size does not meet DoIO's minimum block size.
 *----------------------------------------------------------------------*/

int32
ReadRom (KFont3DO *theFont, uint32 offset, char *buf, int32 len)
{
  int32 err;

  if (theFont->romIOReqItem == (Item)NULL)
    return -1;
  theFont->romioInfo.ioi_Offset = offset >> theFont->logRomBlockSize;
  theFont->romioInfo.ioi_Recv.iob_Buffer = buf;
  theFont->romioInfo.ioi_Recv.iob_Len = len;
  if ((err = DoIO (theFont->romIOReqItem, &theFont->romioInfo)) < 0)
    {
      PrintfSysErr (err);
      DeleteIOReq (theFont->romIOReqItem);
      DIAGNOSE (
          ("ReadRom:  Error in DoIO, offset = %ld ( %lx )\n", offset, offset));
      theFont->romIOReqItem = (Item)NULL;
      return -1;
    }
  return (len);
}

#define FS_CHUNKSIZE 65536

/*------------------------------------------------------------------------
 * ReadFile_T()
 *	Read the Kanji font file using the standard file system routines.
 *----------------------------------------------------------------------*/

int32
ReadFile_T (Stream *fs, int32 size, int32 *buffer, int32 offset)
{
  int32 retval;
  char *cbuf;
  int32 ntoread, nleft;
  int32 retries = 4;

  retval = SeekDiskStream (fs, offset, SEEK_SET);
  nleft = size;
  cbuf = (char *)buffer;
  if (retval >= 0)
    while (nleft > 0)
      {
        ntoread = ((FS_CHUNKSIZE >= nleft) ? nleft : FS_CHUNKSIZE);
        retval = ReadDiskStream (fs, cbuf, ntoread);
        if (retval < 0)
          {
            retries--;
            if (retries == 0)
              break;
          }
        else
          {
            nleft -= ntoread;
            cbuf += ntoread;
            retries = 4;
          }
      }
  return retval;
}

/*------------------------------------------------------------------------
 * ReadFile_E()
 *	If fsRead is TRUE read the Kanji using the standard file system
 *	calls; otherwise, read the Kanji using low-level I/O commands.
 *	Note that this library is currently set to perform ReadFile_T only.
 *----------------------------------------------------------------------*/

int32
ReadFile_E (KFont3DO *theFont, int32 size, int32 *buffer, int32 offset)
{
  if (theFont->fsRead)
    return ReadFile_T (theFont->font_fs, size, buffer, offset);
  else
    return ReadRom (theFont, offset, (char *)buffer, size);
}

/*------------------------------------------------------------------------
 * KLoadFontDesc()
 *	Create the font descriptor from the Kanji font file.
 *----------------------------------------------------------------------*/

int32
KLoadFontDesc (char *name, KFontDesc *fontDesc, KFont3DO *theFont,
               Boolean fullRead, Boolean fsRead)
{
  char fileName[64];
  uint32 dataSize, tmpSize;
  char *dataPtr;
  int32 error;
  int32 fseekOffset;
  int32 i;
  int32 pSize;

  /***********************************************************************\
   *	フォントファイルのヘッダ情報をロードする	                    *
   \***********************************************************************/

  /*------------------------------------------------------------------------
   * Allocate a KFontFileHeader, then load the font file's KFontFileHeader.
   *----------------------------------------------------------------------*/

  strcpy (fileName, (char *)name);

  /* KFontFileHeaderを読み込む */
  dataSize = sizeof (KFontFileHeader);

  /* KFontFileHeader を読み込むためのメモリを確保する */
  dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);

  if (dataPtr == NULL)
    {
      DIAGNOSE (("Cannot allocate storage for font file KFontFileHeader.\n"));
      return (ER_KTextBox_CannotMemAlloc);
    }

  fseekOffset = 0;
  theFont->fsRead = fsRead;

  /* ファイルオープン */

  /*------------------------------------------------------------------------
   * If fsRead is TRUE, open the file using the file system calls;
   *	otherwise, use the low-level IO calls.
   *----------------------------------------------------------------------*/

  if (theFont->fsRead)
    {
      theFont->font_fs = OpenDiskStream (fileName, 0);
      if (theFont->font_fs == NULL)
        {
          FreeMem (dataPtr, dataSize);
          DIAGNOSE (("Cannot open disk stream.\n"));
          return (ER_KTextBox_CannotOpenDS);
        }
    }
  else
    {
      if (OpenRomFile (theFont, fileName) < 0)
        {
          FreeMem (dataPtr, dataSize);
          DIAGNOSE (("Cannot open disk stream.\n"));
          return (ER_KTextBox_CannotOpenDS);
        }
    }

  /*------------------------------------------------------------------------
   * Read the font file header then offset the file pointer accordingly.
   *----------------------------------------------------------------------*/

  error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr, fseekOffset);
  if (error < 0)
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (dataPtr, dataSize);
      DIAGNOSE (("Error reading font file KFontFileHeader.\n"));
      return error;
    }

  fontDesc->header = (KFontFileHeader *)dataPtr;
  fseekOffset
      += sizeof (KFontFileHeader); /* MACのフォントファイルのヘッダの大きさ*/

  /*------------------------------------------------------------------------
   *	Set the FullRead flag, then store-off the offsets to the character
   *bitmaps.
   *----------------------------------------------------------------------*/

  theFont->font_FullRead = fullRead;
  theFont->font_HFSeekOffset = fontDesc->header->offsetHbitmap;
  theFont->font_ZFSeekOffset = fontDesc->header->offsetZbitmap;

  /***********************************************************************
   *	フォントファイルのヘッダ情報をチェックする *
   ***********************************************************************/

  /*------------------------------------------------------------------------
   * Perform some initial file validation checks before reading in the data.
   *	Check the max. char width, max. char height, bits/pixel, grayscale,
   *	total chars, and min/max char codes.
   *----------------------------------------------------------------------*/

  if ((fontDesc->header->fontCharWidth) == 0)
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error fontCharWidth is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  if (fontDesc->header->fontCharHeight == 0)
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error fontCharHeight is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  if (fontDesc->header->bitsPerPixel == 0)
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error bitsPerPixel is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  if (fontDesc->header->grayScaleBit == 0)
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error grayScaleBit is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  if ((fontDesc->header->hcharNum == 0) && (fontDesc->header->zcharNum == 0))
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error hcharNum and zcharNum is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  if (((fontDesc->header->hmincode == 0) || (fontDesc->header->hmaxcode == 0))
      && ((fontDesc->header->zmincode == 0)
          || (fontDesc->header->zmaxcode == 0)))
    {
      if (theFont->fsRead)
        CloseDiskStream (theFont->font_fs);
      FreeMem (fontDesc->header, sizeof (KFontFileHeader));
      DIAGNOSE (("Error minmaxcode is 0.\n"));
      return (ER_KTextBox_BadFontFile);
    }

  /*------------------------------------------------------------------------
   * If the font file contains Hankaku characters, check for a CODE
   *	TABLE and a CHARACTER WIDTH TABLE for the Hankaku character.
   *----------------------------------------------------------------------*/

  if (fontDesc->header->hcharNum > 0)
    {

      /***********************************************************************\
       *	半角のコードテーブルと文字幅テーブルをロードする *
       \***********************************************************************/

      /*------------------------------------------------------------------------
       * Check for a Hankaku code table.  If one exist, allocate some memory,
       *	load the table, then offset the file pointer.
       *----------------------------------------------------------------------*/

      if (fontDesc->header->offsetHcodetbl != 0)
        {
          /* 半角のコードテーブル用のメモリを確保する(テンポラリ) */
          dataSize = fontDesc->header->hcharNum;
          tmpSize = dataSize;
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Cannot allocate storage for font file hcodeTbl.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }
          fseekOffset = fontDesc->header->offsetHcodetbl;
          /* 半角のコードテーブル を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);
          if (error < 0)
            {
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Error reading font file hcodeTbl.\n"));
              return error;
            }
          fontDesc->hcodeTbl = (char *)dataPtr;

          /* 半角のコードテーブル用のメモリを確保する */
          dataSize = fontDesc->header->hcharNum * sizeof (uint16);
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->hcodeTbl, tmpSize);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Cannot allocate storage for font file "
                         "font_Hinfo.codeTbl.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }
          theFont->font_Hinfo.codeTbl = (uint16 *)dataPtr;
          /* KFont3DO構\造体に半角コードテーブル をセットする */
          for (i = 0; i < fontDesc->header->hcharNum; i++)
            {
              theFont->font_Hinfo.codeTbl[i] = (uint16)fontDesc->hcodeTbl[i];
            }
          FreeMem (fontDesc->hcodeTbl, tmpSize);
          fseekOffset += tmpSize;
        }

      /*------------------------------------------------------------------------
       * Check for a character width table.  If one exist, allocate some
       *memory, load the table, then offset the file pointer.
       *----------------------------------------------------------------------*/

      if (fontDesc->header->offsetHwidthtbl != 0)
        { // <HPP> for our sake this is ignored
          /* 半角の文字幅テーブル用のメモリを確保する */
          dataSize = fontDesc->header->hcharNum;
          tmpSize = dataSize;
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (fontDesc->header->offsetHcodetbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.codeTbl,
                           fontDesc->header->hcharNum * sizeof (uint16));
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (
                  ("Cannot allocate storage for font file hwidthTbl.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }
          fseekOffset = fontDesc->header->offsetHwidthtbl;

          /* 半角の文字幅テーブル を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);

          if (error < 0)
            {
              if (fontDesc->header->offsetHcodetbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.codeTbl,
                           fontDesc->header->hcharNum * sizeof (uint16));
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (dataPtr, tmpSize);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Error reading font file hwidthTbl.\n"));
              return error;
            }
          /* KFont3DO構\造体に文字幅テーブル をセットする */
          theFont->font_Hinfo.widthTbl = (uint8 *)dataPtr;
          fseekOffset += dataSize;
        }
    }

  /*------------------------------------------------------------------------
   * If the font file contains Zenkaku characters, check for a CODE
   *	TABLE and a CHARACTER WIDTH TABLE for the Hankaku character.
   *----------------------------------------------------------------------*/

  if (fontDesc->header->zcharNum > 0)
    {

      /***********************************************************************\
       *	全角のコードテーブルと文字幅テーブルをロードする *
       \***********************************************************************/

      /*------------------------------------------------------------------------
       * Check for a Zenkaku code table.  If one exist, allocate some memory,
       *	load the table, then offset the file pointer.
       *----------------------------------------------------------------------*/

      if (fontDesc->header->offsetZcodetbl != 0)
        { // <HPP> for our sake this is ignored
          /* 全角のコードテーブル用のメモリを確保する(テンポラリ) */
          dataSize = fontDesc->header->zcharNum * sizeof (uint16);
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Cannot allocate storage for font file zcodeTbl.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }
          fseekOffset = fontDesc->header->offsetZcodetbl;

          /* 全角のコードテーブル を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);

          if (error < 0)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              FreeMem (dataPtr, dataSize);
              DIAGNOSE (("Error reading font file zcodeTbl.\n"));
              return error;
            }
          /* KFont3DO構\造体に全角コードテーブル をセットする */
          theFont->font_Zinfo.codeTbl = (uint16 *)dataPtr;

          fseekOffset += dataSize;
        }

      /*------------------------------------------------------------------------
       * Check for a character width table.  If one exist, allocate some
       *memory, load the table, then offset the file pointer.
       *----------------------------------------------------------------------*/

      if (fontDesc->header->offsetZwidthtbl != 0)
        { // <HPP> for our sake this is ignored
          /* 全角の文字幅テーブル用のメモリを確保する */
          dataSize = fontDesc->header->zcharNum;
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                }
              if (fontDesc->header->offsetZcodetbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.codeTbl,
                           fontDesc->header->zcharNum * sizeof (uint16));
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (
                  ("Cannot allocate storage for font file zwidthTbl.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }
          fseekOffset = fontDesc->header->offsetZwidthtbl;

          /* 全角の文字幅テーブル を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);

          if (error < 0)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                }
              if (fontDesc->header->offsetZcodetbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.codeTbl,
                           fontDesc->header->zcharNum * sizeof (uint16));
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Error reading font file hwidthTbl.\n"));
              return error;
            }
          /* KFont3DO構\造体に文字幅テーブル をセットする */
          theFont->font_Zinfo.widthTbl = (uint8 *)dataPtr;
          fseekOffset += dataSize;
        }
    }

  /*------------------------------------------------------------------------
   * Now for any Hankaku characters calculate the size of each character's
   *	bitmap image.  Next if the FullRead flag is true, allocate and load
   *	in all the character bitmaps.
   *----------------------------------------------------------------------*/

  if (fontDesc->header->hcharNum > 0)
    {
      /* 半角のフォントデータ用のメモリを確保する */
      pSize = fontDesc->header->fontCharWidth
              * fontDesc->header->fontCharHeight
              * fontDesc->header->bitsPerPixel / 2;
      theFont->font_Hinfo.oneCharSize
          = (pSize + (((pSize + 31) / 32) * 32 - pSize)) / 8;

      if (theFont->font_FullRead)
        { // <HPP 03-08-94>
          dataSize
              = fontDesc->header->hcharNum * theFont->font_Hinfo.oneCharSize;
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (fontDesc->header->offsetHcodetbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.codeTbl,
                           fontDesc->header->hcharNum * sizeof (uint16));
                }
              if (fontDesc->header->offsetHwidthtbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.widthTbl,
                           fontDesc->header->hcharNum);
                }
              if (fontDesc->header->zcharNum > 0)
                {
                  if (fontDesc->header->offsetZcodetbl != 0)
                    {
                      FreeMem (theFont->font_Zinfo.codeTbl,
                               fontDesc->header->zcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetZwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Zinfo.widthTbl,
                               fontDesc->header->zcharNum);
                    }
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (
                  ("Cannot allocate storage for font file hcharData.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }

          /* 半角のフォントデータ を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);

          /*------------------------------------------------------------------------
           * If an error occurs when loading in the bitmap data, release any
           *	memory used for the Hankaku data.
           *----------------------------------------------------------------------*/

          if (error < 0)
            {
              if (fontDesc->header->offsetHcodetbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.codeTbl,
                           fontDesc->header->hcharNum * sizeof (uint16));
                }
              if (fontDesc->header->offsetHwidthtbl != 0)
                {
                  FreeMem (theFont->font_Hinfo.widthTbl,
                           fontDesc->header->hcharNum);
                }
              if (fontDesc->header->zcharNum > 0)
                {
                  if (fontDesc->header->offsetZcodetbl != 0)
                    {
                      FreeMem (theFont->font_Zinfo.codeTbl,
                               fontDesc->header->zcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetZwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Zinfo.widthTbl,
                               fontDesc->header->zcharNum);
                    }
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Error reading font file hcharData.\n"));
              return error;
            }

          theFont->font_Hinfo.charData = (char *)dataPtr;
          fseekOffset += dataSize;
        }
    }

  /*------------------------------------------------------------------------
   * Now for any Zenkaku characters calculate the size of each character's
   *	bitmap image.  Next if the FullRead flag is true, allocate and load
   *	in all the character bitmaps.
   *----------------------------------------------------------------------*/

  if (fontDesc->header->zcharNum > 0)
    {
      /* 全角のフォントデータ用のメモリを確保する */
      pSize = fontDesc->header->fontCharHeight
              * fontDesc->header->fontCharWidth
              * fontDesc->header->bitsPerPixel;
      theFont->font_Zinfo.oneCharSize
          = (pSize + (((pSize + 31) / 32) * 32 - pSize)) / 8;

      if (theFont->font_FullRead)
        { // <HPP 03-08-94>

          dataSize
              = fontDesc->header->zcharNum * theFont->font_Zinfo.oneCharSize;
          dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
          if (dataPtr == NULL)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                  FreeMem (theFont->font_Hinfo.charData,
                           fontDesc->header->hcharNum
                               * theFont->font_Hinfo.oneCharSize);
                }
              if (fontDesc->header->offsetZcodetbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.codeTbl,
                           fontDesc->header->zcharNum * sizeof (uint16));
                }
              if (fontDesc->header->offsetZwidthtbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.widthTbl,
                           fontDesc->header->zcharNum);
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (
                  ("Cannot allocate storage for font file zcharData.\n"));
              return (ER_KTextBox_CannotMemAlloc);
            }

          /* 全角のフォントデータ を読み込む */
          error = ReadFile_E (theFont, (int)dataSize, (int32 *)dataPtr,
                              fseekOffset);

          /*------------------------------------------------------------------------
           * If an error occurs when loading in the bitmap data, release any
           *	memory used for the Zenkaku data.
           *----------------------------------------------------------------------*/

          if (error < 0)
            {
              if (fontDesc->header->hcharNum > 0)
                {
                  if (fontDesc->header->offsetHcodetbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.codeTbl,
                               fontDesc->header->hcharNum * sizeof (uint16));
                    }
                  if (fontDesc->header->offsetHwidthtbl != 0)
                    {
                      FreeMem (theFont->font_Hinfo.widthTbl,
                               fontDesc->header->hcharNum);
                    }
                  FreeMem (theFont->font_Hinfo.charData,
                           fontDesc->header->hcharNum
                               * theFont->font_Hinfo.oneCharSize);
                }
              if (fontDesc->header->offsetZcodetbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.codeTbl,
                           fontDesc->header->zcharNum * sizeof (uint16));
                }
              if (fontDesc->header->offsetZwidthtbl != 0)
                {
                  FreeMem (theFont->font_Zinfo.widthTbl,
                           fontDesc->header->zcharNum);
                }
              if (theFont->fsRead)
                CloseDiskStream (theFont->font_fs);
              FreeMem (theFont->font_Zinfo.charData,
                       fontDesc->header->zcharNum
                           * theFont->font_Zinfo.oneCharSize);
              FreeMem (fontDesc->header, sizeof (KFontFileHeader));
              DIAGNOSE (("Error reading font file zcharData.\n"));
              return error;
            }

          theFont->font_Zinfo.charData = (char *)dataPtr;
          if (theFont->fsRead)
            CloseDiskStream (theFont->font_fs);
        }
    }

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KLoadFont()
 *	Load the Kanji font file.
 *----------------------------------------------------------------------*/

int32
KLoadFont (char *name, KFont3DO *theFont, Boolean fullRead)
{
  KFontDesc theFontDesc;
  int32 ret;

  /***********************************************************************\
   *	Font3DO構\造体を初期化する			                     *
   \***********************************************************************/

  /*------------------------------------------------------------------------
   * Initialize the font descriptor fields.
   *----------------------------------------------------------------------*/

  theFont->font_Gpp = 0;
  theFont->font_Bpp = 0;

  theFont->font_Hinfo.nChars = 0;
  theFont->font_Hinfo.minCode = 0;
  theFont->font_Hinfo.maxCode = 0;
  theFont->font_Hinfo.charHeight = 0;
  theFont->font_Hinfo.charWidth = 0;
  theFont->font_Hinfo.oneCharSize = 0;
  theFont->font_Hinfo.codeTbl = (uint16 *)0;
  theFont->font_Hinfo.widthTbl = (uint8 *)0;
  theFont->font_Hinfo.charData = (char *)0;

  theFont->font_Zinfo.nChars = 0;
  theFont->font_Zinfo.minCode = 0;
  theFont->font_Zinfo.maxCode = 0;
  theFont->font_Zinfo.charHeight = 0;
  theFont->font_Zinfo.charWidth = 0;
  theFont->font_Zinfo.oneCharSize = 0;
  theFont->font_Zinfo.codeTbl = (uint16 *)0;
  theFont->font_Zinfo.widthTbl = (uint8 *)0;
  theFont->font_Zinfo.charData = (char *)0;

  theFont->maxCharSize = 0;

  /***********************************************************************\
   *	KFontDesc構\造体へデータをロードする		                     *
   \***********************************************************************/

  /*------------------------------------------------------------------------
   * Load the font descriptor.
   *----------------------------------------------------------------------*/

  ret = KLoadFontDesc (name, &theFontDesc, theFont, fullRead, true);
  if (ret < 0)
    return (ret);

  /***********************************************************************\
   *	残りのデータをセットする			                    *
   \***********************************************************************/

  theFont->font_Gpp = (uint8)theFontDesc.header->grayScaleBit;
  theFont->font_Bpp = (uint8)theFontDesc.header->bitsPerPixel;

  /*------------------------------------------------------------------------
   * If the font file contains Hankaku characters, then fill in the
   *	corresponding Hankaku fields.
   *----------------------------------------------------------------------*/

  if (theFontDesc.header->hcharNum > 0)
    {
      theFont->font_Hinfo.nChars = (uint16)theFontDesc.header->hcharNum;
      theFont->font_Hinfo.minCode = (uint16)theFontDesc.header->hmincode;
      theFont->font_Hinfo.maxCode = (uint16)theFontDesc.header->hmaxcode;
      theFont->font_Hinfo.charHeight
          = (uint8)theFontDesc.header->fontCharHeight;
      theFont->font_Hinfo.charWidth
          = (uint8)theFontDesc.header->fontCharWidth / 2;
      if (theFont->font_Hinfo.oneCharSize > theFont->maxCharSize)
        theFont->maxCharSize = theFont->font_Hinfo.oneCharSize;
    }

  /*------------------------------------------------------------------------
   * If the font file contains Zenkaku characters, then fill in the
   *	corresponding Zenkaku fields.
   *----------------------------------------------------------------------*/

  if (theFontDesc.header->zcharNum > 0)
    {
      theFont->font_Zinfo.nChars = (uint16)theFontDesc.header->zcharNum;
      theFont->font_Zinfo.minCode = (uint16)theFontDesc.header->zmincode;
      theFont->font_Zinfo.maxCode = (uint16)theFontDesc.header->zmaxcode;
      theFont->font_Zinfo.charHeight
          = (uint8)theFontDesc.header->fontCharHeight;
      theFont->font_Zinfo.charWidth = (uint8)theFontDesc.header->fontCharWidth;
      if (theFont->font_Zinfo.oneCharSize > theFont->maxCharSize)
        theFont->maxCharSize = theFont->font_Zinfo.oneCharSize;
    }

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KFreeFont()
 *	Release any memory used by the Kanji font.
 *----------------------------------------------------------------------*/

int32
KFreeFont (KFont3DO *theFont)
{
  /* 半角のコードテーブルと幅テーブルとピクセルデータ部を解放する */

  /*------------------------------------------------------------------------
   * If any Hankaku information has been allocated, free it.
   *----------------------------------------------------------------------*/

  if (theFont->font_Hinfo.codeTbl != NULL)
    {
      FreeMem (theFont->font_Hinfo.codeTbl,
               (int32)theFont->font_Hinfo.nChars * sizeof (uint16));
      theFont->font_Hinfo.codeTbl = (uint16 *)0;
    }
  if (theFont->font_Hinfo.widthTbl != NULL)
    {
      FreeMem (theFont->font_Hinfo.widthTbl,
               (int32)theFont->font_Hinfo.nChars * sizeof (uint16));
      theFont->font_Hinfo.widthTbl = (uint8 *)0;
    }

  FreeMem (theFont->font_Hinfo.charData,
           theFont->font_Hinfo.nChars * theFont->font_Hinfo.oneCharSize);
  theFont->font_Hinfo.charData = (char *)0;

  /* 全角のコードテーブルと幅テーブルとピクセルデータ部を解放する */

  /*------------------------------------------------------------------------
   * If any Zenkaku information has been allocated, free it.
   *----------------------------------------------------------------------*/

  if (theFont->font_Zinfo.codeTbl != NULL)
    {
      FreeMem (theFont->font_Zinfo.codeTbl,
               (int32)theFont->font_Zinfo.nChars * sizeof (uint16));
      theFont->font_Zinfo.codeTbl = (uint16 *)0;
    }
  if (theFont->font_Zinfo.widthTbl != NULL)
    {
      FreeMem (theFont->font_Zinfo.widthTbl,
               (int32)theFont->font_Zinfo.nChars * sizeof (uint16));
      theFont->font_Zinfo.widthTbl = (uint8 *)0;
    }

  FreeMem (theFont->font_Zinfo.charData,
           theFont->font_Zinfo.nChars * theFont->font_Zinfo.oneCharSize);
  theFont->font_Zinfo.charData = (char *)0;

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KFreeCel()
 *	Release any memory used to create the Kanji text cel.
 *----------------------------------------------------------------------*/

int32
KFreeCel (CCB *theCCB)
{
  int32 boxRowBytes;
  int32 boxHeight;
  int32 pixelDepth;

  if (theCCB == NULL)
    {
      DIAGNOSE (("theCCB is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }

  /* セルの深さを求める*/
  pixelDepth = theCCB->ccb_PRE0 & 0x07;
  /* PLUTが占有するメモリを解放する。 */
  if (pixelDepth == PRE0_BPP_4)
    {
      /* ４ビットセルの場合 */
      FreeMem (theCCB->ccb_PLUTPtr, PLUTSIZE_4BIT_CODEDCEL);
      boxRowBytes = ((((theCCB->ccb_Width + 1) / 2) + 3) / 4) * 4;
    }
  else if (pixelDepth == PRE0_BPP_1)
    {
      /* １ビットセルの場合 */
      FreeMem (theCCB->ccb_PLUTPtr, PLUTSIZE_1BIT_CODEDCEL);
      boxRowBytes = ((((theCCB->ccb_Width + 7) / 8) + 3) / 4) * 4;
    }
  else
    {
      /* 予\期していないセルが与えられた */
      DIAGNOSE (("Bad cel type.\n"));
      return (ER_KTextBox_BadParameter);
    }

  if (boxRowBytes < 8)
    boxRowBytes = 8;
  boxHeight = theCCB->ccb_Height;
  /* ピクセルデータが占有するメモリを解放する。 */
  FreeMem (theCCB->ccb_SourcePtr, boxHeight * boxRowBytes);
  /* CCBが占有するメモリを解放する。 */
  FreeMem (theCCB, sizeof (CCB));

  /* メモリの解放に成功 */
  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KConvertText()
 *	Convert the input string to a double-byte text string.
 *----------------------------------------------------------------------*/

int32
KConvertText (uint8 *src, KCharRec **dst, int32 len)
{
  int32 dataSize;
  int32 i;

  if (*src == '\0')
    {
      DIAGNOSE (("Error: Target text is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (len <= 0)
    {
      DIAGNOSE (("Error: Convert len is less than zero.\n"));
      return (ER_KTextBox_BadParameter);
    }

  dataSize = sizeof (KCharRec) * len;
  *dst = (KCharRec *)AllocMem (dataSize, MEMTYPE_CEL);
  if (*dst == NULL)
    {
      DIAGNOSE (("Cannot allocate storage for convert text.\n"));
      return (ER_KTextBox_CannotMemAlloc);
    }

  for (i = 0; i < len; i++)
    {
      if (isKanji (*src))
        {
          /* シフトJISコードの第１バイトである */
          (*dst + i)->Value = ((int32)*src++ << 8);
          if (isKanji2 (*src))
            {
              /* シフトJISコードの第２バイトである */
              (*dst + i)->Value += *src++;
            }
          else
            {
              PRT (("ERROR:the character(%x) is not a shift jis code.\n",
                    *(Int16 **)(src - 1)));
              return i;
            }
        }
      else if (isANK (*src) || (*src == LF) || (*src == CR))
        {
          /* ASCIIコードである */
          (*dst + i)->Value = *src++;
        }
      else
        {
          PRT (("ERROR:the character(%x) is not a shift jis code.\n", *src));
          return i;
        }
    }

  return i;
}

/*------------------------------------------------------------------------
 * KTextLength()
 *	Returns the length of the text string.
 *----------------------------------------------------------------------*/

int32
KTextLength (uint8 *theText)
{
  int32 n = 0;

  if (*theText == '\0')
    {
      DIAGNOSE (("Error: Terget Text is NULL.\n"));
      return (0);
    }

  for (; *theText != '\0'; theText++)
    {
      if (isKanji (*theText))
        {
          /* シフトJISコードの第１バイトである */
          theText++;
          if (isKanji2 (*theText))
            {
              /* シフトJISコードの第２バイトである */
              n++;
            }
          else
            {
              PRT (("ERROR:the character(%x) is not a shift jis code.\n",
                    *(uint16 *)(--theText)));
              return n;
            }
        }
      else if (isANK (*theText) || (*theText == LF) || (*theText == CR))
        {
          /* ASCIIコードである */
          n++;
        }
      else
        {
          PRT (("ERROR:the character(%x) is not a shift jis code.\n",
                *theText));
          return n;
        }
    }

  return n;
}

/*------------------------------------------------------------------------
 * KTextLength2()
 *	Returns the length of the text string for nbytes.
 *----------------------------------------------------------------------*/

int32
KTextLength2 (uint8 *theText, int32 nbytes)
{
  int32 n = 0;

  if (*theText == '\0')
    {
      DIAGNOSE (("Error: Terget Text is NULL.\n"));
      return (0);
    }
  if (nbytes > strlen (theText))
    {
      DIAGNOSE (("Error: nbytes is too large.\n"));
      return (0);
    }

  for (; nbytes; theText++, nbytes--)
    {
      if (isKanji (*theText))
        {
          /* シフトJISコードの第１バイトである */
          theText++;
          nbytes--;
          if (isKanji2 (*theText))
            {
              /* シフトJISコードの第２バイトである */
              n++;
            }
          else
            {
              PRT (("ERROR:the character(%x) is not a shift jis code.\n",
                    *(uint16 *)(--theText)));
              return n;
            }
        }
      else if (isANK (*theText) || (*theText == LF) || (*theText == CR))
        {
          /* ASCIIコードである */
          n++;
        }
      else
        {
          PRT (("ERROR:the character(%x) is not a shift jis code.\n",
                *theText));
          return n;
        }
    }

  return n;
}

/*------------------------------------------------------------------------
 * KsjisToiCode()
 *	Converts the S-JIS character index to the equilvalent index into
 *	the Kanji font file.
 *----------------------------------------------------------------------*/

int32
KsjisToiCode (int32 c)
{
  int32 high;
  int32 low;
  int32 iCode = 0;

  c &= 0xffff;
  high = ((c & 0xFF00) >> 8);
  low = (c & 0x00FF);
  if (!(SJIS_KANJI_HIGH (high) && SJIS_KANJI_LOW (low)))
    {
      PRT (("ERROR:the character(%x) is not a shift jis code.\n", c));
      return (ER_KTextBox_BadCharCode);
    }

  if (!SJIS_KANJI_LEVEL2 (c))
    {
      if ((0x8140 <= c) && (c <= 0x84fc))
        {
          if (low <= 0x7e)
            {
              iCode = (high - 0x81) * 188 + low - 64; // - 0x40
            }
          else if (0x80 <= low)
            {
              iCode = (high - 0x81) * 188 + low - 65; // -0x7f + 62
            }
        }
      else if ((0x889f <= c) && (c <= 0x88fc))
        {
          iCode = c - 34223; // - 0x889e + 751
        }
      else if ((0x8940 <= c) && (c <= 0x987e))
        {
          if (low <= 0x7e)
            {
              iCode = (high - 0x89) * 188 + low + 782; // -0x3f + 845
            }
          else if (0x80 <= low)
            {
              iCode = (high - 0x89) * 188 + low + 781; // -0x7f + 845 + 63;
            }
        }
    }
  else
    {
      if ((0x989f <= c) && (c <= 0x98fc))
        {
          iCode = NUM_KANJI_LEVEL1 + low - 159; // -1 - 0x9e;
        }
      else if ((0x9940 <= c) && (c <= 0x9ffc))
        {
          if (low <= 0x7e)
            {
              iCode = (high - 0x99) * 188 + low
                      + 3790; // + NUM_KANJI_LEVEL1 -1 +94 - 0x3f;
            }
          else if (0x80 <= low)
            {
              iCode = (high - 0x99) * 188 + low
                      + 3789; // + NUM_KANJI_LEVEL1 -1 +94 - 0x7f + 63;
            }
        }
      else if ((0xe040 <= c) && (c <= 0xeafc))
        {
          if (low <= 0x7e)
            {
              iCode = (high - 0xe0) * 188 + low
                      + 5106; // + NUM_KANJI_LEVEL1-1 +1410 - 0x3f;
            }
          else if (0x80 <= low)
            {
              iCode = (high - 0xe0) * 188 + low
                      + 5105; // + NUM_KANJI_LEVEL1 -1 +1410 - 0x7f + 63;
            }
        }
    }

  return (iCode);
}

/*------------------------------------------------------------------------
 * KankToiCode()
 *	Converts the Katakana character index to the equilvalent index into
 *	the Kanji font file.
 *----------------------------------------------------------------------*/

int32
KankToiCode (int32 c)
{
  if (!isANK (c))
    {
      PRT (("ERROR:the character(%x) is not a ANK code.\n", c));
      return (ER_KTextBox_BadCharCode);
    }

  if ((0x20 <= (c) && (c) <= 0x7E))
    {
      return (c - SPACE);
    }
  else if (isKana (c))
    {
      return (c - KANA_START + (DEL - SPACE));
    }
  return (ER_KTextBox_BadCharCode);
}

/*------------------------------------------------------------------------
 * KCreate4BitCodedCel()
 *	Create a 4-bit coded cel for displaying 4-bit Kanji data.
 *----------------------------------------------------------------------*/

CCB *
KCreate4BitCodedCel (uint32 numRows, uint32 width, int32 rowBytes)
{
  CCB *newCCB;
  int32 scanlinesizeinwords;
  char *celData;
  int32 celDataSize;

  newCCB = (CCB *)AllocMem (sizeof (CCB), MEMTYPE_CEL);
  if (newCCB == NULL)
    {
      DIAGNOSE (("Can't allocate storage for font ccb\n"));
      return NULL;
    }
  /* Fill in the values */
  memset (newCCB, 0, sizeof (CCB));
  newCCB->ccb_HDX = (1 << 20);
  newCCB->ccb_VDY = (1 << 16);
  newCCB->ccb_Width = width;
  newCCB->ccb_Height = numRows;
  newCCB->ccb_Flags = CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE
                      | CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE | CCB_YOXY
                      | CCB_LDPLUT | CCB_ACW | CCB_ACCW;
  newCCB->ccb_PIXC = 0x1F001F00;
  newCCB->ccb_PLUTPtr = NULL;
  newCCB->ccb_PRE0 = ((numRows - 1) << PRE0_VCNT_SHIFT) | PRE0_BPP_4;
  scanlinesizeinwords = ((rowBytes + 3) / 4) - 2;
  if (scanlinesizeinwords < 0)
    scanlinesizeinwords = 0;
  newCCB->ccb_PRE1 = (scanlinesizeinwords << PRE1_WOFFSET8_SHIFT)
                     | ((width - 1) << PRE1_TLHPCNT_SHIFT);
  celDataSize = rowBytes * numRows;
  celData = (char *)AllocMem (celDataSize, MEMTYPE_CEL);
  if (celData == NULL)
    {
      FreeMem (newCCB, sizeof (CCB));
      DIAGNOSE (("Cannot allocate storage for font ccb\n"));
      return NULL;
    }
  memset (celData, 0, celDataSize);
  newCCB->ccb_SourcePtr = (CelData *)celData;

  return newCCB;
}

/*------------------------------------------------------------------------
 * KCreate1BitCodedCel()
 *	Create a 1-bit coded cel for displaying 1-bit Kanji data.
 *----------------------------------------------------------------------*/

CCB *
KCreate1BitCodedCel (uint32 numRows, uint32 width, int32 rowBytes)
{
  CCB *newCCB;
  int32 scanlinesizeinwords;
  char *celData;
  int32 celDataSize;

  newCCB = (CCB *)AllocMem (sizeof (CCB), MEMTYPE_CEL);
  if (newCCB == NULL)
    {
      DIAGNOSE (("Cannot allocate storage for font ccb\n"));
      return NULL;
    }
  /* Fill in the values */
  memset (newCCB, 0, sizeof (CCB));
  newCCB->ccb_HDX = (1 << 20);
  newCCB->ccb_VDY = (1 << 16);
  newCCB->ccb_Width = width;
  newCCB->ccb_Height = numRows;
  newCCB->ccb_Flags = CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE
                      | CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE | CCB_YOXY
                      | CCB_LDPLUT | CCB_ACW | CCB_ACCW;
  newCCB->ccb_PIXC = 0x1F001F00;
  newCCB->ccb_PRE0 = ((numRows - 1) << PRE0_VCNT_SHIFT) | PRE0_BPP_1;
  scanlinesizeinwords = ((rowBytes + 3) / 4) - 2;
  if (scanlinesizeinwords < 0)
    scanlinesizeinwords = 0;
  newCCB->ccb_PRE1 = (scanlinesizeinwords << PRE1_WOFFSET8_SHIFT)
                     | ((width - 1) << PRE1_TLHPCNT_SHIFT);
  celDataSize = rowBytes * numRows;
  celData = (char *)AllocMem (celDataSize, MEMTYPE_CEL);
  if (celData == NULL)
    {
      FreeMem (newCCB, sizeof (CCB));
      DIAGNOSE (("Cannot allocate storage for font ccb\n"));
      return NULL;
    }
  memset (celData, 0, celDataSize);
  newCCB->ccb_SourcePtr = (CelData *)celData;

  return newCCB;
}

/*------------------------------------------------------------------------
 * KBinarySearch()
 *	Use a binary search to find the table index for
 *	a given character value.
 *----------------------------------------------------------------------*/

int32
KBinarySearch (KFontRec *thisFont, uint16 charVal, uint16 n)
{
  int32 mid, left, right;

  left = 0;
  right = ((int32)n) - 1;

  if (thisFont->codeTbl[left] == charVal)
    return left;

  while (left < right)
    {
      mid = (left + right) / 2;
      if (thisFont->codeTbl[mid] < charVal)
        {
          left = mid + 1;
        }
      else
        {
          right = mid;
        }
    }

  if (thisFont->codeTbl[left] == charVal)
    return left;

  return (ER_KTextBox_NotFound);
}

/*------------------------------------------------------------------------
 * KGetCharIndex()
 *	Return the table index for a given character value.
 *----------------------------------------------------------------------*/

int32
KGetCharIndex (KFont3DO *theFont, uint16 charVal)
{
  int32 i = 0;
  int32 min, max;

  /* フォントファイルが縮退フォントの場合 */
  if (isANK (charVal))
    {
      if (theFont->font_Hinfo.codeTbl == NULL)
        {
          /* minとmaxはankであることが保証されている */
          min = KankToiCode (theFont->font_Hinfo.minCode);
          max = KankToiCode (theFont->font_Hinfo.maxCode);
          i = KankToiCode (charVal);
          if (i < 0)
            {
              PRT (("ERROR:the character(%x) is not a ank code.\n", charVal));
              return 0;
            }
          else if (min <= i && i <= max)
            {
              return (i - min);
            }
          else
            {
              PRT (
                  ("ERROR:the character(%x) is not a part of this fontfile.\n",
                   charVal));
              return 0;
            }
        }
      else
        {
          i = KBinarySearch (&theFont->font_Hinfo, charVal,
                             theFont->font_Hinfo.nChars);
          if (i < 0)
            {
              PRT (
                  ("ERROR:the character(%x) is not a part of this fontfile.\n",
                   charVal));
              return 0;
            }
          return i;
        }
    }
  else
    {
      if (theFont->font_Zinfo.codeTbl == NULL)
        {
          /* minとmaxはsjisであることが保証されている */
          min = KsjisToiCode (theFont->font_Zinfo.minCode);
          max = KsjisToiCode (theFont->font_Zinfo.maxCode);
          i = KsjisToiCode (charVal);
          if (i < 0)
            {
              PRT (("ERROR:the character(%x) is not a sjis code.\n", charVal));
              return 0;
            }
          else if (min <= i && i <= max)
            {
              return (i - min);
            }
          else
            {
              PRT (
                  ("ERROR:the character(%x) is not a part of this fontfile.\n",
                   charVal));
              return 0;
            }
        }
      else
        {
          i = KBinarySearch (&theFont->font_Zinfo, charVal,
                             theFont->font_Zinfo.nChars);
          if (i < 0)
            {
              PRT (
                  ("ERROR:the character(%x) is not a part of this fontfile.\n",
                   charVal));
              return 0;
            }

          return i;
        }
    }
}

/*------------------------------------------------------------------------
 * KGetCharWidth()
 *	Return the character width (pixels) for a given character value.
 *----------------------------------------------------------------------*/

int32
KGetCharWidth (KFont3DO *theFont, uint16 charVal)
{
  int32 i;

  if (isANK (charVal))
    {
      if (theFont->font_Hinfo.nChars <= 0)
        {
          PRT (("ERROR:the character(%x) is not a part of this fontfile.\n",
                charVal));
          return 0;
        }
      if (theFont->font_Hinfo.widthTbl == NULL)
        return theFont->font_Hinfo.charWidth;
      i = KGetCharIndex (theFont, charVal);
      return (int32)theFont->font_Hinfo.widthTbl[i];
    }
  else
    {
      if (theFont->font_Zinfo.nChars <= 0)
        {
          PRT (("ERROR:the character(%x) is not a part of this fontfile.\n",
                charVal));
          return 0;
        }
      if (theFont->font_Zinfo.widthTbl == NULL)
        return theFont->font_Zinfo.charWidth;
      i = KGetCharIndex (theFont, charVal);
      return (int32)theFont->font_Zinfo.widthTbl[i];
    }
}

/*------------------------------------------------------------------------
 * KGetCharHeight()
 *	Return the character height (pixels) for a given character value.
 *----------------------------------------------------------------------*/

int32
KGetCharHeight (KFont3DO *theFont, uint16 charVal)
{
  if (isANK (charVal))
    {
      if (theFont->font_Hinfo.nChars <= 0)
        {
          PRT (("ERROR:the character(%x) is not a part of this fontfile.\n",
                charVal));
          return 0;
        }
      return (int32)theFont->font_Hinfo.charHeight;
    }
  else
    {
      if (theFont->font_Zinfo.nChars <= 0)
        {
          PRT (("ERROR:the character(%x) is not a part of this fontfile.\n",
                charVal));
          return 0;
        }
      return (int32)theFont->font_Zinfo.charHeight;
    }
}

/*------------------------------------------------------------------------
 * KGetLineChars()
 *	Given a text string, return the maximum number of characters
 *	that will fit within a specified bow width.
 *----------------------------------------------------------------------*/

int32
KGetLineChars (KFont3DO *theFont, KTextBox *theTextBox, int32 boxWidth,
               KCharRec **lineStart, int32 *textLeft, int32 *lineHeight,
               int32 *lineTextWidth)
{
  int32 lineChars;
  int32 wordChars; /* １ワードの文字数 */
  int32 wordWidth; /* ワード幅 */
  int32 charWidth;
  bool roomLeft; /* ワードを入れる空きがあるかどうか */
  bool startofline;
  bool firstWord;
  int32 widthLeft;   /* 描画可能\な残りの幅*/
  KCharRec *curChar; /* 現在処理中の文字 */

  firstWord = TRUE;
  startofline = TRUE;
  roomLeft = TRUE;
  lineChars = 0;
  wordChars = 0;
  wordWidth = 0;
  *lineTextWidth = 0;
  widthLeft = boxWidth;
  curChar = *lineStart;

  /* ワードラップは、3DOFont英数字のみ有効*/

  if (theFont->font_Hinfo.nChars > 0)
    { /*文字列の先頭に改行コードが入っていた場合のlineHeightを設定する。*/
      *lineHeight = (int32)theFont->font_Hinfo.charHeight;
    }
  else if (theFont->font_Zinfo.nChars > 0)
    {
      *lineHeight = (int32)theFont->font_Zinfo.charHeight;
    }
  while ((*textLeft > 0) && (widthLeft > 0) && (curChar->Value != CR)
         && (curChar->Value != LF))
    {
      charWidth = KGetCharWidth (theFont, (uint16)curChar->Value);
      *lineHeight = KGetCharHeight (theFont, (uint16)curChar->Value);
      if (charWidth > widthLeft)
        {
          widthLeft = 0;
        }
      else
        {
          (*textLeft)--;
          lineChars++;
          curChar++;
          *lineTextWidth += (charWidth + theTextBox->charPitch);
          widthLeft -= (charWidth + theTextBox->charPitch);
        }
    }

  return lineChars;
}

/*------------------------------------------------------------------------
 * KDrawChar4BitCel()
 *	Given a text string, build the Kanji text into a 4-bit cel.
 *----------------------------------------------------------------------*/

int32
KDrawChar4BitCel (KFont3DO *theFont, uint16 charVal, int32 x, int32 y,
                  int32 celByteWidth, CCB *fourBitCel, char *dataPtr)
{
  int32 curX, curY;
  int32 i, j, k;
  int32 xDiff, yDiff;
  int32 startj, endj, startk, endk;
  int32 charWidth, charHeight;
  int32 bitStart;
  char *pCelData, *pImageData, imageByte;
  bool skipChar;
  int32 pixelDepth;
  uint8 lineData[128];
  int32 loop;
  int32 fseekOffset, oneCharSize;
  int32 error;

  if (theFont == NULL)
    {
      DIAGNOSE (("theFont is NULL.\n"));
      return x;
    }

  if (fourBitCel == NULL)
    {
      DIAGNOSE (("fourBitCel is NULL.\n"));
      return x;
    }

  /* 4Bitセル以外はエラー */
  pixelDepth = fourBitCel->ccb_PRE0 & 0x07;
  if (pixelDepth != PRE0_BPP_4)
    {
      DIAGNOSE (("ERROR:This cel's pixel depth isn't PRE0_BPP_4.\n"));
      return x;
    }

  charWidth = KGetCharWidth (theFont, (uint16)charVal);
  if (charWidth == 0)
    return x;
  charHeight = KGetCharHeight (theFont, (uint16)charVal);
  if (charHeight == 0)
    return x;
  skipChar = false;
  startk = 0;
  startj = 0;
  endj = charHeight;

  /* フォントの大きさがセルより大きい */
  if ((charWidth > fourBitCel->ccb_Width)
      || (charHeight > fourBitCel->ccb_Height))
    {
      DIAGNOSE (("ERROR:charWidth or charHeight is greater than cel size."));
      return x;
    }

  curX = x;
  curY = y - charHeight;

  xDiff = curX - ConvertF16_32 (fourBitCel->ccb_XPos);
  yDiff = curY - ConvertF16_32 (fourBitCel->ccb_YPos);

  if (yDiff < 0)
    {
      /* 書き始めが上端からはみ出る場合 */
      if (yDiff + charHeight <= 0)
        skipChar = true;
      else
        startj = -yDiff;
      yDiff = 0;
    }
  else if (yDiff + charHeight > fourBitCel->ccb_Height)
    {
      /* 書き終わりが下端からはみ出る場合 */
      if (yDiff >= fourBitCel->ccb_Height)
        skipChar = true;
      else
        endj = fourBitCel->ccb_Height - yDiff;
    }

  if (xDiff < 0)
    {
      /* 書き始めが左端からはみ出る場合 */
      if (xDiff + charWidth <= 0)
        skipChar = true;
      else
        startk = -xDiff / 2;
      xDiff = 0;
    }
  else if (xDiff + charWidth > fourBitCel->ccb_Width)
    {
      /* 書き終わりが右端からはみ出る場合 */
      if (xDiff >= fourBitCel->ccb_Width)
        skipChar = true;
      else
        endk = (fourBitCel->ccb_Width - xDiff) / 2;
    }

  if (!skipChar)
    {
      bitStart = xDiff & 0x01;
      /* point to right position in celdata */
      pCelData = (char *)fourBitCel->ccb_SourcePtr
                 + ((xDiff / 2) + (celByteWidth * yDiff));

      i = KGetCharIndex (theFont, charVal);

      if (!theFont->font_FullRead)
        { // <HPP & EL 03-08-94> if memory not loaded fully
          if (isANK (charVal))
            {
              oneCharSize = theFont->font_Hinfo.oneCharSize;
              fseekOffset = theFont->font_HFSeekOffset;
            }
          else
            {
              oneCharSize = theFont->font_Zinfo.oneCharSize;
              fseekOffset = theFont->font_ZFSeekOffset;
            }
          error = ReadFile_E (theFont, (int)oneCharSize, (int32 *)dataPtr,
                              fseekOffset + i * oneCharSize);
          if (error < 0)
            printf ("ReadFile_E dataSize %d, fseekOffset %d\n",
                    (int)oneCharSize, (int)fseekOffset + i * oneCharSize);
          pImageData = dataPtr;
        }
      else
        {
          if (isANK (charVal))
            pImageData = theFont->font_Hinfo.charData
                         + i * theFont->font_Hinfo.oneCharSize;
          else
            pImageData = theFont->font_Zinfo.charData
                         + i * theFont->font_Zinfo.oneCharSize;
        }

      if (charWidth & 0x01)
        { /* 文字幅が奇数の場合 */
          loop = charWidth / 2 + 1;
          for (j = 0; j < endj; j++)
            { /* セルの下端からはみ出す手前のピクセル行まで描画する*/
              if (j < startj)
                { /* セルの上端からはみ出すピクセル行は、描画しない*/
                  if (j & 0x01)
                    pImageData += (loop - 1);
                  else
                    pImageData += loop;
                  continue;
                }
              for (i = 0; i < loop; i++)
                {
                  lineData[i] = *pImageData++;
                }
              if (j & 0x01)
                {
                  lineData[0] &= 0x0F;
                  pImageData += (loop - 1);
                }
              else
                {
                  lineData[i - 1] &= 0xF0;
                  pImageData += (loop - 1);
                }

              endk = loop;
              for (k = startk; k < endk; k++)
                {
                  imageByte = lineData[k];
                  *(pCelData + (k - startk)) |= (imageByte >> (bitStart * 4));
                  *(pCelData + (k - startk) + 1) |= imageByte
                                                    << (8 - (bitStart * 4));
                }
              pCelData += celByteWidth;
            }
        }
      else
        { /* 文字幅が偶数の場合 */
          loop = charWidth / 2;
          for (j = 0; j < endj; j++)
            { /* セルの下端からはみ出す手前のピクセル行まで描画する*/
              if (j < startj)
                { /* セルの上端からはみ出すピクセル行は、描画しない*/
                  pImageData += loop;
                  continue;
                }
              for (i = 0; i < loop; i++)
                {
                  lineData[i] = *pImageData++;
                }

              endk = loop;
              for (k = startk; k < endk; k++)
                {
                  imageByte = lineData[k];
                  *(pCelData + (k - startk)) |= (imageByte >> (bitStart * 4));
                  *(pCelData + (k - startk) + 1) |= imageByte
                                                    << (8 - (bitStart * 4));
                }
              pCelData += celByteWidth;
            }
        }
      return x + charWidth;
    }
  return x;
}

/*------------------------------------------------------------------------
 * KDrawChar1BitCel()
 *	Given a text string, build the Kanji text into a 1-bit cel.
 *----------------------------------------------------------------------*/

int32
KDrawChar1BitCel (KFont3DO *theFont, uint16 charVal, int32 x, int32 y,
                  int32 celByteWidth, CCB *oneBitCel, char *dataPtr)
{
  int32 curX, curY;
  int32 i, j, l, m;
  int32 w;
  int32 xDiff, yDiff;
  int32 startj, endj;
  int32 charWidth, charHeight;
  int32 bitStart;
  char *pCelData, *pImageData, imageRow;
  bool skipChar;
  int32 pixelDepth;
  uint8 lineData[32];
  int32 xbit, ybit, zbit;
  int32 leftBit;
  int32 fseekOffset, oneCharSize;
  int32 error;

  if (theFont == NULL)
    {
      DIAGNOSE (("theFont is NULL.\n"));
      return x;
    }

  if (oneBitCel == NULL)
    {
      DIAGNOSE (("oneBitCel is NULL.\n"));
      return x;
    }

  /* 1Bitセル以外はエラー */
  pixelDepth = oneBitCel->ccb_PRE0 & 0x07;
  if (pixelDepth != PRE0_BPP_1)
    {
      DIAGNOSE (("ERROR:This cel's pixel depth isn't PRE0_BPP_1.\n"));
      return x;
    }

  charWidth = KGetCharWidth (theFont, (uint16)charVal);
  if (charWidth == 0)
    return x;
  charHeight = KGetCharHeight (theFont, (uint16)charVal);
  if (charHeight == 0)
    return x;
  skipChar = false;
  startj = 0;
  endj = charHeight;

  /* フォントの大きさがセルより大きい */
  if ((charWidth > oneBitCel->ccb_Width)
      || (charHeight > oneBitCel->ccb_Height))
    {
      DIAGNOSE (("ERROR:charWidth or charHeight is greater than cel size."));
      return x;
    }

  curX = x;
  curY = y - charHeight;

  xDiff = curX - ConvertF16_32 (oneBitCel->ccb_XPos);
  yDiff = curY - ConvertF16_32 (oneBitCel->ccb_YPos);

  if (yDiff < 0)
    {
      /* 書き始めが上端からはみ出る場合 */
      if (yDiff + charHeight <= 0)
        skipChar = true;
      else
        startj = -yDiff;
      yDiff = 0;
    }
  else if (yDiff + charHeight > oneBitCel->ccb_Height)
    {
      /* 書き終わりが下端からはみ出る場合 */
      if (yDiff >= oneBitCel->ccb_Height)
        skipChar = true;
      else
        endj = oneBitCel->ccb_Height - yDiff;
    }

  if ((xDiff + charWidth < 0) || (xDiff > (oneBitCel->ccb_Width)))
    skipChar = true;
  if (!skipChar)
    {

      /* point to right position in celdata */
      pCelData = (char *)oneBitCel->ccb_SourcePtr + (xDiff / 8)
                 + (celByteWidth * yDiff);

      i = KGetCharIndex (theFont, charVal);

      if (!theFont->font_FullRead)
        { // <HPP & EL 03-08-94> if memory not loaded fully
          if (isANK (charVal))
            {
              oneCharSize = theFont->font_Hinfo.oneCharSize;
              fseekOffset = theFont->font_HFSeekOffset;
            }
          else
            {
              oneCharSize = theFont->font_Zinfo.oneCharSize;
              fseekOffset = theFont->font_ZFSeekOffset;
            }
          error = ReadFile_E (theFont, (int)oneCharSize, (int32 *)dataPtr,
                              fseekOffset + i * oneCharSize);
          if (error < 0)
            printf ("ReadFile_E dataSize %d, fseekOffset %d\n",
                    (int)oneCharSize, (int)fseekOffset + i * oneCharSize);
          pImageData = (uint8 *)dataPtr;
        }
      else
        {
          if (isANK (charVal))
            pImageData = (uint8 *)theFont->font_Hinfo.charData
                         + i * theFont->font_Hinfo.oneCharSize;
          else
            pImageData = (uint8 *)theFont->font_Zinfo.charData
                         + i * theFont->font_Zinfo.oneCharSize;
        }

      xbit = 0; /*8bitのMSBから数えてxbit目を指す*/
      ybit = 8 - xbit;
      zbit = 0;
      leftBit = charWidth;
      l = m = 0;

      if (xDiff < 0)
        { /* Draw partial character */
          bitStart = (-xDiff) & 0x07;
          for (j = 0; j < endj; j++)
            { /* セルの下端からはみ出す手前のピクセル行まで描画する*/
              while (leftBit >= 8)
                {
                  lineData[l] = pImageData[m++] << xbit;
                  lineData[l++] += (pImageData[m] >> ybit);
                  leftBit -= 8;
                }
              if (leftBit > ybit)
                {
                  lineData[l] = pImageData[m++] << xbit;
                  zbit = leftBit - ybit;
                  lineData[l] += ((pImageData[m] >> zbit) << (zbit - ybit));
                  xbit = zbit;
                }
              else if (leftBit > 0)
                {
                  lineData[l] = (pImageData[m] << xbit) >> (8 - leftBit);
                  xbit += leftBit;
                }
              ybit = 8 - xbit;
              if (j < startj)
                { /* セルの上端からはみ出すピクセル行は、描画しない*/
                  l = 0;
                  continue;
                }
              imageRow = lineData[0];
              *pCelData |= imageRow << (8 - bitStart);
              for (i = 1, w = charWidth - 8; w > 0; i++, w -= 8)
                {
                  imageRow = lineData[i];
                  *(pCelData) |= imageRow >> bitStart;
                  *(pCelData + i) |= imageRow << (8 - bitStart);
                }
              pCelData += celByteWidth;
              l = 0;
              leftBit = charWidth;
            }
        }
      else if ((xDiff + charWidth) > (oneBitCel->ccb_Width))
        { /* Draw partial character */
          bitStart = (charWidth + xDiff) - (oneBitCel->ccb_Width);
          for (j = 0; j < endj; j++)
            { /* セルの下端からはみ出す手前のピクセル行まで描画する*/
              while (leftBit >= 8)
                {
                  lineData[l] = pImageData[m++] << xbit;
                  lineData[l++] += (pImageData[m] >> ybit);
                  leftBit -= 8;
                }
              if (leftBit > ybit)
                {
                  lineData[l] = pImageData[m++] << xbit;
                  zbit = leftBit - ybit;
                  lineData[l] += ((pImageData[m] >> zbit) << (zbit - ybit));
                  xbit = zbit;
                }
              else if (leftBit > 0)
                {
                  lineData[l] = (pImageData[m] << xbit) >> (8 - leftBit);
                  xbit += leftBit;
                }
              ybit = 8 - xbit;
              if (j < startj)
                { /* セルの上端からはみ出すピクセル行は、描画しない*/
                  l = 0;
                  continue;
                }
              imageRow = lineData[0];
              *pCelData |= (imageRow >> bitStart);
              for (i = 1, w = charWidth - 8; w > 0; i++, w -= 8)
                {
                  imageRow = lineData[i];
                  *(pCelData) |= imageRow << (8 - bitStart);
                  *(pCelData + i) |= imageRow >> bitStart;
                }
              pCelData += celByteWidth;
              l = 0;
              leftBit = charWidth;
            }
        }
      else
        { /* Draw whole character */
          bitStart = xDiff & 0x07;
          for (j = 0; j < endj; j++)
            { /* セルの下端からはみ出す手前のピクセル行まで描画する*/
              if (leftBit >= 8)
                {
                  while (leftBit >= 8)
                    {
                      if (xbit > 0)
                        {
                          lineData[l] = pImageData[m++] << xbit;
                          leftBit -= ybit;
                          lineData[l++] += pImageData[m] >> ybit;
                          leftBit -= xbit;
                        }
                      else
                        {
                          lineData[l++] = pImageData[m++];
                          leftBit -= 8;
                        }
                    }
                  if (leftBit > 0)
                    {
                      if (leftBit >= ybit)
                        {
                          lineData[l] = pImageData[m++] << xbit;
                          leftBit -= ybit;
                          if (leftBit > 0)
                            lineData[l]
                                += ((pImageData[m] >> (8 - leftBit)) << ybit);
                          xbit = leftBit;
                        }
                      else
                        {
                          zbit = 8 - leftBit;
                          lineData[l] = (pImageData[m] >> zbit) << zbit;
                          xbit = leftBit;
                        }
                    }
                }
              else
                {
                  if (xbit == 0)
                    {
                      lineData[l] = pImageData[m] >> (8 - leftBit);
                      xbit = leftBit;
                    }
                  else if (leftBit > ybit)
                    {
                      lineData[l] = (pImageData[m++] << (leftBit - ybit))
                                    & MASK_TABLE[leftBit - 0];
                      leftBit -= ybit;
                      if (leftBit > 0)
                        lineData[l] += (pImageData[m] >> (8 - leftBit));
                      xbit = leftBit;
                    }
                  else if (leftBit == ybit)
                    {
                      lineData[l] = pImageData[m++] & MASK_TABLE[leftBit - 0];
                      xbit = 0;
                    }
                  else
                    {
                      zbit = 8 - leftBit;
                      lineData[l] = (pImageData[m] >> zbit) << zbit;
                      xbit = leftBit;
                    }
                }

              ybit = 8 - xbit;

              if (j < startj)
                { /* セルの上端からはみ出すピクセル行は、描画しない*/
                  l = 0;
                  for (i = 0; i < l; i++)
                    lineData[i] = 0;
                  continue;
                }

              imageRow = lineData[0];
              *pCelData |= (imageRow >> bitStart);
              *(pCelData + 1) |= imageRow << (8 - bitStart);
              for (i = 1, w = charWidth - 8; w > 0; i++, w -= 8)
                {
                  imageRow = lineData[i];
                  *(pCelData + i) |= (imageRow >> bitStart);
                  *(pCelData + i + 1) |= imageRow << (8 - bitStart);
                }
              pCelData += celByteWidth;
              for (i = 0; i < l; i++)
                lineData[i] = 0;
              l = 0;
              leftBit = charWidth;
            }
        }
      return x + charWidth;
    }
  return x;
}

/*------------------------------------------------------------------------
 * KDrawTextH()
 *	Build the Kanji text image from left to right.
 *----------------------------------------------------------------------*/

void
KDrawTextH (KFont3DO *theFont, KCharRec *theLine, int32 lineChars, int32 x,
            int32 y, int32 charPitch, int32 celByteWidth, CCB *theCCB)
{
  int32 i;
  int32 retVal;
  uint16 c;
  char *dataPtr = NULL;
  int32 dataSize = 0;

  if (!theFont->font_FullRead)
    { // <HPP&EL 03-09-94>
      dataSize = theFont->maxCharSize;
      dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
    }

  for (i = 0; i < lineChars; i++)
    {
      c = (uint16)theLine[i].Value;
      if (theFont->font_Bpp == FOURBITS_PER_PIXEL)
        retVal = KDrawChar4BitCel (theFont, c, x, y, celByteWidth, theCCB,
                                   dataPtr);
      else
        retVal = KDrawChar1BitCel (theFont, c, x, y, celByteWidth, theCCB,
                                   dataPtr);
      if (x != retVal)
        x = (retVal + charPitch);
    }

  if (!theFont->font_FullRead && dataPtr) // <HPP&EL 03-09-94>
    FreeMem (dataPtr, dataSize);
}

/*------------------------------------------------------------------------
 * KDrawTextV()
 *	Build the Kanji text image from top to bottom.
 *----------------------------------------------------------------------*/

void
KDrawTextV (KFont3DO *theFont, KCharRec *theLine, int32 lineChars, int32 x,
            int32 y, int32 charPitch, int32 celByteWidth, CCB *theCCB)
{
  int32 i;
  int32 retVal;
  uint16 c;
  char *dataPtr = NULL;
  int32 dataSize = 0;

  if (!theFont->font_FullRead)
    { // <HPP&EL 03-09-94>
      dataSize = theFont->maxCharSize;
      dataPtr = (char *)AllocMem (dataSize, MEMTYPE_CEL);
    }

  for (i = 0; i < lineChars; i++)
    {
      c = (uint16)theLine[i].Value;
      if (theFont->font_Bpp == FOURBITS_PER_PIXEL)
        retVal = KDrawChar4BitCel (theFont, c, x, y, celByteWidth, theCCB,
                                   dataPtr);
      else
        retVal = KDrawChar1BitCel (theFont, c, x, y, celByteWidth, theCCB,
                                   dataPtr);
      if (x != retVal)
        y += (KGetCharHeight (theFont, c) + (uint16)charPitch);
    }

  if (!theFont->font_FullRead && dataPtr) // <HPP&EL 03-09-94>
    FreeMem (dataPtr, dataSize);
}

/*------------------------------------------------------------------------
 * KDrawTextA()
 *	Align the Kanji text within its box bounds.
 *----------------------------------------------------------------------*/

void
KDrawTextA (KFont3DO *theFont, KTextBox *theTextBox, int32 boxWidth,
            KCharRec *lineStart, int32 lineChars, int32 celByteWidth,
            int32 lineTextWidth, int32 curY, CCB *theCCB)
{
  /*
    For the rest of the text alignments (left, center, and right), we just
    move the pen to the right place and draw the text with DrawText.
    */
  int32 left, right, x;
  int32 charPitch;

  if (theTextBox->otherFlags & O_VERTICAL)
    {
      ;
    }
  else
    {
      left = ConvertF16_32 (theTextBox->wrapRect.rectf16_XLeft);
      right = ConvertF16_32 (theTextBox->wrapRect.rectf16_XRight);
      charPitch = theTextBox->charPitch;

      switch (theTextBox->align)
        {
        case justLeft:
          x = left;
          break;
        case justRight:
          x = right - lineTextWidth;
          break;
        case justCenter:
          x = left + ((boxWidth - lineTextWidth) / 2);
          break;
        case justJustify:
          /* 現在ジャスティファイは実装できていない。*/
          x = left;
          break;
        default:
          x = left;
          break;
        }

      KDrawTextH (theFont, lineStart, lineChars, x, curY, charPitch,
                  celByteWidth, theCCB);
    }
}

/*------------------------------------------------------------------------
 * KWrapAndDraw()
 *	Step thru the text string, and build the Kanji text within
 *	the specified bounds.
 *----------------------------------------------------------------------*/

int32
KWrapAndDraw (KFont3DO *theFont, KTextBox *theTextBox, int32 boxWidth,
              KCharRec *lineStart, KCharRec *textEnd, int32 lineLeft,
              int32 boxRowBytes, int32 top, int32 bottom, CCB *theCCB)
{
  int32 lineCount;  /* Number of lines we've drawn */
  int32 lineChars;  /* Number of bytes in one line */
  int32 lineHeight; /* Calculated line height */
  int32 curY;       /* Current vertical pen location */
  int32 lineTextWidth;

  /* 書き出し位置Y座標を初期化 */
  curY = top;
  lineCount = 0;

  /*
    This is the main wrap-and-draw loop.  I bet you never thought wrapping
    text could be so easy...
    Keep figuring out lines until out of text or run out of vertical space
   */

  if (theTextBox->otherFlags & O_KINSOKU)
    {
      /*禁足処理がONの場合*/
    }
  else
    {
      do
        {
          /* Figure out how many words we can write on this line */
          lineChars = KGetLineChars (theFont, theTextBox, boxWidth, &lineStart,
                                     &lineLeft, &lineHeight, &lineTextWidth);
          curY += lineHeight;
          if (curY > bottom)
            {
              if (lineCount == 0)
                {
                  /* 一行目の書き出し位置がはみ出る場合はエラー*/
                  DIAGNOSE (("The font height size is greater than the "
                             "wrapRect height.\n"));
                  return (ER_KTextBox_BadParameter);
                }
              else
                {
                  return (ER_KTextBox_NoError); /* 二行目の場合はノットエラー*/
                }
            }

          KDrawTextA (theFont, theTextBox, boxWidth, lineStart, lineChars,
                      boxRowBytes, lineTextWidth, curY, theCCB);

          /*
            Now we advance our vertical position down by the height of one
            line, advance lineStart by the number of bytes we just drew,
            calculate a new textLeft, and increment our line count.
            */

          curY += theTextBox->linePitch;
          lineStart += lineChars;
          /* Make sure to skip over the newline characters that were
           * encountered */
          if (lineStart->Value == CR || lineStart->Value == LF)
            {
              lineStart++;
              lineLeft--;
            }
          lineCount++;
        }
      while ((lineStart < textEnd) && (curY <= bottom));
    }

  return (ER_KTextBox_NoError);
}

/*------------------------------------------------------------------------
 * KTextBoxMain()
 *	Create and build a cel from any given text string.
 *----------------------------------------------------------------------*/

int32
KTextBoxMain (KFont3DO *theFont, KTextBox *theTextBox, uint8 *theText,
              int32 textLen, CCB **userCCB)
{
  int32 cel_left, cel_right, cel_top, cel_bottom;
  int32 left, right, top, bottom;
  int32 cel_boxWidth, boxWidth;        /* Width of the wrapBox */
  int32 cel_boxHeight, boxHeight;      /* Height of the wrapBox */
  int32 cel_boxRowBytes = 0;           /* RowBytes of the wrapBox */
  int32 textLeft;                      /* Pointer to remaining bytes of text */
  KCharRec *lineStart, *origLineStart; /* Pointer to beginning of a line */
  KCharRec *textEnd;                   /* Pointer to the end of input text */
  CCB *theCCB = NULL;
  int32 retValue;

  /* 指定された引き数のチェックをする*/
  /* Do some idiot testing */
  if (theFont == NULL)
    {
      DIAGNOSE (("theFont is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (*theText == '\0')
    {
      DIAGNOSE (("theText is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (textLen == 0)
    {
      DIAGNOSE (("textLen is 0.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (theTextBox == NULL)
    {
      DIAGNOSE (("theTextBox is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (userCCB == NULL)
    {
      DIAGNOSE (("userCCB is NULL.\n"));
      return (ER_KTextBox_BadParameter);
    }
  cel_left = ConvertF16_32 (theTextBox->celRect.rectf16_XLeft);
  cel_right = ConvertF16_32 (theTextBox->celRect.rectf16_XRight);
  cel_top = ConvertF16_32 (theTextBox->celRect.rectf16_YTop);
  cel_bottom = ConvertF16_32 (theTextBox->celRect.rectf16_YBottom);

  left = ConvertF16_32 (theTextBox->wrapRect.rectf16_XLeft);
  right = ConvertF16_32 (theTextBox->wrapRect.rectf16_XRight);
  top = ConvertF16_32 (theTextBox->wrapRect.rectf16_YTop);
  bottom = ConvertF16_32 (theTextBox->wrapRect.rectf16_YBottom);

  if ((left < cel_left) || (right > cel_right) || (top < cel_top)
      || (bottom > cel_bottom))
    {
      DIAGNOSE (("wrapRect is too large.\n"));
      return (ER_KTextBox_BadParameter);
    }

  cel_boxWidth = cel_right - cel_left;
  cel_boxHeight = cel_bottom - cel_top;
  if (cel_boxWidth <= 0)
    {
      DIAGNOSE (("celRect is too small.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (cel_boxHeight <= 0)
    {
      DIAGNOSE (("celRect is too small.\n"));
      return (ER_KTextBox_BadParameter);
    }

  boxWidth = right - left;
  boxHeight = bottom - top;
  if (boxWidth <= 0)
    {
      DIAGNOSE (("wrapRect is too small.\n"));
      return (ER_KTextBox_BadParameter);
    }
  if (boxHeight <= 0)
    {
      DIAGNOSE (("wrapRect is too small.\n"));
      return (ER_KTextBox_BadParameter);
    }

  /* 指定された文字数だけテキストデータのフォーマットをコンバートする。*/
  retValue = KConvertText (theText, (KCharRec **)&(lineStart), textLen);
  if (retValue < 0)
    {
      return (retValue);
    }
  else if (retValue < textLen)
    {
      FreeMem (lineStart, retValue * sizeof (KCharRec));
      return (ER_KTextBox_BadCharCode);
    }
  origLineStart = lineStart;
  /* 最後の文字の次のアドレス */
  textEnd = lineStart + textLen;
  /* まだ描画していない残りの文字数 */
  textLeft = textLen;

  if (theFont->font_Bpp == ONEBITS_PER_PIXEL)
    {
      /*	1bitセルのバイト幅を求める	*/
      cel_boxRowBytes = ((((cel_boxWidth + 7) / 8) + 3) / 4) * 4;
      if (cel_boxRowBytes < 8)
        cel_boxRowBytes = 8;
      /*  user requested that we allocate the memory for the CCB */

      if (*userCCB == NULL)
        theCCB = KCreate1BitCodedCel (cel_boxHeight, cel_boxWidth,
                                      cel_boxRowBytes);
      else
        {
          /* user requested that we use the CCB passed into the routine */
          theCCB = *userCCB;

          if (theCCB->ccb_SourcePtr == NULL)
            {
              DIAGNOSE (("theCCB->ccb_SourcePtr is NULL.\n"));
              retValue = ER_KTextBox_BadParameter;
              goto ERROR;
            }
          memset (theCCB->ccb_SourcePtr, 0, cel_boxHeight * cel_boxRowBytes);
        }
      if (theCCB == NULL)
        {
          retValue = ER_KTextBox_CannotMemAlloc;
          goto ERROR;
        }
      theCCB->ccb_XPos = theTextBox->celRect.rectf16_XLeft;
      theCCB->ccb_YPos = theTextBox->celRect.rectf16_YTop;

      retValue = KMakePLUT2 (theTextBox->fgColor, theTextBox->bgColor,
                             theTextBox->opaqueFlag,
                             (uint16 **)&(theCCB->ccb_PLUTPtr));
      if (retValue < 0)
        goto ERROR;
    }
  else if (theFont->font_Bpp == FOURBITS_PER_PIXEL)
    {
      /*	4bitセルのバイト幅を求める	*/
      cel_boxRowBytes = ((((cel_boxWidth + 1) / 2) + 3) / 4) * 4;
      if (cel_boxRowBytes < 8)
        cel_boxRowBytes = 8;
      /* user requested that we allocate the memory for the CCB */
      if (*userCCB == NULL)
        theCCB = KCreate4BitCodedCel (cel_boxHeight, cel_boxWidth,
                                      cel_boxRowBytes);
      else
        {
          /* user requested that we use the CCB passed into the routine */
          theCCB = *userCCB;

          if (theCCB->ccb_SourcePtr == NULL)
            {
              DIAGNOSE (("theCCB->ccb_SourcePtr is NULL.\n"));
              retValue = ER_KTextBox_BadParameter;
              goto ERROR;
            }
          memset (theCCB->ccb_SourcePtr, 0, cel_boxHeight * cel_boxRowBytes);
        }
      if (theCCB == NULL)
        {
          retValue = ER_KTextBox_CannotMemAlloc;
          goto ERROR;
        }

      theCCB->ccb_XPos = theTextBox->celRect.rectf16_XLeft;
      theCCB->ccb_YPos = theTextBox->celRect.rectf16_YTop;

      /* 4bitセルのPLUTを設定する */
      retValue = KMakePLUT16 (theTextBox->fgColor, theTextBox->bgColor,
                              theTextBox->opaqueFlag,
                              (uint16 **)&(theCCB->ccb_PLUTPtr));
      if (retValue < 0)
        goto ERROR;
    }

  retValue = KWrapAndDraw (theFont, theTextBox, boxWidth, lineStart, textEnd,
                           textLeft, cel_boxRowBytes, top, bottom, theCCB);
  if (retValue < 0)
    goto ERROR;

  FreeMem (origLineStart, sizeof (KCharRec) * textLen);
  *userCCB = theCCB;
  return (ER_KTextBox_NoError);
ERROR:
  FreeMem (origLineStart, sizeof (KCharRec) * textLen);
  return (retValue);
}

/*------------------------------------------------------------------------
 * KTextBox_A()
 *	Create and build a cel from any given text string.
 *----------------------------------------------------------------------*/

CCB *
KTextBox_A (KFont3DO *theFont, uint8 *theText, int32 textLen, Rectf16 *wrapBox,
            TextAlign *theAlign, Color fgColor, Color bgColor, CCB *userCCB)
{
  KTextBox theTextBox;

  if (wrapBox == NULL)
    {
      DIAGNOSE (("wrapBox is NULL.\n"));
      return (0);
    }
  if (theAlign == NULL)
    {
      DIAGNOSE (("theAlign is NULL.\n"));
      return (0);
    }

  theTextBox.celRect.rectf16_XLeft = wrapBox->rectf16_XLeft;
  theTextBox.celRect.rectf16_XRight = wrapBox->rectf16_XRight;
  theTextBox.celRect.rectf16_YTop = wrapBox->rectf16_YTop;
  theTextBox.celRect.rectf16_YBottom = wrapBox->rectf16_YBottom;
  theTextBox.wrapRect.rectf16_XLeft = wrapBox->rectf16_XLeft;
  theTextBox.wrapRect.rectf16_XRight = wrapBox->rectf16_XRight;
  theTextBox.wrapRect.rectf16_YTop = wrapBox->rectf16_YTop;
  theTextBox.wrapRect.rectf16_YBottom = wrapBox->rectf16_YBottom;
  theTextBox.align = theAlign->align;
  theTextBox.charPitch = theAlign->charPitch;
  theTextBox.linePitch = theAlign->linePitch;
  theTextBox.fgColor = fgColor;
  theTextBox.bgColor = bgColor;
  theTextBox.opaqueFlag = (fg_Opaque | bg_Opaque);
  theTextBox.otherFlags = 0;
  theTextBox.reserved = 0;

  KTextBoxMain (theFont, &theTextBox, theText, KTextLength2 (theText, textLen),
                &userCCB);

  return userCCB;
}

/*------------------------------------------------------------------------
 * KTextBox_B()
 *	Create and build a cel from any given text string with a
 *	KTextBox structure provided.
 *----------------------------------------------------------------------*/

int32
KTextBox_B (KFont3DO *theFont, KTextBox *theTextBox, uint8 *theText,
            int32 textLen, CCB **userCCB)
{
  return (KTextBoxMain (theFont, theTextBox, theText,
                        KTextLength2 (theText, textLen), userCCB));
}

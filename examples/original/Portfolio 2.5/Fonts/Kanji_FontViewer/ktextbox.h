#ifndef _KTEXTBOX_H
#define _KTEXTBOX_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ktextbox.h,v 1.4 1994/11/21 22:06:49 vertex Exp $
**
******************************************************************************/

#include "ktextbox_error.h"

#define justLeft 0    /* 左揃え */
#define justRight 1   /* 中央揃え */
#define justCenter 2  /* 右揃え */
#define justJustify 3 /* 均等揃え */

#define SJIS_KANJI_HIGH(c)                                                    \
  (((0x81 <= (c)) && ((c) <= 0x9f)) || ((0xe0 <= (c)) && ((c) <= 0xfc)))
#define SJIS_KANJI_LOW(c)                                                     \
  (((0x40 <= (c)) && ((c) <= 0x7e)) || ((0x80 <= (c)) && ((c) <= 0xfc)))
#define SJIS_KANJI_LEVEL2(c) ((0x989f <= (c)) && ((c) <= 0xeafc))
#define NUM_KANJI_LEVEL1 3760

#define fg_Opaque 0x01 /* 文字色に不透明属性を設定 */
#define bg_Opaque 0x02 /* 背景色に不透明属性を設定 */

typedef struct Rectf16
{
  frac16 rectf16_XLeft;
  frac16 rectf16_YTop;
  frac16 rectf16_XRight;
  frac16 rectf16_YBottom;
} Rectf16;

typedef struct
{
  short align; /* 左寄せ、中央、右寄せ*/
  /* Text alignment (left, right, center). */
  int32 charPitch; /* 字間（ドット単位）*/
  /* Inter-character spacing. */
  int32 linePitch; /* 行間（ドット単位）*/
                   /* Spacing bewteen adjactent rows (leading). */
} TextAlign;

typedef struct
{
  Rectf16 celRect;  /* セルの大きさを指定 */
                    /* Cel bounds. */
  Rectf16 wrapRect; /* テキスト描画領域を指定 */
                    /* Bounds to wrap text into. */
  uint16 align;     /* テキストの揃え方の指定 */
                    /* Text alignment (left, right, center, justify). */
  int32 charPitch;  /* 字間を指定 */
                    /* Inter-character spacing. */
  int32 linePitch;  /* 行間を指定 */
                    /* Spacing bewteen adjactent rows (leading). */
  Color fgColor;    /* 文字色を指定 */
                    /* Foreground color (text color). */
  Color bgColor;    /* 背景色を指定 */
                    /* Background color (color beneath text). */
  uint8 opaqueFlag; /* 不透明属性を指定 */
                    /* Is the text background opaque? */
  uint8 otherFlags; /* 予\約 */
                    /* Reserved.  Set to NULL. */
  int32 reserved;   /* 予\約 */
                    /* Reserved.  Set to NULL. */
} KTextBox;

typedef struct
{
  uint16 nChars;      /* 含まれる文字コードの数*/
                      /* Number of characters. */
  uint16 minCode;     /* 最小の文字コード*/
                      /* Minimum character code. */
  uint16 maxCode;     /* 最大の文字コード*/
                      /* Maximum character code. */
  uint8 charHeight;   /* 文字の高さ(pixels) */
                      /* Character height (pixels). */
  uint8 charWidth;    /* 文字の幅(pixels) */
                      /* Character width (pixels). */
  uint32 oneCharSize; /* １文字のピクセルデータのバイト数(pad byteを含む) */
  /* Size of one character bitmap in bytes (long-word aligned). */
  uint16 *codeTbl; /* 文字コードテーブルへのポインタ */
                   /* Optional character code table. */
  uint8 *widthTbl; /* 可変ピッチ時の文字幅テーブルへのポインタ */
                   /* Optional caracter width table. */
  char *charData;  /* ピクセルデータの先頭ポインタ*/
                   /* Character bitmap data. */
} KFontRec;

typedef struct
{
  uint8 font_Gpp;           /* Gray scale bits per pixel*/
  uint8 font_Bpp;           /* Bits per pixel */
  KFontRec font_Hinfo;      /* 半角文字用FontRec構\造体 */
                            /* Hankaku (half-width) character information. */
  KFontRec font_Zinfo;      /* 全角文字用FontRec構\造体 */
                            /* Zenkaku (full-width) character information. */
  Stream *font_fs;          /* Font file stream */
  Boolean font_FullRead;    /* If true, read whole font image into memory. */
  uint32 font_HFSeekOffset; /* File offset to Hankaku bitmap data. */
  uint32 font_ZFSeekOffset; /* File offset to Zenkaku bitmap data. */
  Boolean fsRead; /* If true, use file system calls; otherwise, use low-level
                     IO calls. */
  uint32 maxCharSize;     /* Maximum char size */
  IOInfo romioInfo;       /* Internal use only. */
  Item romIOReqItem;      /* Internal use only. */
  IOReq *romior;          /* Internal use only. */
  uint32 logRomBlockSize; /* Internal use only. */
} KFont3DO;

extern int32 KLoadFont (char *name, KFont3DO *theFont, Boolean fullRead);

extern int32 KFreeFont (KFont3DO *theFont);

extern CCB *KTextBox_A (KFont3DO *theFont, uint8 *theText, int32 textLen,
                        Rectf16 *wrapBox, TextAlign *theAlign, Color fgColor,
                        Color bgColor, CCB *userCCB);

extern int32 KTextBox_B (KFont3DO *theFont, KTextBox *theTextBox,
                         uint8 *theText, int32 textLen, CCB **userCCB);

extern int32 KTextLength (uint8 *theText);

extern int32 KFreeCel (CCB *theCCB);

extern int32 KGetCharWidth (KFont3DO *theFont, uint16 charVal);

extern int32 KGetCharHeight (KFont3DO *theFont, uint16 charVal);

#endif /* _KTEXTBOX_H */

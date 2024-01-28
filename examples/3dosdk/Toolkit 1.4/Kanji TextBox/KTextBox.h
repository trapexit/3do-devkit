#ifndef _KTEXTBOX_H
#define _KTEXTBOX_H
/*
#ifndef _CTYPE_H
#include "ctype.h"
#endif
*/
#ifndef _KTEXTBOXERROR_H
#include "KTextBoxError.h"
#endif

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

typedef struct
{
  short align;     /* 左寄せ、中央、右寄せ*/
  Int32 charPitch; /* 字間（ドット単位）*/
  Int32 linePitch; /* 行間（ドット単位）*/
} TextAlign;

typedef struct
{
  Rectf16 celRect;  /* セルの大きさを指定 */
  Rectf16 wrapRect; /* テキスト描画領域を指定 */
  uint16 align;     /* テキストの揃え方の指定 */
  Int32 charPitch;  /* 字間を指定 */
  Int32 linePitch;  /* 行間を指定 */
  Color fgColor;    /* 文字色を指定 */
  Color bgColor;    /* 背景色を指定 */
  uint8 opaqueFlag; /* 不透明属性を指定 */
  uint8 otherFlags; /* 予\約 */
  Int32 reserved;   /* 予\約 */
} KTextBox;

typedef struct
{
  uint16 nChars;      /* 含まれる文字コードの数*/
  uint16 minCode;     /* 最小の文字コード*/
  uint16 maxCode;     /* 最大の文字コード*/
  uint8 charHeight;   /* 文字の高さ(pixels) */
  uint8 charWidth;    /* 文字の幅(pixels) */
  uint32 oneCharSize; /* １文字のピクセルデータのバイト数(pad byteを含む) */
  uint16 *codeTbl;    /* 文字コードテーブルへのポインタ */
  uint8 *widthTbl;    /* 可変ピッチ時の文字幅テーブルへのポインタ */
  char *charData;     /* ピクセルデータの先頭ポインタ*/
} KFontRec;           /* 6 x 4 = 24 bytes */

typedef struct
{
  uint8 font_Gpp;           /* Gray scale bits per pixel*/
  uint8 font_Bpp;           /* Bits per pixel */
  KFontRec font_Hinfo;      /* 半角文字用FontRec構\造体 */
  KFontRec font_Zinfo;      /* 全角文字用FontRec構\造体 */
  Stream *font_fs;          /* font file stream */
  Boolean font_FullRead;    /* <HPP 03-08-94> if true read whole font image */
  uint32 font_HFSeekOffset; /* <HPP&EL 03-09-94> */
  uint32 font_ZFSeekOffset; /* <HPP&EL 03-09-94>	*/
  Boolean fsRead;
  uint32 maxCharSize; /* maximum char size */
  IOInfo romioInfo;
  Item romIOReqItem;
  IOReq *romior;
  uint32 logRomBlockSize;

} KFont3DO; /* 4 + 24 + 24 = 52 bytes */

/****************************************************************************/
extern Int32 KLoadFont (char *name, KFont3DO *theFont, Boolean fullRead);

extern Int32 KFreeFont (KFont3DO *theFont);

extern CCB *KTextBox_A (KFont3DO *theFont, uint8 *theText, Int32 textLen,
                        Rectf16 *wrapBox, TextAlign *theAlign, Color fgColor,
                        Color bgColor, CCB *userCCB);

extern Int32 KTextBox_B (KFont3DO *theFont, KTextBox *theTextBox,
                         uint8 *theText, Int32 textLen, CCB **userCCB);

extern Int32 KTextLength (uint8 *theText);

extern Int32 KFreeCel (CCB *theCCB);

extern Int32 KGetCharWidth (KFont3DO *theFont, uint16 charVal);

extern Int32 KGetCharHeight (KFont3DO *theFont, uint16 charVal);

#endif /* _KTEXTBOX_H */

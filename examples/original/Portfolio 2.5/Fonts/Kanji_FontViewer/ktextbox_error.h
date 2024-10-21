#ifndef _KTEXTBOXERROR_H
#define _KTEXTBOXERROR_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ktextbox_error.h,v 1.3 1994/11/21 22:06:49 vertex Exp $
**
******************************************************************************/

#ifndef __OPERROR_H
#include "operror.h"
#endif

#define ER_KTextBox_NoError 0         /* 正常終了 */
                                      /* No error encountered. */
#define ER_KTextBox_BadParameter -1   /* パラメータが不正です */
                                      /* Illegal parameter error. */
#define ER_KTextBox_CannotMemAlloc -2 /* メモリが確保できません */
                                      /* Can not allocate memory error. */
#define ER_KTextBox_NotFound -3       /* サーチ対象が見つかりません */
                                      /* Cannot find searched object error. */
#define ER_KTextBox_BadCharCode -4    /* 文字コードが不正です */
                                      /* Illegal character code error. */
#define ER_KTextBox_BadFontFile -5    /* フォントファイルが不正です */
                                      /* Invalid font file error. */
#define ER_KTextBox_CannotOpenDS                                              \
  -6 /* ディスクストリームをオープンできません */
     /* Cannot open disk stream error. */

#endif /* _KTEXTBOXERROR_H */

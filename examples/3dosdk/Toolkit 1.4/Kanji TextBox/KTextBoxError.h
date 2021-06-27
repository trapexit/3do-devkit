#ifndef _KTEXTBOXERROR_H
#define _KTEXTBOXERROR_H

#ifndef _OPERROR_H
#include "operror.h"
#endif

#define ER_KTextBox_NoError 		 0 /* 正常終了 */
#define ER_KTextBox_BadParameter	-1 /* パラメータが不正です */
#define ER_KTextBox_CannotMemAlloc	-2 /* メモリが確保できません */
#define ER_KTextBox_NotFound		-3 /* サーチ対象が見つかりません */
#define ER_KTextBox_BadCharCode		-4 /* 文字コードが不正です */
#define ER_KTextBox_BadFontFile		-5 /* フォントファイルが不正です */
#define ER_KTextBox_CannotOpenDS	-6 /* ディスクストリームをオープンできません */

#endif /* _KTEXTBOXERROR_H */

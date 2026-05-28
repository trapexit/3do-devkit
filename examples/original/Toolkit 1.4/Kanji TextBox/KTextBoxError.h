#ifndef _KTEXTBOXERROR_H
#define _KTEXTBOXERROR_H

#ifndef _OPERROR_H
#include "operror.h"
#endif

#define ER_KTextBox_NoError 0         /* ³iI—¹ */
#define ER_KTextBox_BadParameter -1   /* ƒpƒ‰ƒ[ƒ^‚a•s³‚A‚* */
#define ER_KTextBox_CannotMemAlloc -2 /* ƒƒ‚ƒŠ‚aŠm•Û‚A‚"‚U‚¹‚ñ */
#define ER_KTextBox_NotFound -3       /* ƒT[ƒ`‘ÎÛ‚aŒ(c)‚Â‚(c)‚è‚U‚¹‚ñ */
#define ER_KTextBox_BadCharCode -4    /* •--šƒR[ƒh‚a•s³‚A‚* */
#define ER_KTextBox_BadFontFile -5    /* ƒtƒHƒ“ƒgƒtƒ@ƒCƒ‹‚a•s³‚A‚* */
#define ER_KTextBox_CannotOpenDS                                              \
  -6 /* ƒfƒBƒXƒNƒXƒgƒŠ[ƒ€‚ğƒI[ƒvƒ“‚A‚"‚U‚¹‚ñ */

#endif /* _KTEXTBOXERROR_H */

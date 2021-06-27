#ifndef __JSTRING_H
#define __JSTRING_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jstring.h,v 1.4 1994/11/04 17:53:11 vertex Exp $
**
**  JString folio interface definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __OPERROR_H
#include "operror.h"
#endif


/*****************************************************************************/


/* kernel interface definitions */
#define JSTR_FOLIONAME  "jstring"


/*****************************************************************************/


/* jstring folio errors */
#define MakeJstrErr(svr,class,err) MakeErr(ER_FOLI,ER_JSTR,svr,ER_E_SSTM,class,err)

#define JSTR_ERR_BUFFERTOOSMALL MakeJstrErr(ER_SEVERE,ER_C_NSTND,1)


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenJStringFolio(void);
Err CloseJStringFolio(void);

/* conversion routines */
int32 ConvertShiftJIS2UniCode(const char *string, unichar *result,
                              uint32 resultSize, uint8 filler);
int32 ConvertUniCode2ShiftJIS(const unichar *string, char *result,
                              uint32 resultSize, uint8 filler);
int32 ConvertASCII2ShiftJIS(const char *string, char *result,
                            uint32 resultSize, uint8 filler);
int32 ConvertShiftJIS2ASCII(const char *string, char *result,
                            uint32 resultSize, uint8 filler);
int32 ConvertRomaji2Hiragana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertRomaji2FullKana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertRomaji2HalfKana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHiragana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHiragana2HalfKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHiragana2FullKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertFullKana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertFullKana2HalfKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertFullKana2Hiragana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2FullKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2Hiragana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);


#ifdef __cplusplus
}
#endif


/****************************************************************************/


/* user function offsets */
#define CONVERTSHIFTJIS2UNICODE  -1
#define CONVERTUNICODE2SHIFTJIS  -2
#define CONVERTSHIFTJIS2ASCII    -3
#define CONVERTASCII2SHIFTJIS    -4
#define CONVERTROMAJI2HIRAGANA 	 -5
#define CONVERTROMAJI2FULLKANA 	 -6
#define CONVERTROMAJI2HALFKANA 	 -7
#define CONVERTHIRAGANA2ROMAJI 	 -8
#define CONVERTHIRAGANA2FULLKANA -9
#define CONVERTHIRAGANA2HALFKANA -10
#define CONVERTFULLKANA2HIRAGANA -11
#define CONVERTFULLKANA2HALFKANA -12
#define CONVERTFULLKANA2ROMAJI   -13
#define CONVERTHALFKANA2HIRAGANA -14
#define CONVERTHALFKANA2FULLKANA -15
#define CONVERTHALFKANA2ROMAJI 	 -16


/*****************************************************************************/


#endif /* __JSTRING_H */

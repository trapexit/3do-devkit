#ifndef __INTL_H
#define __INTL_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: intl.h,v 1.21 1994/12/08 17:10:00 vertex Exp $
**
**  International folio interface definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __FOLIO_H
#include "folio.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __OPERROR_H
#include "operror.h"
#endif


/****************************************************************************/


/* kernel interface definitions */
#define INTL_FOLIONAME  "international"


/****************************************************************************/


/* International folio item types */
#define INTL_LOCALE_NODE  1


/**********************************************************************/


/* International folio errors */
#define INTL_ERR_BADTAG           MakeIErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define INTL_ERR_NOMEM            MakeIErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define INTL_ERR_ITEMNOTFOUND     MakeIErr(ER_SEVERE,ER_C_STND,ER_NotFound)
#define INTL_ERR_BADITEM          MakeIErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define INTL_ERR_BADSUBTYPE       MakeIErr(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define INTL_ERR_NOSIGNALS        MakeIErr(ER_SEVERE,ER_C_STND,ER_NoSignals)
#define INTL_ERR_NOTSUPPORTED     MakeIErr(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define INTL_ERR_BADRESULTBUFFER  MakeIErr(ER_SEVERE,ER_C_NSTND,1)
#define INTL_ERR_BUFFERTOOSMALL   MakeIErr(ER_SEVERE,ER_C_NSTND,2)
#define INTL_ERR_BADNUMERICSPEC   MakeIErr(ER_SEVERE,ER_C_NSTND,3)
#define INTL_ERR_FRACTOOLARGE     MakeIErr(ER_SEVERE,ER_C_NSTND,4)
#define INTL_ERR_IMPOSSIBLEDATE   MakeIErr(ER_SEVERE,ER_C_NSTND,5)
#define INTL_ERR_BADGREGORIANDATE MakeIErr(ER_SEVERE,ER_C_NSTND,6)
#define INTL_ERR_BADDATESPEC      MakeIErr(ER_SEVERE,ER_C_NSTND,7)
#define INTL_ERR_CANTFINDCOUNTRY  MakeIErr(ER_SEVERE,ER_C_NSTND,8)
#define INTL_ERR_BADCHARACTERSET  MakeIErr(ER_SEVERE,ER_C_NSTND,9)
#define INTL_ERR_BADSOURCEBUFFER  MakeIErr(ER_SEVERE,ER_C_NSTND,10)
#define INTL_ERR_BADFLAGS         MakeIErr(ER_SEVERE,ER_C_NSTND,11)


/*****************************************************************************/


#define MAKE_LANG(a,b) ((a << 24) | (b << 16))

/* These definitions come from the standard "ISO-639, Codes for the
 * representation of names of languages". Not all codes are defined herein,
 * only the most widely used ones. However, all of the ISO codes are
 * valid values for the loc_Language field of the Locale structure,
 * even though only a subset of the codes are given names in this include
 * file
 */
typedef enum LanguageCodes
{
    INTL_LANG_ARABIC       = MAKE_LANG('a','r'),
    INTL_LANG_ARMENIAN     = MAKE_LANG('h','y'),
    INTL_LANG_BULGARIAN    = MAKE_LANG('b','g'),
    INTL_LANG_CHINESE      = MAKE_LANG('z','h'),
    INTL_LANG_CZECH        = MAKE_LANG('c','s'),
    INTL_LANG_DANISH       = MAKE_LANG('d','a'),
    INTL_LANG_DUTCH        = MAKE_LANG('n','l'),
    INTL_LANG_ENGLISH      = MAKE_LANG('e','n'),
    INTL_LANG_FINNISH      = MAKE_LANG('f','i'),
    INTL_LANG_FRENCH       = MAKE_LANG('f','r'),
    INTL_LANG_GERMAN       = MAKE_LANG('d','e'),
    INTL_LANG_GREEK        = MAKE_LANG('e','l'),
    INTL_LANG_HEBREW       = MAKE_LANG('i','w'),
    INTL_LANG_HUNGARIAN    = MAKE_LANG('h','u'),
    INTL_LANG_ICELANDIC    = MAKE_LANG('i','s'),
    INTL_LANG_INDONESIAN   = MAKE_LANG('i','n'),
    INTL_LANG_IRISH        = MAKE_LANG('g','a'),
    INTL_LANG_ITALIAN      = MAKE_LANG('i','t'),
    INTL_LANG_JAPANESE     = MAKE_LANG('j','a'),
    INTL_LANG_KOREAN       = MAKE_LANG('k','o'),
    INTL_LANG_LATIN        = MAKE_LANG('l','a'),
    INTL_LANG_MONGOLIAN    = MAKE_LANG('m','n'),
    INTL_LANG_NORWEGIAN    = MAKE_LANG('n','o'),
    INTL_LANG_PERSIAN      = MAKE_LANG('f','a'),
    INTL_LANG_POLISH       = MAKE_LANG('p','l'),
    INTL_LANG_PORTUGUESE   = MAKE_LANG('p','t'),
    INTL_LANG_ROMANIAN     = MAKE_LANG('r','o'),
    INTL_LANG_RUSSIAN      = MAKE_LANG('r','u'),
    INTL_LANG_SCOTS_GAELIC = MAKE_LANG('g','d'),
    INTL_LANG_SINGHALESE   = MAKE_LANG('s','i'),
    INTL_LANG_SPANISH      = MAKE_LANG('e','s'),
    INTL_LANG_SWEDISH      = MAKE_LANG('s','v'),
    INTL_LANG_TURKISH      = MAKE_LANG('t','r'),
    INTL_LANG_UKRAINIAN    = MAKE_LANG('u','k'),
    INTL_LANG_VIETNAMESE   = MAKE_LANG('v','i'),
    INTL_LANG_WELSH        = MAKE_LANG('c','y')
} LanguageCodes;


/*****************************************************************************/


/* language dialects describing regional variations of languages */
typedef uint32 DialectCodes;

/* defines the end of an array of dialects */
#define INTL_DIALECT_ARRAY_END 0

/* dialects for INTL_LANG_ENGLISH */
#define INTL_ED_AMERICAN    1
#define INTL_ED_AUSTRALIAN  2
#define INTL_ED_BRITISH     3
#define INTL_ED_IRISH       4
#define INTL_ED_SCOTTISH    5

/* dialects for INTL_LANG_FRENCH */
#define INTL_FD_CANADIAN 1
#define INTL_FD_FRENCH   2
#define INTL_FD_SWISS    3


/*****************************************************************************/


/* These definitions come from the standard "ISO-3166, Codes for the
 * representation of names of countries". Not all codes are defined herein,
 * only the most widely used ones. However, all of the ISO codes are
 * valid values for the loc_Country field of the Locale structure,
 * even though only a subset of the codes are given names in this include
 * file
 */
typedef enum CountryCodes
{
    INTL_CNTRY_ARGENTINA            = 32,
    INTL_CNTRY_AUSTRALIA            = 36,
    INTL_CNTRY_AUSTRIA              = 40,
    INTL_CNTRY_BAHAMAS              = 44,
    INTL_CNTRY_BARBADOS             = 52,
    INTL_CNTRY_BELGIUM              = 56,
    INTL_CNTRY_BRAZIL               = 76,
    INTL_CNTRY_BULGARIA             = 100,
    INTL_CNTRY_CANADA               = 124,
    INTL_CNTRY_CHILE                = 152,
    INTL_CNTRY_CHINA                = 156,
    INTL_CNTRY_COLOMBIA             = 170,
    INTL_CNTRY_CUBA                 = 192,
    INTL_CNTRY_CZECH_REPUBLIC       = 203,
    INTL_CNTRY_DENMARK              = 208,
    INTL_CNTRY_EGYPT                = 818,
    INTL_CNTRY_EL_SALVADOR          = 222,
    INTL_CNTRY_FINLAND              = 246,
    INTL_CNTRY_FRANCE               = 250,
    INTL_CNTRY_FRANCE_METROPOLITAIN = 248,
    INTL_CNTRY_GERMANY              = 276,
    INTL_CNTRY_GREECE               = 300,
    INTL_CNTRY_GREENLAND            = 304,
    INTL_CNTRY_GUATEMALA            = 320,
    INTL_CNTRY_HAITI                = 332,
    INTL_CNTRY_HONDURAS             = 340,
    INTL_CNTRY_HONG_KONG            = 344,
    INTL_CNTRY_HUNGARY              = 348,
    INTL_CNTRY_ICELAND              = 352,
    INTL_CNTRY_INDIA                = 356,
    INTL_CNTRY_INDONESIA            = 360,
    INTL_CNTRY_IRAN                 = 364,
    INTL_CNTRY_IRAQ                 = 368,
    INTL_CNTRY_IRELAND              = 372,
    INTL_CNTRY_ISRAEL               = 376,
    INTL_CNTRY_ITALY                = 380,
    INTL_CNTRY_JAMAICA              = 388,
    INTL_CNTRY_JAPAN                = 392,
    INTL_CNTRY_KOREA_NORTH          = 408,
    INTL_CNTRY_KOREA_SOUTH          = 410,
    INTL_CNTRY_KUWAIT               = 414,
    INTL_CNTRY_LEBANON              = 422,
    INTL_CNTRY_LUXEMBOURG           = 442,
    INTL_CNTRY_MADAGASCAR           = 450,
    INTL_CNTRY_MEXICO               = 484,
    INTL_CNTRY_MONACO               = 492,
    INTL_CNTRY_MONGOLIA             = 496,
    INTL_CNTRY_NETHERLANDS          = 528,
    INTL_CNTRY_NEW_ZEALAND          = 554,
    INTL_CNTRY_NICARAGUA            = 558,
    INTL_CNTRY_NORWAY               = 578,
    INTL_CNTRY_PAKISTAN             = 586,
    INTL_CNTRY_PANAMA               = 591,
    INTL_CNTRY_PERU                 = 604,
    INTL_CNTRY_PHILIPPINES          = 608,
    INTL_CNTRY_POLAND               = 616,
    INTL_CNTRY_PORTUGAL             = 620,
    INTL_CNTRY_PUERTO_RICO          = 630,
    INTL_CNTRY_ROMANIA              = 642,
    INTL_CNTRY_RUSSIAN_FEDERATION   = 643,
    INTL_CNTRY_SAUDI_ARABIA         = 682,
    INTL_CNTRY_SENEGAL              = 686,
    INTL_CNTRY_SINGAPORE            = 702,
    INTL_CNTRY_SOUTH_AFRICA         = 710,
    INTL_CNTRY_SPAIN                = 724,
    INTL_CNTRY_SWEDEN               = 752,
    INTL_CNTRY_SWITZERLAND          = 756,
    INTL_CNTRY_TAIWAN               = 158,
    INTL_CNTRY_TURKEY               = 792,
    INTL_CNTRY_UKRAINE              = 804,
    INTL_CNTRY_UNITED_KINGDOM       = 826,
    INTL_CNTRY_UNITED_STATES        = 840,
    INTL_CNTRY_VATICAN_CITY         = 336,
    INTL_CNTRY_VENEZUELA            = 862,
    INTL_CNTRY_VIET_NAM             = 704,
    INTL_CNTRY_YUGOSLAVIA           = 891
} CountryCodes;


/*****************************************************************************/


/* types of measuring systems */
typedef enum MeasuringSystems
{
    INTL_MS_AMERICAN,       /* American system             */
    INTL_MS_IMPERIAL,       /* Imperial system             */
    INTL_MS_METRIC          /* International Metric system */
} MeasuringSystems;


/*****************************************************************************/


/* types of calendar representations */
typedef enum CalendarTypes
{
    INTL_CT_ARABIC_CIVIL,
    INTL_CT_GREGORIAN_MONDAY,     /* Gregorian, Monday is the first day */
    INTL_CT_GREGORIAN_SUNDAY,     /* Gregorian, Sunday is the first day */
    INTL_CT_JEWISH,
    INTL_CT_PERSIAN
} CalendarTypes;


/*****************************************************************************/


/* types of driving */
typedef enum DrivingTypes
{
    INTL_DT_LEFTHAND,
    INTL_DT_RIGHTHAND
} DrivingTypes;


/*****************************************************************************/


/* character attribute flags, for use with intlGetCharAttrs() */
#define INTL_ATTRF_UPPERCASE     (1 << 0)
#define INTL_ATTRF_LOWERCASE     (1 << 1)
#define INTL_ATTRF_PUNCTUATION   (1 << 2)
#define INTL_ATTRF_DECIMAL_DIGIT (1 << 3)
#define INTL_ATTRF_NUMBERS       (1 << 4)
#define INTL_ATTRF_NONSPACING    (1 << 5)
#define INTL_ATTRF_SPACE         (1 << 6)
#define	INTL_ATTRF_HALF_WIDTH    (1 << 7)
#define	INTL_ATTRF_FULL_WIDTH    (1 << 8)
#define INTL_ATTRF_KANA          (1 << 9)
#define INTL_ATTRF_HIRAGANA      (1 << 10)
#define INTL_ATTRF_KANJI         (1 << 11)


/*****************************************************************************/


/* character conversion flags, for use with intlConvertString() */
#define INTL_CONVF_UPPERCASE         (1 << 0)
#define INTL_CONVF_LOWERCASE         (1 << 1)
#define INTL_CONVF_STRIPDIACRITICALS (1 << 2)
#define INTL_CONVF_HALF_WIDTH        (1 << 3)
#define INTL_CONVF_FULL_WIDTH        (1 << 4)


/*****************************************************************************/


/* For use with intlTransliterateString() */
typedef enum CharacterSets
{
    INTL_CS_ASCII,      /* standard 8-bit ASCII    */
    INTL_CS_UNICODE,    /* standard 16-bit UniCode */
    INTL_CS_SHIFT_JIS,  /* standard 8/16-bit SJIS  */
    INTL_CS_ROMAJI,     /* standard 8-bit ASCII    */
    INTL_CS_HALF_KANA,  /* standard 8-bit SJIS     */
    INTL_CS_FULL_KANA,  /* standard 16-bit SJIS    */
    INTL_CS_HIRAGANA    /* standard 16-bit SJIS    */
} CharacterSets;


/*****************************************************************************/


/* For use with intlFormatDate() */
typedef struct GregorianDate
{
    uint32 gd_Year;         /* 1..0xfffff                          */
    uint32 gd_Month;        /* 1..12                               */
    uint8  gd_Day;          /* 1..28/29/30/31   (depends on month) */
    uint8  gd_Hour;         /* 0..23                               */
    uint8  gd_Minute;       /* 0..59                               */
    uint8  gd_Second;       /* 0..59                               */
} GregorianDate;


/*****************************************************************************/


/* A grouping descriptor defines where group separators should be
 * inserted within a formatted number. For example, if bit 0 is ON,
 * it means a separator sequence should be inserted after digit 0 of
 * the formatted number.
 */
typedef uint32 GroupingDesc;


/*****************************************************************************/


/* This structure is used as a parameter to the intlFormatNumber() function.
 * It describes how to format numeric values. Three seperate instances
 * of this structure exist within the Locale structure, describing the
 * number formatting, currency formatting, and small currency formatting.
 * You can use these three NumericSpec structures in order to get fully
 * localized numeric conversions, or you can provide a custom structure
 * for special needs.
 */
typedef struct NumericSpec
{
    /* how to generate a positive number */
    GroupingDesc      ns_PosGroups;              /* grouping description */
    unichar          *ns_PosGroupSep;            /* separates the groups */
    unichar          *ns_PosRadix;               /* decimal mark         */
    GroupingDesc      ns_PosFractionalGroups;    /* grouping description */
    unichar          *ns_PosFractionalGroupSep;  /* separates the groups */
    unichar          *ns_PosFormat;              /* for post-processing  */
    uint32            ns_PosMinFractionalDigits; /* min # of frac digits */
    uint32            ns_PosMaxFractionalDigits; /* max # of frac digits */

    /* how to generate a negative number */
    GroupingDesc      ns_NegGroups;              /* grouping description */
    unichar          *ns_NegGroupSep;            /* separates the groups */
    unichar          *ns_NegRadix;               /* decimal mark         */
    GroupingDesc      ns_NegFractionalGroups;    /* grouping description */
    unichar          *ns_NegFractionalGroupSep;  /* separates the groups */
    unichar          *ns_NegFormat;              /* for post-processing  */
    uint32            ns_NegMinFractionalDigits; /* min # of frac digits */
    uint32            ns_NegMaxFractionalDigits; /* max # of frac digits */

    /* when the number is zero, this string is used 'as-is' */
    unichar          *ns_Zero;

    /* additional formatting options */
    uint32            ns_Flags;                  /* for now, always 0    */
} NumericSpec;


/*****************************************************************************/


/* This is a format string similar in concept to a printf()-style format
 * string. It uses different formatting commands which are
 * especially tailored to output date information.
 */
typedef unichar *DateSpec;


/*****************************************************************************/


/* defines a set of localized attributes */
typedef struct Locale
{
    ItemNode          loc;                   /* system linkage */

    /* prefered language to use */
    LanguageCodes     loc_Language;

    /* An array of dialects for the current language, listed in order
     * of preference, and terminated with INTL_DIALECTS_ARRAY_END
     */
    DialectCodes     *loc_Dialects;

    /* ISO-3166 numeric-3 code for the user's country */
    CountryCodes      loc_Country;

    /* general description of the user's environment */
    int32             loc_GMTOffset;         /* minutes from GMT            */
    MeasuringSystems  loc_MeasuringSystem;   /* measuring system to use     */
    CalendarTypes     loc_CalendarType;      /* calendar type to use        */
    DrivingTypes      loc_DrivingType;       /* side of the street          */

    /* number formatting */
    NumericSpec       loc_Numbers;
    NumericSpec       loc_Currency;
    NumericSpec       loc_SmallCurrency;

    /* date formatting */
    DateSpec          loc_Date;
    DateSpec          loc_ShortDate;
    DateSpec          loc_Time;
    DateSpec          loc_ShortTime;
} Locale;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/***** International folio functions and macros *****/

/* folio management */
Err intlOpenFolio(void);
Err intlCloseFolio(void);

/* core Locale functions */
#define intlOpenLocale(tags)      FindAndOpenItem(MKNODEID(NST_INTL,INTL_LOCALE_NODE),tags)
#define intlCloseLocale(locItem)  CloseItem(locItem)
#define intlLookupLocale(locItem) ((Locale *)CheckItem(locItem,(uint8)NST_INTL,INTL_LOCALE_NODE))

/* date formatting */
int32 intlFormatDate(Item locItem, DateSpec spec,
                     const GregorianDate *date,
                     unichar *result, uint32 resultSize);

/* number and currency formatting */
int32 intlFormatNumber(Item locItem, const NumericSpec *spec,
                       uint32 whole, uint32 frac, bool negative, bool doFrac,
                       unichar *result, uint32 resultSize);

/* localized string handling routines */
int32 intlCompareStrings(Item locItem, const unichar *string1, const unichar *string2);
int32 intlConvertString(Item locItem, const unichar *string, unichar *result, uint32 resultSize, uint32 flags);
int32 intlTransliterateString(const void *string, CharacterSets stringSet, void *result, CharacterSets resultSet, uint32 resultSize, uint8 unknownFiller);
uint32 intlGetCharAttrs(Item locItem, unichar character);


#ifdef __cplusplus
}
#endif


/****************************************************************************/


/* user function offsets */
#define INTLFORMATNUMBER        -1
#define INTLFORMATDATE          -2
#define INTLTRANSLITERATESTRING -3
#define INTLCOMPARESTRINGS      -4
#define INTLCONVERTSTRING       -5
#define INTLGETCHARATTRS        -6


/*****************************************************************************/


#endif /* __INTL_H */

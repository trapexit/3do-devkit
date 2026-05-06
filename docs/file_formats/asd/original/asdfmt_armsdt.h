/*
  Title:        Acorn Object Format
  Status:       C Library Extension
  Copyright:    (C) 1991, 1994, Advanced RISC Machines Limited. All rights reserved.

  RCS $Revision: 1.18 $
  Checkin $Date: 1997/03/20 17:01:52 $
  Revising $Author: hmeeking $
*/

/*
 * This header defines the essential structure of ASD debugging tables.
 */

#ifndef __asd_h
#define __asd_h

#include "host.h"

#define ASD_FORMAT_VERSION 3
/* decaof is already 4, stayed for armsd for the moment */

/* Most table items start with a word containing a type code and the length
   in bytes of the item (most items end as defined here in an array of size 1,
   which in actual tables is made as large as needed).
 */

typedef unsigned32 CodeAndLen;

#define asd_code_(x) ((x) & 0xffff)
#define asd_len_(x) ((x) >> 16)

#define asd_codeword(c, l) ((c) | ((l) << 16))

/* the code field of a CodeAndLen may have the following values */

#define ITEMSECTION     1
#define ITEMPROC        2
#define ITEMENDPROC     3
#define ITEMVAR         4
#define ITEMTYPE        5
#define ITEMSTRUCT      6
#define ITEMARRAY       7
#define ITEMSUBR        8
#define ITEMSET         9
#define ITEMFILEINFO    10
#define ITEMENUMC       11
#define ITEMENUMD       12
#define ITEMFUNCTION    13
#define ITEMSCOPEBEGIN  14
#define ITEMSCOPEEND    15
#define ITEMBITFIELD    16
#define ITEMDEFINE      17
#define ITEMUNDEF       18
#define ITEMCLASS       19
#define ITEMUNION       20
#define ITEMFPMAPFRAG   32

/* Type descriptions are contained in items ... */

typedef int32 asd_Type;  /* with the structure ... */

#define TYPE_PTRCOUNT(t) ((t) & 0xff)
#define TYPE_TYPECODE(t) ((t) >> 8)

#define TYPE_TYPEWORD(t, p) (((t) << 8) | (p))

/* where TYPE_TYPECODE(type) is
     negative, and its negated value is the offset in the debug data of an
               item defining a type
   or positive, and its value is one of those listed below
 */

#define TYPEVOID         0

#define TYPESBYTE       10
#define TYPESHALF       11
#define TYPESWORD       12
#define TYPESDWORD      13

#define TYPEUBYTE       20
#define TYPEUHALF       21
#define TYPEUWORD       22
#define TYPEUDWORD      23

#define TYPEFLOAT       30
#define TYPEDOUBLE      31
#define TYPELDOUBLE     32

#define TYPECOMPLEX     40
#define TYPEDCOMPLEX    41

#define TYPESTRING      50   /* a primitive (char *) type (needed for Number) */

#define TYPEFUNCTION    100

#define type_integral(t) ((unsigned32)(t) < 30)
#define type_signed(t)   ((unsigned32)(t) < 20)
#define type_fpoint(t)   ((t) >= 30 && (t) < 40)
#define type_complex(t)  ((t) >= 40 && (t) < 50)

typedef unsigned32 asd_Address;
typedef unsigned32 asd_FileOffset;

/* sourcepos items contain line number and character position fields */

#define FILE_LINE(f)  ((f) & 0x3fffffL)
#define FILE_CHPOS(f) (((f) >> 22) & 0x3ff)

#define FILE_POSWORD(f, ch) ((f) | ((ch) << 22))

typedef struct ItemFileInfo ItemFileInfo;
typedef struct ItemFileEntry ItemFileEntry;
typedef struct ItemEndProc ItemEndProc;

/* Values for the lang field of an ItemSection */

#define LANG_NONE       0
#define LANG_C          1
#define LANG_PASCAL     2
#define LANG_FORTRAN    3
#define LANG_ASM        4

/* Bits set in the flags field of an ItemSection */

#define FLAG_LINES(f) ((f) & 1)
#define FLAG_VARS(f)  ((f) & 2)
#define FLAG_FPMAP(f) ((f) & 4)

typedef union asd_NameP
{   char name[1];
} asd_Name;

typedef struct ItemSection
{   CodeAndLen    id;
    unsigned char lang;
    unsigned char flags;
    unsigned char unused;
    unsigned char asdversion;
    asd_Address   codestart;
    asd_Address   datastart;
    int32         codesize;
    int32         datasize;
    union {
      asd_FileOffset i;
                } fileinfo;
    unsigned32    debugsize;
    union {
      char        name[1]; /* high level section */
      unsigned32  nsyms;   /* low level section: nsyms ItemSymbols follow */
                } n;
} ItemSection;

/* the  sym  field of an ItemSymbol consists of
 *    24 (ls) bits of string table index
 *     8 (ms) bits of flags.
 */

#define STR_INDEX(s)  ((s) & 0xffffffL)

#define ASD_LOCSYM      0
#define ASD_GLOBSYM     0x01000000L
#define ASD_SYMGLOBAL(s) ((s) & 0x1000000L)

#define ASD_ABSSYM      0
#define ASD_TEXTSYM     0x02000000L
#define ASD_DATASYM     0x04000000L
#define ASD_ZINITSYM    0x06000000L
#define ASD_SYMTYPE(s)  ((s) & 0x6000000L)

#define ASD_32BITSYM    0
#define ASD_16BITSYM    0x10000000L
#define ASD_SYM16(s)    ((s) & 0x10000000L)


typedef struct ItemSymbol
{   unsigned32  sym;
    unsigned32  value;
} ItemSymbol;

typedef union
{   asd_FileOffset i;
} asd_FileEntryP;

typedef struct ItemProc
{   CodeAndLen  id;
    asd_Type    type;   /* of the return value */
    unsigned32  args;
    unsigned32  sourcepos;
    asd_Address startaddr;
    asd_Address entry;
    union {
      asd_FileOffset i;
              } endproc;
    asd_FileEntryP fileentry;
    asd_Name       n;
} ItemProc;

struct ItemEndProc
{   CodeAndLen  id;
    unsigned32  sourcepos;
    asd_Address endpoint;
    asd_FileEntryP fileentry;
    unsigned32  nreturns;
    asd_Address retaddrs[1];
};

/* Values for ItemVar storageclass field */
typedef enum StgClass {
    C_NONE,
    C_EXTERN,
    C_STATIC,
    C_AUTO,
    C_REG,
    C_VAR,
    C_FARG,
    C_FCARG,
    C_LOCAL,
    C_FILTERED,
    C_GLOBALREG
} StgClass;

typedef struct ItemVar
{   CodeAndLen  id;
    asd_Type    type;
    unsigned32  sourcepos;
    unsigned32  storageclass; /* StgClass really */
    union {
      int32 offset;
      asd_Address address;
              } location;
    asd_Name    n;
} ItemVar;

typedef struct ItemType
{   CodeAndLen  id;
    asd_Type    type;
    asd_Name    n;
} ItemType;

typedef struct StructField
{   unsigned32  offset;
    asd_Type    type;
    asd_Name    n;
} StructField;

typedef struct SUC
{   CodeAndLen  id;
    unsigned32  fields;
    unsigned32  size;
    StructField fieldtable[1];
} SUC;

typedef SUC ItemStruct;
typedef SUC ItemClass;
typedef SUC ItemUnion;

/* Meaning of the flags field in an ItemArray */

#define ARRAY_UNDEF_LBOUND(f) ((f) & 1)
#define ARRAY_CONST_LBOUND(f) ((f) & 2)
#define ARRAY_UNDEF_UBOUND(f) ((f) & 4)
#define ARRAY_CONST_UBOUND(f) ((f) & 8)
#define ARRAY_VAR_LBOUND(f) ((f) & 16)
#define ARRAY_VAR_UBOUND(f) ((f) & 32)

typedef union asd_ArrayBound
{   int32       i;   /* (undef), const */
    int32       o;   /* no flag bit set: bound is a variable on the stack
                      * with this fp offset */
    asd_FileOffset v;/* var: file form */
} asd_ArrayBound;

typedef struct ItemArray
{   CodeAndLen  id;
    unsigned32  size;
    unsigned32  flags;
    asd_Type    basetype;
    asd_ArrayBound lowerbound,
                   upperbound;
} ItemArray;

/* The sizeandtype field of an ItemSubRange */

#define SUBRANGE_SIZE(s) ((s) & 0xffff)
#define SUBRANGE_TYPECODE(s) ((s) >> 16)

typedef struct ItemSubrange
{   CodeAndLen  id;
    int32       sizeandtype;
    int32       lb, hb;
} ItemSubrange;

typedef struct ItemSet
{   CodeAndLen  id;
    unsigned32  size;
} ItemSet;

typedef struct ItemEnumC
{   CodeAndLen  id;
    asd_Type    type;   /* of the container */
    unsigned32  count;
    int32       base;
    union {
      char nametable[1];
              } nt;
} ItemEnumC;

typedef struct asd_EnumMember
{   int32       val;
    asd_Name    n;
} asd_EnumMember;

typedef struct ItemEnumD
{   CodeAndLen  id;
    asd_Type    type;   /* of the container */
    unsigned32  count;
    asd_EnumMember nametable[1];
} ItemEnumD;

typedef struct asd_Arg
{   asd_Type    type;
    asd_Name    n;
} asd_Arg;

typedef struct ItemFunction
{   CodeAndLen  id;
    asd_Type    type;   /* of the returned value */
    unsigned32  argcount;
    asd_Arg     args[1];
} ItemFunction;

typedef struct ItemBitfield
{   CodeAndLen  id;
    asd_Type    type;   /* of the extracted value */
    asd_Type    container;
    unsigned char size, offset,
                  pad1, pad2;
} ItemBitfield;

typedef struct ItemScope /* begin or end */
{   CodeAndLen  id;
    asd_Address codeaddress;
} ItemScope;

typedef struct ItemDefine
{   CodeAndLen  id;
    asd_FileEntryP fileentry;
    unsigned32  sourcepos;
    union {
      asd_FileOffset i;
              } body;
    int32       argcount;
    union {
      asd_FileOffset i;
              } argtable;
    asd_Name    n;
} ItemDefine;

typedef struct ItemUndef
{   CodeAndLen  id;
    asd_FileEntryP fileentry;
    unsigned32  sourcepos;
    asd_Name    n;
} ItemUndef;

typedef struct Fragment
{   unsigned32  size;
    unsigned32  firstline, lastline;
    asd_Address codestart;
    unsigned32  codesize;
    unsigned char lineinfo[1];
} Fragment;

#define Asd_LineInfo_Short_MaxLine 63

/* short form lineinfo items have code and file position increments < 256.
 * If the current file position is (line, col), a file position increment
 * fpinc describes a new file position
 *   (line+fpinc, 1) if inc <= Asd_LineInfo_Short_MaxLine
 *   (line, col+fpinc-Asd_LineInfo_Short_MaxLine-1) otherwise
 *
 * fpinc = 0, codeinc = 0 indicates that the item is in long format 1:
 * new file position is (line+lineinc, 1)
 *
 * fpinc = Asd_LineInfo_Short_MaxLine+1, codeinc = 0 indicates that the
 * item is in long format 2. new file position is
 *   (line+lineinc, newcol)
 */

typedef struct
{   unsigned char codeinc;
    unsigned char lineinc;
} asd_LineInfo_Short;

typedef struct
{   unsigned short marker;
    unsigned short lineinc; /* (in target byte order) */
    unsigned short codeinc; /* (in target byte order) */
} asd_LineInfo_Long_1;

typedef struct
{   unsigned short marker;
    unsigned short lineinc; /* (in target byte order) */
    unsigned short codeinc; /* (in target byte order) */
    unsigned short newcol;  /* (in target byte order) */
} asd_LineInfo_Long_2;

struct ItemFileEntry
{   unsigned32  len;
    unsigned32  date;
    char        filename[1];
};

struct ItemFileInfo
{   CodeAndLen  id;
    ItemFileEntry entries[1];
};

typedef struct ItemFPMapFragment {
  unsigned32  marker;
  int32 bytes;
  asd_Address codestart;
  asd_Address saveaddr;
  unsigned32  codesize;
  int32 initoffset;
  char b[1];
} ItemFPMapFragment;

typedef struct {
  unsigned char codeinc;
  signed char offsetinc;
} asd_FPInfo_Short;

typedef struct {
  char marker[2];
  unsigned short codeinc;
  signed short offsetinc;
} asd_FPInfo_Long;

typedef union {
    unsigned char b;
    CodeAndLen c;
    ItemSection sect;
    ItemProc p;
    ItemEndProc e;
    ItemVar v;
    ItemArray a;
    ItemType t;
    ItemStruct s;
    ItemSubrange sr;
    ItemSet set;
    ItemFileInfo f;
    ItemFileEntry fe;
    ItemEnumC ec;
    ItemEnumD ed;
    ItemFunction fn;
    ItemScope sc;
    ItemBitfield bf;
    ItemDefine def;
    ItemUndef undef;
    ItemFPMapFragment fp;
} Item;

#endif

/*
  Title:     ARM Object Format
  Status:    C Library Extension
  Copyright: (C) 1991-93, Advanced RISC Machines Limited. All rights reserved.
*/

/*
 * RCS $Revision: 1.10.12.1 $
 * Checkin $Date: 1998/02/23 11:50:45 $
 * Revising $Author: lsmith $
 */

/*
 * This header defines the essential structure of ARM Object Format (AOF).
 */

#ifndef __aof_h
#define __aof_h

#include "host.h"

#define  OBJ_HEAD      "OBJ_HEAD"
#define  OBJ_AREA      "OBJ_AREA"
#define  OBJ_IDFN      "OBJ_IDFN"
#define  OBJ_SYMT      "OBJ_SYMT"
#define  OBJ_STRT      "OBJ_STRT"

#define  SHL_LIBY      "SHL_LIBY"  /* Shared Library binary chunk.        */

#define  AOF_RELOC     0xc5e2d080  /* Magic number for reloactable AOF.   */

#define  AOF_VERSION   311

#define  AOF_ALMASK    0xffL       /* ALignment in ls byte of attributes. */

#define  AOF_ABSAT     0x000100L   /* area has an absolute base address.  */
#define  AOF_CODEAT    0x000200L   /* area is code; otherwise, it's data. */
#define  AOF_COMDEFAT  0x000400L   /* area defines a common block...      */
#define  AOF_COMREFAT  0x000800L   /* ...or refers to a common block...   */
#define  AOF_0INITAT   0x001000L   /* area is 0-initialised.              */
#define  AOF_RONLYAT   0x002000L   /* area is read-only.                  */
#define  AOF_PICAT     0x004000L   /* position-independent code.          */
#define  AOF_DEBUGAT   0x008000L   /* area contains debugging data.       */

/* APCS-3/ARM6 attributes                                                 */
#define  AOF_CODEXATs  0x7f0000     /* the next 4 are code attributes...  */
#define  AOF_32bitAT   0x010000     /* doesn't preserve PSR over fn calls */
#define  AOF_REENTAT   0x020000     /* re-entrant, sb-relative addressing */
#define  AOF_FP3AT     0x040000     /* code uses R-3 of the FP instr set. */
#define  AOF_NOSWSTKCK 0x080000     /* code uses R-3 of the FP instr set. */
#define  AOF_THUMB     0x100000     /* Thumb code (all relocs are Thumb)  */
#define  AOF_HALFWORD  0x200000     /* code uses ARM halfword insts.      */
#define  AOF_INTERWORK 0x400000     /* suitable for ARM/Thumb interworking*/
/* and the next are data area attributes...                               */
#define  AOF_BASEDAT   0x100000     /* area has an associated base reg... */
#define  AOF_SHLDATA   0x200000     /* area is shared lib stub data...    */
#define  AOF_BASEMASK  0xf000000    /* ...in this field of the flags...   */
#define  AOF_BASESHIFT 24           /* ...i.e. shifted by 20 bits.        */

typedef struct {
   int32 area_name;         /* idx in the string table of the area's name */
   unsigned32 area_attributes;    /* alignment and attribute flags.       */
   int32 area_size;         /* Byte size of area - a multiple of 4.       */
   int32 area_nrelocs;      /* no of relocation directives in this area.  */
   int32 area_base;         /* base address of an absolute area.          */
} aof_area;

typedef struct {
   int32 aof_type;          /* the object module type - usually AOF_RELOC */
   int32 aof_vsn;           /* usually AOF_VERSION                        */
   int32 aof_nareas;        /* the number of areas in this AOF file       */
   int32 aof_nsyms;         /* the number of symbols in this AOF file     */
   int32 aof_entryarea;     /* the index of the area containing the entry */
   int32 aof_entryoffset;   /* point, and its offset in the area, or 0, 0 */
   aof_area aof_areas[1];
} aof_header;

#define  REL_TYPE2     0x80000000
#define  REL_B         0x10000000
#define  REL_A         0x08000000
#define  REL_R         0x04000000
#define  REL_BYTE      0x00000000
#define  REL_SHORT     0x01000000
#define  REL_LONG      0x02000000
#define  REL_INSTR     0x03000000

typedef struct {
     int32 rel_offset;     /* offset in area of field to be relocated   */
unsigned32 rel_flags;      /* really split up as follows...             */
} aof_reloc;

/* Field    Type-1 Type-2  -- least- to most-significant bit order...
   rel_sid   :16    :24    -- index of symbol in SYMT w.r.t. which relocating
   rel_FT    :2     :2     -- field type: 00=>byte,01=>short,10=>long,
                                          11=>instruction sub-field
   rel_R     :1     :1     -- relocation type: pc relative if 1.
   rel_A     :1     :1     -- if R==0, relocate rel (A==0 ? area : sym)
   rel_B     --     :1     -- 1 => relocate base-reg-relative.
   rel_ins   --     :2     -- if FT = 11 then max no of instructions in the
                              instruction sequence; ins = 0 => max = 4.
   rel_type2 :1     :1     -- the discriminant: 1 for type 2
*/

#define REL_PCREL          1
#define rel_type2(flags)   (int)((flags) >> 31)

#define rel1_sid(flags)    (int)((flags) & 0xffff) 
#define rel1_FT(flags)     (int)(((flags) >> 16) & 3L)
#define rel1_R(flags)      (int)(((flags) >> 18) & 1L)
#define rel1_A(flags)      (int)(((flags) >> 19) & 1L)

#define rel2_sid(flags)    (int)((flags) & 0xffffff) 
#define rel2_FT(flags)     (int)(((flags) >> 24) & 3L)
#define rel2_R(flags)      (int)(((flags) >> 26) & 1L)
#define rel2_A(flags)      (int)(((flags) >> 27) & 1L)
#define rel2_B(flags)      (int)(((flags) >> 28) & 1L)
#define rel2_ins(flags)    (int)(((flags) >> 29) & 3L)
/* The nonesense flags value 0xf0000000 means 'no relocation' */
/* (No-relocations are used to note inter-AREA referneces).   */
#define rel2_noreloc(flags) ((flags)>>24 == 0xf0)

#define  SYM_REFDEFMASK    0x03L            /* mask for 1st 2-bit field */
#define  SYM_DEFAT         0x01L
#define  SYM_LOCALDEFAT    0x01L
#define  SYM_REFAT         0x02L
#define  SYM_GLOBALDEFAT   0x03L

#define  SYM_ABSAT         0x04L
#define  SYM_NOCASEAT      0x08L
#define  SYM_WEAKAT        0x10L
#define  SYM_STRONGAT      0x20L
#define  SYM_COMMONAT      0x40L

/* Validity conditions: SYM_CODEAT may be set only if SYM_DEFAT is set. */
/* SYM_USESSBAT and SYM_LEAFAT shall only be set if SYM_CODEAT is set.  */
/* Prior to AOF-3, none of these bits is set.                           */

#define  SYM_DATAAT      0x0100L         /* symbol names a datum        */
#define  SYM_FPREGAT     0x0200L         /* named/referenced fn passes  */
                                         /* FP args in FP registers.    */
#define  SYM_USESSBAT    0x0400L         /* defined fn 'uses' sb        */
#define  SYM_LEAFAT      0x0800L         /* defined fn is a leaf fn     */

#define  SYM_THUMB       0x1000L         /* Symbol defines a 'Thumb' entry point */

#define  SYM_LINKATTR    0x8000L         /* for use by link: well clear */
                                         /* of 'real' symbol attributes */

typedef struct {
    int32 sym_name;        /* offset in string table of symbol's name   */
    unsigned32 sym_AT;     /* symbol's attributes.                      */
    int32 sym_value;       /* the symbol's 'value'.                     */
    int32 sym_areaname;    /* index in STRT of name of area containing  */
                           /* the definition of this symbol, else 0.    */
} aof_symbol;

#endif

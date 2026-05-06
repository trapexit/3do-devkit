/*
  Title:     Acorn Image Format
  Status:    C Library Extension
  Copyright: (C) 1991 Advanced RISC Machines Limited. All rights reserved.
  $Revision: 1.6 $  27-Apr-91
*/


#ifndef __aif_h
#define __aif_h

#include "host.h"

#define AIF_NOOP      0xe1a00000             /* MOV r0, r0 */
#define AIF_BLAL      0xeb000000
#define OS_EXIT       0xef000011             /* SWI OS_Exit */
#define OS_GETENV     0xef000010             /* SWI OS_GetEnv */
#define AIF_IMAGEBASE 0x00008000             /* Default load address = 32K */
#define AIF_BLZINIT   0xeb00000C
#define DEBUG_TASK    0xef041d41             /* RISC OS SWI DDE_Debug */
#define AIF_DBG_SRC   2
#define AIF_DBG_LL    1

#define AIF_DATABASAT 0x100L                 /* if has a separate data base */

typedef struct aif_hdr {
    unsigned32 compress_br;
    unsigned32 reloc_br;
    unsigned32 zinit_br;
    unsigned32 entry_br;
    unsigned32 exit_swi;
    unsigned32 rosize, rwsize, dbgsize, zinitsize;
    unsigned32 dbgtype;
    unsigned32 imagebase;
    unsigned32 workspace;
    unsigned32 address_mode;       /* and flags */
    unsigned32 data_base;
    unsigned32 fragment_offset;
    unsigned32 spare2;
    unsigned32 debug_swi;
    unsigned32 zinitcode[15];
} aif_hdr;

typedef struct {
    unsigned32 next_fragment_offset;
    unsigned32 load_address;
    unsigned32 size;
    char name[32];
} fragment_header;

#endif

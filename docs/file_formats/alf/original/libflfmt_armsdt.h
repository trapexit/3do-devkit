/*
  Title:     Library File Format
  Status:    C Library Extension
  Copyright: (C) 1991, Advanced RISC Machines Ltd., Cambridge, England.
  $Revision: 1.3 $  LDS 08-Mar-91
*/

#ifndef libflfmt_h
#define libflfmt_h

/*
 * A library file is a chunk file containing (at least) the following chunks:-
 * (see chunkflfmt.h for a description of the format of chunk files)
 *
 * LIB_DIRY - a directory of library members
 * LIB_TIME - a time stamp on the whole library
 * LIB_VRSN - a version number for the library
 * LIB_DATA - one or more chunks with this name are the library members.
 *
 * A directory chunk is a sequence of entry records in the format
 * described below (lib_direntry).
 */

#include "host.h"

#define  LIB_DIRY    "LIB_DIRY"
#define  LIB_TIME    "LIB_TIME"
#define  LIB_VRSN    "LIB_VRSN"
#define  LIB_DATA    "LIB_DATA"
#define  LIB_ENTRY   "LIB_DATA"
#define  OFL_SYMT    "OFL_SYMT"
#define  OFL_TIME    "OFL_TIME"

typedef struct {
    int32 lib_chunkidx;         /* index of chunk in the library or 0 */
    int32 lib_entrylength;      /* length of data area following      */
    int32 lib_datalength;       /* length used in the following area  */
    char  lib_entry[1];         /* Actually lib_entrylength...        */
} lib_direntry;

typedef struct {
    unsigned32 msw, lsw;
} lib_timestamp;

/*
 * The lib_entry part of the above records consists of a text name, padded
 * to a 4-byte boundary; followed by an 8-byte binary time-stamp in abstract
 * filestamp format (see filestamp.h); followed by implementation-specific
 * (library-specific) data.
 */

#endif

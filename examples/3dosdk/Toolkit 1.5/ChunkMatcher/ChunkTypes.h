/*******************************************************************************************
 *	File:			ChunkTypes.h
 *
 *	Contains:
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __CHUNKTYPES__
#define __CHUNKTYPES__

/*************/
/*  INLINES  */
/*************/

// Handy for making four byte TYPE idents

#define CHAR4LITERAL(a, b, c, d)                                              \
  ((unsigned long)((unsigned long)a << 24) | ((unsigned long)b << 16)         \
   | ((unsigned long)c << 8) | d)

/***************/
/*  CONSTANTS  */
/***************/

// Macintosh creator and type for Form-3DO cel chunk file

#define CEL_FILE_CREATOR CHAR4LITERAL ('3', 'D', 'O', 'e')
#define CEL_FILE_TYPE CHAR4LITERAL ('3', 'D', 'O', 'c')

// Types of chunks in a 3DO chunk-form file

#define CHUNK_PDAT CHAR4LITERAL ('P', 'D', 'A', 'T') // pixel data
#define CHUNK_CCB CHAR4LITERAL ('C', 'C', 'B', ' ')  // cel control block
#define CHUNK_PLUT CHAR4LITERAL ('P', 'L', 'U', 'T') // pixel data
#define CHUNK_ANIM CHAR4LITERAL ('A', 'N', 'I', 'M') // animation header
#define CHUNK_ANY CHAR4LITERAL ('*', '*', '*', '*')  // matches any chunk

// Types of chunks in a stream chunk file

#define CHUNK_FILL CHAR4LITERAL ('F', 'I', 'L', 'L')    // stream filler chunk
#define CHUNK_CONTROL CHAR4LITERAL ('C', 'T', 'R', 'L') // stream control chunk
#define CONTROL_GOTO CHAR4LITERAL ('G', 'O', 'T', 'O')  // goto chunk
#define CONTROL_SYNC CHAR4LITERAL ('S', 'Y', 'N', 'C')  // sync chunk
#define CONTROL_ALARM CHAR4LITERAL ('A', 'L', 'R', 'M') // alarm chunk
#define CONTROL_PAUSE CHAR4LITERAL ('P', 'A', 'U', 'S') // pause chunk
#define CONTROL_STOP CHAR4LITERAL ('S', 'T', 'O', 'P')  // stop chunk

// Streamed animations

#define CHUNK_SANM CHAR4LITERAL ('S', 'A', 'N', 'M') // any SANM chunk
#define SANM_HEADER                                                           \
  CHAR4LITERAL ('A', 'H', 'D', 'R') // SANM header chunk sub-type
#define SANM_CCB CHAR4LITERAL ('A', 'C', 'C', 'B') // SANM CCB chunk sub-type
#define SANM_FRAME                                                            \
  CHAR4LITERAL ('F', 'R', 'M', 'E') // SANM frame chunk sub-type
#define SANM_POS                                                              \
  CHAR4LITERAL ('A', 'P', 'O', 'S') // SANM POSition chunk sub-type
#define SANM_MAP                                                              \
  CHAR4LITERAL ('A', 'M', 'A', 'P') // SANM MAPcel chunk sub-type

// Streamed Anti-Aliased Animations

#define CHUNK_SAAA CHAR4LITERAL ('S', 'A', 'A', 'A') // any SAAA chunk
#define SAAA_CCB CHAR4LITERAL ('A', 'A', 'C', 'C')   // SAAA CCB chunk
#define SAAA_PLUT CHAR4LITERAL ('A', 'P', 'L', 'T')  // SAAA PLUT chunk

// Streamed audio samples

#define CHUNK_SNDS                                                            \
  CHAR4LITERAL ('S', 'N', 'D', 'S') // any streamed audio chunk
#define SNDS_HEADER CHAR4LITERAL ('S', 'H', 'D', 'R') // audio header chunk
#define SNDS_SAMPLE                                                           \
  CHAR4LITERAL ('S', 'S', 'M', 'P') // audio sample data chunk

// Streamed Cinepak

#define CHUNK_FILM                                                            \
  CHAR4LITERAL ('F', 'I', 'L', 'M') // any streamed cinepak chunk
#define FILM_HEADER CHAR4LITERAL ('F', 'H', 'D', 'R') // cinepak header chunk
#define FILM_FRAME CHAR4LITERAL ('F', 'R', 'M', 'E')  // cinepak frame data

// Streamed Join Elements

#define CHUNK_JOIN CHAR4LITERAL ('J', 'O', 'I', 'N') // any streamed join chunk
#define JOIN_HEADER CHAR4LITERAL ('J', 'H', 'D', 'R') // join header chunk
#define JOIN_DATA CHAR4LITERAL ('J', 'D', 'A', 'T')   // join data chunk

// Streamed cels

#define CHUNK_SCEL CHAR4LITERAL ('S', 'C', 'E', 'L')   // any SCEL chunk
#define SCEL_HEADER CHAR4LITERAL ('H', 'E', 'A', 'D')  // SCEL header chunk
#define SCEL_CELLIST CHAR4LITERAL ('C', 'L', 'S', 'T') // SCEL cel-list chunk

// Types of cel lists in a SCel chunk file or stream

#define CELLIST_NORMAL CHAR4LITERAL ('C', 'L', 'S', 'T') // regular cel list
#define CELLIST_DOUBLEVISION                                                  \
  CHAR4LITERAL ('D', 'B', 'L', 'V') // Double-Vision cel list

#endif

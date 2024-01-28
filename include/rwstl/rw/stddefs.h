#ifndef __RWSTDDEFS_H__
#define __RWSTDDEFS_H__

/***************************************************************************
 *
 * stddefs.h - Common definitions
 *
 * $Id: stddefs.h@#/main/sl1main/sl121main/spm_bldmdl/0  03/07/97 14:05:05  smithey (SL121RA_UNIX)
 *
 ***************************************************************************
 *
 * (c) Copyright 1994-1997 Rogue Wave Software, Inc.
 * ALL RIGHTS RESERVED
 *
 * The software and information contained herein are proprietary to, and
 * comprise valuable trade secrets of, Rogue Wave Software, Inc., which
 * intends to preserve as trade secrets such software and information.
 * This software is furnished pursuant to a written license agreement and
 * may be used, copied, transmitted, and stored only in accordance with
 * the terms of such license and with the inclusion of the above copyright
 * notice.  This software and information or any other copies thereof may
 * not be provided or otherwise made available to any other person.
 *
 * Notwithstanding any other lease or license that may pertain to, or
 * accompany the delivery of, this computer software and information, the
 * rights of the Government regarding its use, reproduction and disclosure
 * are as set forth in Section 52.227-19 of the FARS Computer
 * Software-Restricted Rights clause.
 * 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause at DFARS 252.227-7013.
 * Contractor/Manufacturer is Rogue Wave Software, Inc.,
 * P.O. Box 2328, Corvallis, Oregon 97339.
 *
 * This computer software and information is distributed with "restricted
 * rights."  Use, duplication or disclosure is subject to restrictions as
 * set forth in NASA FAR SUP 18-52.227-79 (April 1985) "Commercial
 * Computer Software-Restricted Rights (April 1985)."  If the Clause at
 * 18-52.227-74 "Rights in Data General" is specified in the contract,
 * then the "Alternate III" clause applies.
 *
 **************************************************************************/

# include "stdcomp.h"	/* Set compiler-specific flags */

STARTWRAP
#ifndef RWSTD_NO_NEW_HEADER
#include <cstddef>		/* Looking for size_t */
#else
#include <stddef.h>		/* Looking for size_t */
#endif
ENDWRAP

/*
 * In rare instances, the following few lines may have to be reworked
 * to deal with naming conflicts.
 */

#ifndef RWSTD_NO_RWBOOLEAN

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

#ifdef RWSTD_NO_BOOL 
typedef int RWBoolean;
#else 
typedef bool RWBoolean;
#endif

#endif  /* RWSTD_NO_RWBOOLEAN */

#define rwstdnil	0
#define	RWSTDNIL	-1L

#ifndef RWSTD_NO_NEW_HEADER
#ifndef RWSTD_NO_NAMESPACE
using std::size_t;
#endif
#endif
  
/*
 *     W I N D O W S - S P E C I F I C   C O D E
 *
 * Enable or disable, as necessary, for Microsoft Windows
 */
#if defined(__WIN32__) || defined (__WIN16__) 
#  include "rw/stdwind.h"
#else
   /* Disable Windows hacks if we are not compiling for Windows: */
#  define RWSTDExport
#  define RWSTDExportTemplate
#  define RWSTDExportFunc(ReturnType) ReturnType
#  define RWSTDHuge
#  define rwexport
#endif

#ifndef _MSC_VER
#define RWSTDGExport RWSTDExport
#else
#define RWSTDGExport
#endif


/*************************************************************************
**************************************************************************
**									**
**		From here on, it's pretty much boilerplate		**
**		and rarely requires any tuning.				**
**									**
**************************************************************************
**************************************************************************/

/*
 *     D E B U G G I N G
 *
 * Use -DRWSTDDEBUG to compile a version of the libraries to debug
 * the user's code.  This will perform pre- and post-condition checks
 * upon entering routines, but will be larger and run more slowly.
 *
 * Use -DRWMEMCK to add memory checking software.
 *
 * VERY IMPORTANT!  *All* code must be compiled with the same flag.
 */

#if defined(RDEBUG) && !defined(RWSTDDEBUG)
#  define RWSTDDEBUG 1
#endif

#if defined(RWSTDDEBUG)
#  ifndef RWSTDBOUNDS_CHECK
#    define RWSTDBOUNDS_CHECK 1	/* Turn on bounds checking when debugging. */
#  endif
STARTWRAP
#ifndef RWSTD_NO_NEW_HEADER
#  include <cassert>
#else
#  include <assert.h>
#endif
ENDWRAP
#  define RWSTDPRECONDITION(a)	assert( (a) != 0 ) /* Check pre- and post-conditions */
#  define RWSTDPOSTCONDITION(a)	assert( (a) != 0 )
#ifdef RWSTD_NO_NESTED_QUOTES
#  define RWSTDPRECONDITION2(a,b)	assert( (a) != 0 )
#  define RWSTDPOSTCONDITION2(a,b)	assert( (a) != 0 )
#else
#  define RWSTDPRECONDITION2(a,b)	assert((b, (a) !=0))
#  define RWSTDPOSTCONDITION2(a,b)	assert((b, (a) !=0))
#endif
#  define RWSTDASSERT(a)		assert( (a) != 0 )
#else
#  define RWSTDPRECONDITION(a)
#  define RWSTDPOSTCONDITION(a)
#  define RWSTDPRECONDITION2(a,b)
#  define RWSTDPOSTCONDITION2(a,b)
#  define RWSTDASSERT(a)
#endif

/* Check for USL hook for multi-thread mode: */
#if defined(_REENTRANT) && !defined(RWSTD_MULTI_THREAD)
# define RWSTD_MULTI_THREAD 1
#endif



/*
 * The following allows getting the declaration for RTL classes
 * right without having to include the appropriate header file
 * (in particular, istream & ostream).
 */
#ifdef __TURBOC__
#  include <_defs.h>	/* Looking for _CLASSTYPE */
#  define _RWSTDCLASSTYPE _CLASSTYPE
#else
#  define _RWSTDCLASSTYPE
#endif

/* No RCS for MS-DOS (it has enough memory problems already!): */
#ifdef __MSDOS__
#define RCSID(a)
#define RW_RCSID(a)
#else
#define RCSID(a) static const char rcsid[] = a
#define RWSTD_RCSID(a) static const char rcsid[] = a
#endif

/* Enable near/far pointers if we are using segmented architecture: */
#if (defined(__MSDOS__) || defined(I8086)) && !defined(__HIGHC__)
#  define RWSTD_SEGMENTED_ARCHITECTURE
#  define rwnear near
#  define rwfar  far
#  define rwhuge huge
#else
#  define rwnear
#  define rwfar
#  define rwhuge
#endif

#ifdef __cplusplus


const size_t RWSTD_NPOS            = ~(size_t)0;


#ifdef RW_STD_IOSTREAM
#include <iosfwd>
#else
class _RWSTDCLASSTYPE istream;
class _RWSTDCLASSTYPE ostream;
class _RWSTDCLASSTYPE ios;
#endif

#ifdef RW_MULTI_THREAD
class RWSTDMutex;
#endif

#ifdef RWSTD_NO_FRIEND_INLINE_DECL
#  define RWSTD_INLINE_FRIEND friend
#else
#  define RWSTD_INLINE_FRIEND inline friend
#endif


#endif // if C++

#endif // __RWSTDDEFS_H__

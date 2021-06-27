/***************************************************************************
 *
 * rw/math.h - Standard Library vs math.h exception conflict hack.
 *
 * $Id: math.h@#/main/sl1main/sl121main/0  02/21/97 12:05:46  smithey (SL121RA_UNIX)
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
 ***************************************************************************
 *
 *  Many compilers provide a structure named "exception" in math.h, used to
 *  report errors from a floating point unit.  Some Microsoft compilers also
 *  #define complex as a synonym for a structure named _complex.  To prevent
 *  collisions with the Standard Library exception and complex classes, this
 *  header causes the old exception class to be renamed "math_exception" and
 *  removes the Microsoft complex symbol.
 *
 *  If you want to use math.h in conjunction with the Rogue Wave Standard
 *  library, always #include <rw/math.h> rather than <math.h>!
 *
 *  As an alternate to this solution to the exception problem you 
 *  can define RWSTD_RENAME_EXCEPTION.  This will rename the standard
 *  library exception calls to std_exception.  
 *
 **************************************************************************/

#ifndef __STD_RW_MATH_H
#define __STD_RW_MATH_H

#ifndef __STDRWCOMPILER_H__
#include <stdcomp.h>
#endif

#ifndef RWSTD_RENAME_EXCEPTION
//
// Avoid a conflicting exception structure
//
#if !defined (_MSC_VER)
#define exception math_exception
#endif
#endif /* RWSTD_RENAME_EXCEPTION */

#ifndef RWSTD_NO_NEW_HEADER
#include <cmath>
#else
#include <math.h>
#endif

#ifndef RWSTD_RENAME_EXCEPTION
//
// Undefine exception macro now
//
#ifdef exception
#undef exception
#endif
#endif /* RWSTD_RENAME_EXCEPTION */

//
// MSVC provides it's own complex macro too
//
#ifdef _MSC_VER
#ifdef complex
#undef complex
#endif
#endif

#endif /* __STD_RW_MATH_H */


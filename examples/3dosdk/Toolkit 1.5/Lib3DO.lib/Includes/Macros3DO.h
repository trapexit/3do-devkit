/*****************************************************************************
 *	File:		Macros3DO.h
 *
 *	Contains:	Common utility macros.
 *
 *	Written by: Ian Lepore
 *
 *	Copyright:	(c) 1993 by The 3DO Company. All rights reserved.
 *
 *	Change History:
 *
 *	02/26/94  Ian	New today.
 * 
 *	Implementation notes:
 *	
 *	AddToPtr() adds a byte offset to any type of pointer.  It casts its args
 *	such that any datatype will work for either arg.
 *
 *	ArrayElements() evaluates to the number of elements the array was defined
 *	with.  It works only for array types.
 ****************************************************************************/

#pragma include_only_once

#ifndef MACROS3DO_H
#define MACROS3DO_H

#ifndef AddToPtr
  #define AddToPtr(ptr, val) ((void*)((((char *)(ptr)) + (long)(val))))
#endif

#ifndef ArrayElements
  #define ArrayElements(a)	(sizeof(a) / sizeof((a)[0]))
#endif

#endif // MACROS3DO_H

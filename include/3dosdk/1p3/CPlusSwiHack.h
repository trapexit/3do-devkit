/*
	File:		CPlusSwiHack.h

	Contains:	Include file which fixes the swi call link problem.
				Must be included in all .cp files after all other
				header includes. 
				during the StreamEdit step of the armCPlus script:
				The CPlusSwiHack typedef is replaced with a #include "swi.h"
				The other typedefs are replaced by #defines of
				the symbols used in swi.h to conditionally enable the declaration
				of the corresponding swi calls.
				

	Written by:	Charlie Eckhaus (per Anup Murarka's suggestion)

	Copyright:	© 1994 by The 3DO Company Inc., all rights reserved.

	Change History (most recent first):

				 2/9/94		CDE		New today.

	To Do:
*/

#ifdef __audio_h
typedef long audio_swiHack ;
#endif

#ifdef __DEBUG_H
typedef long DEBUG_swiHack ;
#endif

#ifdef __H_FILEFUNCTIONS
typedef long FILEFUNCTIONS_swiHack ;
#endif

#ifdef __TYPES_H
typedef long TYPES_swiHack ;
#endif

#ifdef __FOLIO_H
typedef long FOLIO_swiHack ;
#endif

#ifdef __HARDWARE_H
typedef long HARDWARE_swiHack ;
#endif

#ifdef __IO_H
typedef long IO_swiHack ;
#endif

#ifdef __ITEM_H
typedef long ITEM_swiHack ;
#endif

#ifdef __MEM_H
typedef long MEM_swiHack ;
#endif

#ifdef __MSGPORT_H
typedef long MSGPORT_swiHack ;
#endif

#ifdef __operamath_h
typedef long operamath_swiHack ;
#endif

#ifdef __SEMAPHORE_H
typedef long SEMAPHORE_swiHack ;
#endif

#ifdef __TASK_H
typedef long TASK_swiHack ;
#endif

typedef long CPlus_swiHack ;

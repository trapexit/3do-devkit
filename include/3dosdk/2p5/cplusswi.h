#ifdef __cplusplus
#ifndef __CPLUSSWI_H
#define __CPLUSSWI_H


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: cplusswi.h,v 1.2 1994/12/08 23:02:31 vertex Exp $
**
**  This include file is meant for C++ use only. It must be included by
**  every .cp file, following every other include files.
**
******************************************************************************/


#ifdef __AUDIO_H
typedef long __REDFN_AUDIO;
#endif

#ifdef __DEBUG_H
typedef long __REDFN_DEBUG;
#endif

#ifdef __H_FILEFUNCTIONS
typedef long __REDFN_FILEFUNCTIONS;
#endif

#ifdef __GRAPHICS_H
typedef long __REDFN_GRAPHICS;
#endif

#ifdef __HARDWARE_H
typedef long __REDFN_HARDWARE;
#endif

#ifdef __IO_H
typedef long __REDFN_IO;
#endif

#ifdef __ITEM_H
typedef long __REDFN_ITEM;
#endif

#ifdef __MEM_H
typedef long __REDFN_MEM;
#endif

#ifdef __MSGPORT_H
typedef long __REDFN_MSGPORT;
#endif

#ifdef __OPERAMATH_H
typedef long __REDFN_OPERAMATH;
#endif

#ifdef __SEMAPHORE_H
typedef long __REDFN_SEMAPHORE;
#endif

#ifdef __TASK_H
typedef long __REDFN_TASK;
#endif

#ifdef __TIME_H
typedef long __REDFN_TIME;
#endif

#ifdef __TYPES_H
typedef long __REDFN_TYPES;
#endif

typedef long __REDFN_CPLUS;


/*****************************************************************************/


#endif /* __CPLUSSWI_H */
#endif /* __cplusplus */

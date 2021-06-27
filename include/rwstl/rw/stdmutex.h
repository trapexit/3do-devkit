#ifndef __RWSTD_MUTEX_H__
#define __RWSTD_MUTEX_H__

/*
 * Declarations for class RWSTDMutex and RWSTDGuard.
 *
 * $Id: stdmutex.h@#/main/sl1main/sl121main/spm_bldmdl/1  03/11/97 18:42:15  smithey (SL121RA_UNIX)
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
 * This class is a portable implementation of a simple mutex lock
 * to be used for synchronizing multiple threads within a single process.
 * It is not suitable for use among threads of different processes.
 * This code was taken from the tools mutex.h.
 * 
 ***************************************************************************/

#include <stdcomp.h>

#ifndef __RWSTDDEFS_H__
#include "rw/stddefs.h"
#endif

#ifdef RWSTD_MULTI_THREAD /* This class only relevant in MT situation */

#if defined(sun)          /* assuming Solaris 2.1 or greater */
#include <synch.h>
typedef mutex_t RWSTDMutexType;
#elif defined(RWSTD_POSIX_THREADS)
#include <pthread.h>
typedef pthread_mutex_t RWSTDMutexType;
#define RWSTD_NEEDS_SEM_INIT
#elif defined(__WIN32__)
#include <windows.h>
typedef CRITICAL_SECTION RWSTDMutexType;
#define RWSTD_NEEDS_SEM_INIT
#elif defined(__OS2__)
#include <exception>
#define INCL_DOSSEMAPHORES
#define RWSTD_NEEDS_SEM_INIT
#include <os2.h>
typedef HMTX RWSTDMutexType;
extern const char* __rw_mutex_exception;

class RWSTDExport thread_error : public RWSTD_exception
{
  public:
    thread_error () RWSTD_THROW_SPEC_NULL : RWSTD_exception( )
    { ; }
    virtual const char * what () const  RWSTD_THROW_SPEC_NULL
    {
        return __rw_mutex_exception;
    }
};                            

#else
#error Class RWSTDMutex is not yet supported in this environment
#endif

class RWSTDMutex
{
  private:

    RWSTDMutexType mutex;
#if defined(RWSTD_NEEDS_SEM_INIT)
    int initFlag;
#endif

    void init ();
    //
    // Disallow copying and assignment.
    //
    RWSTDMutex (const RWSTDMutex&);
    RWSTDMutex& operator= (const RWSTDMutex&);

public:

  enum StaticCtor { staticCtor };

  RWSTDMutex ();             // Construct the mutex.
  RWSTDMutex (StaticCtor);   // Some statics need special handling.
  ~RWSTDMutex ();            // Destroy the mutex.

  void acquire ();           // Acquire the mutex.
  void release ();           // Release the mutex.
};

class RWSTDGuard
{
  private:

    RWSTDMutex& rwmutex;

  public:

    RWSTDGuard  (RWSTDMutex& m);  // Acquire the mutex.
    ~RWSTDGuard ();               // Release the mutex.
};

/*
** For those OSs that require a non-zero mutex, we must treat static 
** mutexes specially; they may not be initialized when we need them.
** For efficiency, we do conditional compilation in several methods
** based on that need.
*/

inline RWSTDMutex::RWSTDMutex (RWSTDMutex::StaticCtor)
{
    //
    // Empty, because acquire() may already have been used.
    //
}
#include <iostream.h>

inline RWSTDMutex::~RWSTDMutex () 
{ 
#if defined(RWSTD_NEEDS_SEM_INIT)
    if (0 == initFlag)
        return;
    else
        initFlag = 0;
#endif
#if defined(sun)
    mutex_destroy(&mutex); 
#elif defined(RWSTD_POSIX_THREADS)
    pthread_mutex_destroy(&mutex); 
#elif defined(__WIN32__)
    DeleteCriticalSection(&mutex);
#elif defined(__OS2__)
    APIRET rv;
    RWSTD_THROW_NO_MSG(0 != (rv = DosCloseMutexSem(mutex)),
                thread_error);
#endif
}


void inline RWSTDMutex::init ()
{ 
#if defined(sun)
    mutex_init(&mutex, USYNC_THREAD, NULL); 
# elif defined(RWSTD_POSIX_THREADS)
#   if defined(RWSTD_POSIX_D10_THREADS)
#     if defined(RWSTD_MUTEXATTR_DEFAULT)
    pthread_mutex_init(&mutex, &pthread_mutexattr_default);
#      else
    pthread_mutex_init(&mutex, 0);
#      endif
#   else
    pthread_mutex_init(&mutex, pthread_mutexattr_default);
#   endif
#elif defined(__WIN32__)
    InitializeCriticalSection(&mutex);
#elif defined(__OS2__)
    APIRET rv;
    RWSTD_THROW_NO_MSG(0 != (rv = DosCreateMutexSem(0,&mutex,DC_SEM_SHARED,FALSE)),
                thread_error);
#endif
#if defined(RWSTD_NEEDS_SEM_INIT)
    initFlag = 1;
#endif
}  

inline RWSTDMutex::RWSTDMutex () 
{ 
  init(); 
}  // Initialize the mutex.   

inline void RWSTDMutex::acquire ()
{ 
#if defined(RWSTD_NEEDS_SEM_INIT)
    if(0 == initFlag)
        init();
#endif
#if defined(sun)
    mutex_lock(&mutex);    
#elif defined(RWSTD_POSIX_THREADS)
    pthread_mutex_lock(&mutex);    
#elif defined(__WIN32__)
    EnterCriticalSection(&mutex);
#elif defined(__OS2__)
    APIRET rv;
    RWSTD_THROW_NO_MSG(0 != (rv = DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)),
               thread_error); 
#endif
}

inline void RWSTDMutex::release ()
{ 
#if defined(sun)
    mutex_unlock(&mutex);  
#elif defined(RWSTD_POSIX_THREADS)
    pthread_mutex_unlock(&mutex);  
#elif defined(__WIN32__)
    LeaveCriticalSection(&mutex);
#elif defined(__OS2__)
    APIRET rv;
    RWSTD_THROW_NO_MSG(0 != (rv = DosReleaseMutexSem(mutex)),
                thread_error);
#endif
}

inline RWSTDGuard::RWSTDGuard (RWSTDMutex& m) : rwmutex(m)
{
    rwmutex.acquire();
}

inline RWSTDGuard::~RWSTDGuard () { rwmutex.release(); }


#define STDGUARD(a) RWSTDGuard(a)
#else
#define STDGUARD(a) 
#endif  /*RWSTD_MULTI_THREAD*/
#endif  /*__RWSTD_MUTEX_H__*/





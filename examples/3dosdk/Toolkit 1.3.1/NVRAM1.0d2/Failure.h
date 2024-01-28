/*
        File:		Failure.h

        Contains:	"Try/Catch" failure handling for Portfolio. Examples of
   it's use can be found in the file "Failure.doc."

        Written by:	Edward G. Harp

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                11/20/93	EGH		Added
   PopHandler, used it in ENDTRY macro. 11/13/93	EGH		New
   file.

        To Do:
*/

#ifndef _Failure_h
#define _Failure_h

#ifndef _TYPES_H
#include "types.h"
#endif

#ifndef _SETJMP_H
#include "setjmp.h"
#endif

#ifndef __GRAPHICS_H
#include "Graphics.h"
#endif

typedef struct FailInfo
{
  jmp_buf fi_Environment;       // the saved environment
  Err fi_Error;                 // the error that occured
  int32 fi_Message;             // some error message (you define it)
  struct FailInfo *fi_NextInfo; // next handler in the stack
} FailInfo, *FailInfoPtr;

/* Initialize the failure handler
 */
void InitFailureHandler (void);

/* Push a handler on the stack
 */
void PushHandler (FailInfoPtr failInfoP);

/* Pop a handler off of the stack
 */
void PopHandler (FailInfoPtr failInfoP);

/* Successful end of a Try block
 */
void Success (FailInfoPtr failInfoP);

/* Signal a failure
 */
void Failure (Err err, int32 message);

/* Pass the failure up to the next handler
 */
void Resignal (FailInfoPtr failInfoP);

/* Fail if there is an error
 */
void FailErr (Err err);

/* Fail if the pointer is NULL
 */
void FailNULL (void *failPtr);

/* Fail if the CCB is NULL
 */
void FailNullCCB (CCB *failCCB);

/* Fail if the boolean is FALSE
 */
void FailFalse (bool failBool);

/* Pointer to the stack of handlers
 * (not normally needed)
 */
extern FailInfoPtr gHandlerStack;

/* Handy macros for using failure handling:
 */

#define TRY                                                                   \
  {                                                                           \
    FailInfo failInfo1;                                                       \
                                                                              \
    PushHandler (&failInfo1);                                                 \
                                                                              \
    if (setjmp (failInfo1.fi_Environment) == 0)                               \
      {

#define CATCH                                                                 \
  Success (&failInfo1);                                                       \
  }                                                                           \
  else                                                                        \
  {

// finish off the handler (falls through)
#define ENDTRY                                                                \
  PopHandler (&failInfo1);                                                    \
  }                                                                           \
  }

// throw exception up to next handler (if there is one)
#define RETHROW                                                               \
  Resignal (&failInfo1);                                                      \
  }                                                                           \
  }

#endif


/*
        File:		baseUtils.h

        Contains:	Base utilities for folio call tests (Header)

        Written by:	Robert Chinn, Charlie Eckhaus

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <4>	 6/30/93	CDE		Added USE_STRCASECMP,
   changed #ifdef <3>	 6/29/93	CDE		Added define
   strncasecmp for 4B1
                 <2>	 6/29/93	CDE		General fixup per
   Robert Removed DBUG, FULLDBUG macros <1>	 6/22/93	CDE
   Prepared for public consumption

        To Do:
*/
#pragma include_only_once

#include "ctype.h"
#include "stdlib.h"
#include "types.h"
#include "varargs.h"

#include "audio.h"
#include "debug.h"

#include "folio.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "semaphore.h"
#include "task.h"

#include "stdio.h"
#include "strings.h"

#include "filestream.h"
#include "filestreamfunctions.h"

#include "graphics.h"
#include "operamath.h"

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#define STATUS(x)                                                             \
  {                                                                           \
    if (gShowStatus)                                                          \
      kprintf ("## %s\n", x);                                                 \
  }

#define HIWRD(aLong) (((aLong) >> 16) & 0xFFFF)
#define LOWRD(aLong) ((aLong)&0xFFFF)

#define TICKCOUNT() GetAudioTime () //  240 to the second
#define kTickStr " -> for %i ticks\n"

#define kPrintTickStr " -> for %i Audio ticks (240/second)\n"

#define kPlusChar "+"
#define kMinusChar "-"

/* switch constants */
#define kOff "off"
#define kOn "on"

/* Just showing help or running the test */
#define kAbortTest FALSE
#define kShowHelp FALSE
#define kRunTest TRUE

/* return this time value to break out of iterations */
#define kBreakTime -1L

/* functionality verification control */
#define kBeforeCall TRUE
#define kAfterCall FALSE

/* Error-checking constants */
#define kNone 0
#define kCont 1
#define kStop 2

/* Call selection constants */
#define kFullTest 0

/* Default values for standard tool options: */
#define kDefaultMinInnerIterations 1
#define kDefaultMaxInnerIterations 1
#define kDefaultMinOuterIterations 1
#define kDefaultMaxOuterIterations 1
#define kDefaultCheckErrors kCont
#define kDefaultCheckIncidentalErrors kStop
#define kDefaultVerify TRUE
#define kDefaultOutputTiming TRUE
#define kDefaultCheckFault FALSE
#define kDefaultCallSelector kFullTest

#ifdef USE_STRCASECMP
#define strncasecmp strcasecmp
#endif

//
//	PROTOTYPES
//

long power (long x, long y);
void pipeData (char *formatStr, ...);
Item GetMyItem (void);
Boolean GetNextParam (int theArgCount, char **theArgList, int paramIndx,
                      char *paramStr);
Boolean GetFlag (int theArgCount, char **theArgList, int *paramIndx,
                 char *paramName, bool *theFlag);
int32 SleepSeconds (int32 Secs);

/*
	File:		Portfolio.h

	Contains:	Common header files for 3DO Station

	Written by:	Neil Cormia

	Copyright:	© 1992 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	 1/28/93	dsm		Header file cleanup. Defined MAGNETO = 1 rather than trying to
									define this at compile time through compiler switches.
		 <3>	12/16/92	pgb		globaly defining DOAUDIO
		 <2>	12/14/92	JAY		Bringing headers upto Magneto Release
		 <1>	 12/9/92	JML		first checked in

	To Do:
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "graphics.h"
#include "hardware.h"
#include "operror.h"
#include "audio.h"
#include "operamath.h"

#include "form3do.h"

#define DOAUDIO	1

#define MAGNETO 1


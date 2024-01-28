/*****************************************************************************
 *	File:		Portfolio.h
 *
 *	Contains:	Common header files for 3DO (more than you probably
 *need)
 *
 *	Written by: Neil Cormia
 *
 *	Copyright:	(c) 1993-1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *	07/11/94  Ian	Standarized protective wrapper macro name to
 *PORTFOLIO_H. Standardized comment blocks. 01/28/93  dsm	Header file
 *cleanup. Defined MAGNETO = 1 rather than trying to define this at compile
 *time through compiler switches. 12/16/92  pgb	globaly defining DOAUDIO
 *	12/14/92  JAY	Bringing headers upto Magneto Release
 *	12/09/92  JML	first checked in
 *
 *	Implementation notes:
 *
 *	Think twice before including this file.  It includes way more stuff
 *	than any single source module is likely to need.  Using this header
 *	file will cost an awful lot in terms of wasted compile time.  You're
 *	much better off identifying which OS and Lib3DO header files you
 *	actually need, and including just those from within your source code.
 ****************************************************************************/

#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "Form3DO.h"
#include "audio.h"
#include "debug.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "operror.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#define DOAUDIO 1
#define MAGNETO 1

#endif

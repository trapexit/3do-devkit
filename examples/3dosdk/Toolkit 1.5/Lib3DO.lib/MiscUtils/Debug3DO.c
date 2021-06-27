/*****************************************************************************
 *	File:			Debug3DO.c
 *
 *	Contains:		Routines to report errors.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/25/94  Ian	Added logic to ensure that the semaphore we create is
 *					owned by the main parent task even if the first time
 *					through this code is run in the context of a thread.
 *					(Pity it doesn't work due to SetItemOwner() returning
 *					a NOTSUPPORTED error.  ::sigh::)
 *	03/27/94  Ian	Removed the ugly ArgsByValue hack and added vprintf()
 *					prototype.  Turns out vprintf() is present in clib.lib
 *					but its prototype is missing from stdio.h.
 *
 *	12/19/93  Ian 	New today.
 *
 *	Implementation notes:
 *	
 ****************************************************************************/

#include "Debug3DO.h"
#include "kernel.h"
#include "semaphore.h"
#include "operror.h"
#include "filefunctions.h"

/*----------------------------------------------------------------------------
 * misc static data
 *--------------------------------------------------------------------------*/

static Item		the_semaphore;
static int32	last_error;
static int32	module_line;
static char *	module_name;

static uint32	verbose_mask = 0xFFFFFFFF;

/*----------------------------------------------------------------------------
 * data for handling user hooks in diagnose handling
 *--------------------------------------------------------------------------*/

static char * default_diagnose_hook(int32 err, char *module, int32 line, char *sysErrMsg, char *fmt, va_list args);

static DiagnoseHookFunc *	diagnose_hook = default_diagnose_hook;

/*----------------------------------------------------------------------------
 * code...
 *--------------------------------------------------------------------------*/

/*****************************************************************************
 * default_diagnose_hook()
 *	The default output intercept routine, when the caller doesn't specify one.
 *	
 *	An intercept routine is expected to do these things:
 *	 - 	Return NULL to suppress printf() output by the standard report routine.
 *	 -	Return non-NULL: a pointer to a character string which appears as 
 *		the 'title' of the task/module/line output string.  (A pointer to an
 *		empty string ("") can be used for 'titleless' unsuppressed output.)
 *	 -	Supply an alternate 'system error message' string, by placing up to
 *		DIAGNOSE_SYSMSGBUFSIZE characters into the sysErrMsg buffer.  (Placing
 *		a zero at the start of the buffer suppresses system message output by
 *		the standard routines, meaning only the task/module/line id output
 *		string and any caller-specified formatted output would appear.)
 *	 -	Any other things you want to do with the output.  For example, you 
 *		can selectively suppress output, log messages to a file (with or
 *		without suppression of output by the standard report routine), or 
 *		even force termination of the task if that works for your needs.
 *	 -  If the intercept does not terminate the task, it is expected to 
 *		return.  (IE, it shouldn't longjmp() out or anything.)  Also, the
 *		intercept routine must not do anything that would cause re-entrance
 *		into the diagnose routines.  Both of these limitations are to prevent
 *		deadlocks on the semaphore used to sync the phase 1 & 2 processing.
 *
 *	This default intercept routine just supplies a title of ERROR or VERBOSE,
 *	keyed off the error code passed in.
 ****************************************************************************/

static char * default_diagnose_hook(int32 err, char *module, int32 line, char *sysErrMsg, char *fmt, va_list args)
{
	(void)module;
	(void)line;
	(void)sysErrMsg;
	(void)fmt;
	(void)args;
	
	return (err < 0) ? "ERROR" : "VERBOSE";
}

/*****************************************************************************
 * make_semaphore()
 *	Create the semaphore we need to sync phase 1 & 2 processing.  If we are
 *	running in the context of a thread, transfer ownership of the semaphore
 *	item to the parent task.
 ****************************************************************************/

static void make_semaphore(void)
{
	if ((the_semaphore = CreateSemaphore("DiagnoseSemaphore", 1)) < 0) {
		return; // oh well, we'll do without a semaphore then.  ::sigh::
	}
	
#if 0 // right now, SetItemOwner() returns NOTSUPPORTED.  Maybe the OS guys can fix this.
  {
	Err		err;
	Task *	curTask;
	
	curTask = CURRENTTASK;
	while (curTask->t_ThreadTask != NULL) {
		curTask = curTask->t_ThreadTask;
	}

	if (curTask != CURRENTTASK) {
		if ((err = SetItemOwner(the_semaphore, curTask->t.n_Item)) < 0) {
			printf("Debug3DO.c: SetItemOwner(semaphore) failed\n");
			PrintfSysErr(err);
			DeleteItem(the_semaphore);
			the_semaphore = 0;
			return;
		}
	}
  }
#endif

	LockItem(the_semaphore, 1);
}

/*****************************************************************************
 * Diagnose1_()
 *	Phase-1 diagnostic output; lock the single-thread semaphore so that we
 *	can write to static variables and ensure that output from multiple threads
 *	doesn't get all mixed together.  Save the parameters in the static vars
 *	for use by the phase-2 routine.
 ****************************************************************************/

void Diagnose1_(int32 err, char *moduleName, int32 moduleLine)
{

	if (LockItem(the_semaphore, 1) < 0) {
		make_semaphore();
	}
	
	last_error  = err;
	module_name = moduleName;
	module_line = moduleLine;
	
}

/*****************************************************************************
 * Diagnose2_()
 *	Phase-2 diagnostic output; assemble the caller-specified part of the
 *	output, and pass it and the phase-1 info along to the userhook routine.
 *	If the userhook routine doesn't want the output suppressed, write it
 *	to the debugger window using printf().  When all done, unlock the 
 *	single-thread semaphore.
 ****************************************************************************/

void Diagnose2_(char *fmt, ...)
{
	va_list	args;
	char *	msgTitle;
	char	sysErrMsg[DIAGNOSE_SYSMSGBUFSIZE];
	
	sysErrMsg[0] = 0;
	
	if (last_error >= 0) {
		if (last_error > 31 || !(verbose_mask & (1L << last_error))) {
			goto SKIP_OUTPUT;
		}
	} else if (last_error != DIAGNOSE_NOSYSMSG) {
		GetSysErr(sysErrMsg, sizeof(sysErrMsg), last_error);
	}

	va_start(args, fmt);

	msgTitle = diagnose_hook(last_error, module_name, module_line, sysErrMsg, fmt, args);
	if (msgTitle != NULL) {
		printf("%s %s %s(%ld): ", msgTitle, CURRENTTASK->t.n_Name, module_name, module_line);
		vprintf(fmt, args);
		if (sysErrMsg[0] != 0) {
			printf("%s\n", sysErrMsg);
		}
	}
	
	va_end(args);

SKIP_OUTPUT:

	UnlockItem(the_semaphore);
}

/*****************************************************************************
 * SetVerboseMask()
 *	Set verbose category bits as specified.
 *	Input is category bitmask; use VERBOSE_CATEGORY2MASK() to convert 
 *	category numbers to bitmasks for this function.
 *	Returns old verbose category bits.
 ****************************************************************************/

uint32 SetVerboseMask(uint32 maskBits)
{
	uint32 oldMask;
	
	oldMask = verbose_mask;
	verbose_mask = maskBits;
	return oldMask;
}

/*****************************************************************************
 * GetVerboseMask()
 *	Returns current verbose category bits.
 ****************************************************************************/

uint32 GetVerboseMask(void)
{
	return verbose_mask;
}

/*****************************************************************************
 * UnsetDiagnoseHook()
 *	Remove prior user-specified output intercept routine (if any).
 ****************************************************************************/

void UnsetDiagnoseHook(void)
{
	diagnose_hook = default_diagnose_hook;
}

/*****************************************************************************
 * SetDiagnoseHook()
 *	Install user-specified output intercept routine.
 ****************************************************************************/

void SetDiagnoseHook(DiagnoseHookFunc *newDiagnoseHookFunc)
{
	if (newDiagnoseHookFunc != NULL) {
		diagnose_hook = newDiagnoseHookFunc;
	} else {
		UnsetDiagnoseHook();
	}
}

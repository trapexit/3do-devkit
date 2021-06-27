/*
	File:		Failure.c

	Contains:	Failure handling for Portfolio.

	Written by:	Edward G. Harp

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				11/20/93	EGH		Added PopHandler, used it in ENDTRY macro.
				11/13/93	EGH		New file.

	To Do:
*/

#include "Failure.h"

FailInfoPtr gHandlerStack;		// pointer to the stack of handlers

/* InitFailureHandler
 *
 * Initialize the failure handler.
 */
void InitFailureHandler(void)
	{
	gHandlerStack = NULL;
	}

/* PushHandler
 *
 * Push a handler on the stack.
 */
void PushHandler(
	FailInfoPtr failInfoP)
	{
	failInfoP-> fi_NextInfo = gHandlerStack;
	gHandlerStack = failInfoP;
	failInfoP-> fi_Error = 0;
	failInfoP-> fi_Message = 0;
	}

/* PopHandler
 *
 * Pop a handler off of the stack.
 */
void PopHandler(
	FailInfoPtr failInfoP)
	{
	gHandlerStack = failInfoP-> fi_NextInfo;
	}

/* Success
 *
 * Successful end of a Try block.
 */
void Success(
	FailInfoPtr failInfoP)
	{
#ifdef _debug
	if (gHandlerStack != failInfoP)
		{
		printf("\pSuccess calls out of order!");
		}
#endif
	// pop this handler off of the stack
	gHandlerStack = failInfoP-> fi_NextInfo;
	}

/* Failure
 * 
 * Signal a failure.
 */
void Failure(
	Err		err,
	int32	message)
	{
	FailInfoPtr failInfoP = gHandlerStack;

	if (failInfoP)
		{
		// clear the handler off of the stack
		PopHandler(failInfoP);
		
		// set the error & message
		failInfoP-> fi_Error = err;
		failInfoP-> fi_Message = message;
		
		// jump to the handler block
		// this causes the original setjmp() call to return other than 0.
		longjmp(failInfoP-> fi_Environment, 1);
		}
	}

/* Pass the failure up to the next handler
 */
void Resignal(
	FailInfoPtr failInfoP)
	{
	Failure(failInfoP-> fi_Error, failInfoP-> fi_Message);
	}

/* FailErr
 * 
 * Fail if there is an error.
 */
void FailErr(
	Err err)
	{
	if (err < 0)
		Failure(err, 0);
	}

/* FailNULL
 *
 */
void FailNULL(
	void *failPtr)
	{
	if (failPtr == NULL)
		FailErr(-1);
	}

/* FailNullCCB
 *
 * Fail if the CCB * is zero.
 */
void FailNullCCB(
	CCB *failCCB)
	{
	if (failCCB == NULL)
		FailErr(-1);
	}

/* FailFalse
 */
void FailFalse(
	bool failBool)
	{
	if (!failBool)
		FailErr(-1);
	}


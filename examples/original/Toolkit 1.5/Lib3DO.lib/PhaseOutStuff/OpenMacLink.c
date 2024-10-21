/*****************************************************************************
 *	File:			OpenMacLink.c
 *
 *	Contains:		Routine to open the 'mac' device for
 *WriteMacFile().
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	This function is now obsolete.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Debug3DO.h"
#include "Init3DO.h"
#include "kernel.h"

Boolean
OpenMacLink (void)
{
  return TRUE;
}

#if 0 // obsolete code...

/*=========================================================
	GLOBAL VARIABLES ACCESSED BY OTHER LIBS

	NOTE: DO NOT TAMPER WITH THESE NAMES
	UNLESS YOU ALSO CHANGE THEIR REFERENCES
	IN PARSE3DO.LIB AND UTILS3DO.LIB.
--------------------------------------------------------*/
	
	Item TheIOPort = -1;
	IOReq* TheReplyPort = NULL;
	Item MacDev = -1;
	Item ReplyPort = -1;

/*========================================================*/

Boolean OpenMacLink(void)
{
	TagArg targs[3];
	
	if(KernelBase->kb_CPUFlags & KB_NODBGR) {
		return FALSE;
	}
	if ( (MacDev = OpenItem( FindNamedItem(MKNODEID (KERNELNODE, DEVICENODE),
				"mac"), 0) ) < 0 )
		{
		DIAGNOSE(("Cannot open Mac device"));
		return FALSE;
		}

	targs[0].ta_Tag = TAG_ITEM_NAME;
	targs[0].ta_Arg = (void *)"reply port";
	targs[1].ta_Tag = TAG_END;

	if ( (ReplyPort = CreateItem (MKNODEID (KERNELNODE, MSGPORTNODE),
				     targs)) < 0 )
		{
		DIAGNOSE(( "Can't create reply port.\n" ));
		return FALSE;
		}
	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *)MacDev;
	targs[1].ta_Tag = CREATEIOREQ_TAG_REPLYPORT;
	targs[1].ta_Arg = (void *)ReplyPort;
	targs[2].ta_Tag = TAG_END;
	if ( (TheIOPort = CreateItem( MKNODEID (KERNELNODE, IOREQNODE),
				 targs ))  < 0 )
		{
		DIAGNOSE(("Cannot create IO port"));
		return FALSE;
		}
	
	TheReplyPort = (IOReq *)LookupItem( TheIOPort );
	return TRUE;
}

#endif // obsolete

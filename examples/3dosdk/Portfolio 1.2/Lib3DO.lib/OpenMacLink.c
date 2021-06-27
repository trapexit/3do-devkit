#include "Portfolio.h"
#include "filefunctions.h"
#include "Init3DO.h"

#if DEBUG
#define DIAGNOSTIC(s)		kprintf("Error: %s\n", s)
#define DIAGSTRING(s,t)		kprintf("Fyi: %s %s\n", s ,t )
#else
#define DIAGNOSTIC(s)
#define DIAGSTRING(s,t)
#endif

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

bool OpenMacLink( void )
	{
	TagArg targs[3];
	
	if(KernelBase->kb_CPUFlags & KB_NODBGR) {
		return FALSE;
	}
	if ( (MacDev = OpenItem( FindNamedItem(MKNODEID (KERNELNODE, DEVICENODE),
				"mac"), 0) ) < 0 )
		{
		DIAGNOSTIC("Cannot open Mac device");
		return FALSE;
		}

	targs[0].ta_Tag = TAG_ITEM_NAME;
	targs[0].ta_Arg = (void *)"reply port";
	targs[1].ta_Tag = TAG_END;

	if ( (ReplyPort = CreateItem (MKNODEID (KERNELNODE, MSGPORTNODE),
				     targs)) < 0 )
		{
		DIAGNOSTIC( "Can't create reply port.\n" );
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
		DIAGNOSTIC("Cannot create IO port");
		return FALSE;
		}
	
	TheReplyPort = (IOReq *)LookupItem( TheIOPort );
	return TRUE;
	}

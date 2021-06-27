#ifndef __OPERROR_H
#define __OPERROR_H

/*****

$Id: operror.h,v 1.40 1994/03/25 22:26:44 dplatt Exp $

$Log: operror.h,v $
 * Revision 1.40  1994/03/25  22:26:44  dplatt
 * Add a BADAIF construct
 *
 * Revision 1.39  1994/03/16  23:06:27  dplatt
 * Add a device-ID for the Control Port
 *
 * Revision 1.38  1994/03/14  23:27:01  sdas
 * MakeErrId() and MakeTErr() macros defined; plus, some clean-ups
 *
 * Revision 1.37  1994/03/11  19:00:36  sdas
 * Added ER_INTL for international folio
 *
 * Revision 1.36  1994/02/25  01:10:16  limes
 * If DEVELOPMENT is not turned on, PrintError expands to nothing, not (0) ...
 *
 * Revision 1.35  1994/02/17  02:41:52  limes
 * Add support for PrintError macro which calls clib_PrintError
 *
 * Revision 1.34  1994/02/09  22:11:14  dplatt
 * Add two-letter device-driver-error codes for CD-ROM drivers ('CD') and
 * audio/video codecs ('CO').
 *
 * Revision 1.33  1994/02/03  22:19:39  dplatt
 * Add a core error code for "illegal I/O parameters".
 *
 * Revision 1.32  1994/02/03  22:06:11  dplatt
 * Add new core error codes: device offline, device error, medium error,
 * end of medium.
 *
 * Revision 1.31  1994/01/28  23:20:22  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.30  1994/01/21  18:18:27  sdas
 * C++ compatibility - updated
 *
 * Revision 1.29  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.28  1993/06/23  08:44:52  andy
 * defined new NOTOWNER standard error message
 *
 * Revision 1.27  1993/05/15  22:20:49  dale
 * ER_Kr_BadAIFHeader
 *
 * Revision 1.26  1993/04/14  23:32:48  dale
 * NOSUPPORT macro added
 *
 * Revision 1.25  1993/04/01  04:51:13  dale
 * ER_Kr_ReplyPortNeeded,ER_Kr_IllegalSignal,ER_Kr_BadLockArg
 *
 * Revision 1.24  1993/03/26  18:25:25  dale
 * ER_IOIncomplete
 *
 * Revision 1.23  1993/03/16  06:45:51  dale
 * api
 *
 * Revision 1.22  1993/03/13  23:15:44  dale
 * case fix to MAKEFErr
 *
 * Revision 1.21  1993/03/13  23:04:35  dale
 * hardware error, 'T' for task errors.
 *
 * Revision 1.20  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.19  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.18  1993/01/13  04:12:49  dale
 * new ErrorText kernelnode
 *
 * Revision 1.17  1993/01/03  17:47:23  dale
 * added extra () for some defines.
 *
 * Revision 1.16  1992/12/23  01:03:37  dale
 * new Kernel error
 *
 * Revision 1.15  1992/12/16  21:55:50  dale
 * printf and syserr
 *
 * Revision 1.14  1992/12/02  21:58:20  dale
 * LegalNames
 *
 * Revision 1.13  1992/11/25  00:01:25  dale
 * new msgs
 *
 * Revision 1.12  1992/11/18  00:06:31  dale
 * RSA stuff
 *
 * Revision 1.11  1992/10/24  01:41:07  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "types.h"
#include "nodes.h"
#include "item.h"

extern char _SixToAscii[];
#define	MakeASCII(n) (_SixToAscii[(n)])
#define	MAKEASCII(n) MakeASCII(n)


#define Make6Bit(n) ((n >= 'a')? (n-'a'+11): ((n <= '9')? (n-'0'+1):(n-'A'+37)))
#define MAKE6BIT(n) Make6Bit(n)

/* Bits in errfield */
#define ERR_ERRSIZE	(8)
#define ERR_CLASSIZE	(1)
#define ERR_ENVSIZE	(2)
#define ERR_SEVERESIZE	(2)
#define ERR_IDSIZE	(12)
#define ERR_OBJSIZE	(6)

#define ERR_ERRSHIFT	(0)
#define ERR_CLASHIFT	(ERR_ERRSHIFT+ERR_ERRSIZE)
#define ERR_ENVSHIFT	(ERR_CLASHIFT+ERR_CLASSIZE)
#define ERR_SEVERESHIFT	(ERR_ENVSHIFT+ERR_ENVSIZE)
#define ERR_IDSHIFT	(ERR_SEVERESHIFT+ERR_SEVERESIZE)
#define ERR_OBJSHIFT	(ERR_IDSHIFT+ERR_IDSIZE)

#define MAKEERR(o,id,severity,env,class,err) (0x80000000 \
					| ((id)<<ERR_IDSHIFT) \
					| (severity<<ERR_SEVERESHIFT) \
					| (env<<ERR_ENVSHIFT) \
					| (class<<ERR_CLASHIFT) \
			 | (o<<ERR_OBJSHIFT) | (err<<ERR_ERRSHIFT) )
#define MakeErr(o,id,severity,env,class,er) MAKEERR(o,id,severity,env,class,er)

/* Make ID from the 2-char erring object name */
#define	MakeErrId(ch1,ch2) ((Make6Bit((uchar)(ch1))<<6)|Make6Bit((uchar)(ch2)))
#define	MAKEERRID(ch1,ch2) MakeErrId(ch1,ch2)

/* Objects */
#define ER_FOLI	Make6Bit('F')
#define ER_DEVC	Make6Bit('D')
#define ER_TASK	Make6Bit('T')

/* Folio ids */
#define	ER_KRNL	MakeErrId('K','r')
#define	ER_GRFX	MakeErrId('G','r')
#define	ER_FSYS	MakeErrId('F','S')
#define	ER_ADIO	MakeErrId('A','u')
#define	ER_TIMR	MakeErrId('T','M')
#define	ER_INTL	MakeErrId('I','N')

/* Device-Driver ids */
#define ER_CDROM MakeErrId('C','D')
#define ER_CODEC MakeErrId('C','O')
#define ER_CPORT MakeErrId('C','P')

/* Severity */
#define ER_INFO	0
#define ER_WARN	1
#define ER_SEVERE	2
#define ER_SEVER	ER_SEVERE	/* for old code */
#define ER_KYAGB	3
#define ER_FATAL	ERR_KYAGB

/* Environment */
#define ER_E_SSTM	0
#define ER_E_APPL	1
#define ER_E_USER	2
#define ER_E_RESERVED	3

/* Class */
#define ER_C_STND	0
#define ER_C_NSTND	1

/* Core Error Numbers */

#define BadItem	1		/* Should use ERR_BadItem instead */
#define ER_BadItem	1	/* undefined Item passed in */
#define ER_BadTagArg	2	/* undefined tag */
#define ER_BadTagArgVal	3	/* bad value in tagarg list */
#define ER_NotPrivileged 4	/* attempt to do privileged op by nonpriv task */
#define ER_NotFound	5	/* Item with that name not found */
#define ER_NoMem	6	/* insufficient memory to complete */
#define ER_BadSubType	7	/* Bad SubType for this Folio/Cmd*/
#define ER_SoftErr	8	/* System Software error, should not */
				/* happen */
#define ER_BadPtr	9	/* ptr/range is illegal for this task */
#define ER_Aborted	10	/* operation aborted */
#define ER_BadUnit	11	/* Bad Unit field in IOReq */
#define ER_BadCommand	12	/* Bad Command field in IOReq */
#define ER_BadIOArg	13	/* Bad field in IOReq */
#define ER_BadName	14	/* Bad Name for Item */
#define ER_IONotDone	15	/* IO in progress for this IOReq */
#define ER_NotSupported	16	/* Function not supported in this rev */
#define ER_IOIncomplete	17	/* IO terminated but incomplete (bytes left over) */
#define ER_NotOwner	18	/* Attempt to manipulate object not owned by task */
#define ER_DeviceOffline 19     /* Device is off-line and inaccessible */
#define ER_DeviceError  20      /* I/O error due to device/hardware trouble */
#define ER_MediaError   21      /* I/O error due to media (e.g. scratched CD) */
#define ER_EndOfMedium  22      /* Physical/logical end of medium or file */
#define ER_ParamError   23      /* I/O error due to illegal operation or parameters */

/* Kernel-only Error Numbers */
#define ER_Kr_BadType		1	/* No such Folio for 'type' */
#define ER_Kr_RsrcTblOvfFlw	3	/* could not make room */
					/* for new item in resource table */
#define ER_Kr_BadDrvrItem	4	/* DriverItem is not valid */
#define ER_Kr_ItemNotOpen	5	/* Item is not opened by you */
#define ER_Kr_ItemTableFull	6	/* Item Table Full */
#define ER_Kr_BadMemCmd		7	/* Bad Command for ControlMem */
#define ER_Kr_NoSigs		8	/* Out of signals */
#define ER_Kr_MsgSent		9	/* Msg already sent */
#define ER_Kr_NoReplyPort	10	/* No Reply Port in Msg */
#define ER_Kr_BadSize		11	/* size less than min or grtr than max*/
#define ER_Kr_BadPriority	12	/* Bad Priority for SetItemPri */
#define ER_Kr_ThreadError	13	/* illegal operation for thread */
#define ER_Kr_BadQuanta		14	/* bad quanta size request */
#define ER_Kr_BadStackSize	15	/* bad stack size request */
#define ER_Kr_NoTimer		16	/* No more hardware timers */
#define ER_Kr_CantOpen		17	/* Can't open that item */
#define ER_Kr_RSAFail		18	/* RSA failed in CreateTask */
#define ER_Kr_NoHardware	19	/* No Devices on Expansion Bus */
#define ER_Kr_XBUSOverFlow	20	/* Too many Devices on Expansion Bus */
#define ER_Kr_ReplyPortNeeded	21	/* Reply needed in this call */
#define ER_Kr_IllegalSignal	22	/* attempt to use an unallocated signal */
#define ER_Kr_BadLockArg	23	/* arg to LockItem is bad */
#define ER_Kr_BadAIFHeader	24	/* Bad AIF Header detected creating task */

/* Timer-device Error Numbers */
#define ER_TmrDrv_InitErr	1

/* Make Folio-specific Errors */
#define MakeKErr(svr,class,err) MakeErr(ER_FOLI,ER_KRNL,svr,ER_E_SSTM,class,err)
#define MakeGErr(svr,class,err) MakeErr(ER_FOLI,ER_GRFX,svr,ER_E_SSTM,class,err)
#define MakeFErr(svr,class,err) MakeErr(ER_FOLI,ER_FSYS,svr,ER_E_SSTM,class,err)
#define MakeIErr(svr,class,err) MakeErr(ER_FOLI,ER_INTL,svr,ER_E_SSTM,class,err)

#define MAKEKERR(svr,class,err) MakeKErr(svr,class,err)
#define MAKEGERR(svr,class,err) MakeGErr(svr,class,err)
#define MAKEFERR(svr,class,err) MakeFErr(svr,class,err)
#define MAKEIERR(svr,class,err) MakeIErr(svr,class,err)

/* Make Task-specific Errors */
#define MakeTErr(id,svr,class,err) MakeErr(ER_TASK,id,svr,ER_E_APPL,class,err)
#define MAKETERR(id,svr,class,err) MakeTErr(id,svr,class,err)

/* Kernel Short-cuts */
#define BADTAG		MakeKErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define BADTAGVAL	MakeKErr(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define BADPRIV		MakeKErr(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define BADSUBTYPE	MakeKErr(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define BADITEM		MakeKErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define BADSIZE		MakeKErr(ER_SEVERE,ER_C_STND,ER_Kr_BadSize)
#define NOMEM		MakeKErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define BADPTR		MakeKErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define BADUNIT		MakeKErr(ER_SEVERE,ER_C_STND,ER_BadUnit)
#define BADIOARG	MakeKErr(ER_SEVERE,ER_C_STND,ER_BadIOArg)
#define BADCOMMAND	MakeKErr(ER_SEVERE,ER_C_STND,ER_BadCommand)
#define ABORTED		MakeKErr(ER_SEVERE,ER_C_STND,ER_Aborted)
#define BADNAME		MakeKErr(ER_SEVERE,ER_C_STND,ER_BadName)
#define IOINCOMPLETE	MakeKErr(ER_SEVERE,ER_C_STND,ER_IOIncomplete)
#define NOSUPPORT	MakeKErr(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define ILLEGALSIGNAL	MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_IllegalSignal)
#define NOTOWNER	MakeKErr(ER_SEVERE,ER_C_STND,ER_NotOwner)
#define BADAIF          MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadAIFHeader)

typedef struct ErrorText
{
	ItemNode et;
	uint32	et_ObjID;	/* 12 bit identifier */
	uint8	et_MaxErr;	/* max size of table */
	uint8	et_MaxStringSize;	/* size of largest string */
	uint8	et_Reserved[2];
	char	**et_ErrorTable;	/* ptr to table of char * */
} ErrorText;

enum errtxt_tags
{
	ERRTEXT_TAG_OBJID = TAG_ITEM_LAST+1,
	ERRTEXT_TAG_MAXERR,
	ERRTEXT_TAG_TABLE,
	ERRTEXT_TAG_MAXSTR
};
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

void PrintfSysErr(Item);	/* printfs the error */
int32 GetSysErr(char *buff,int32 buffsize,Item err);	/* fills buffer with error text */

#ifdef	DEVELOPMENT
void	clib_PrintError(char *who, char *what, char *whom, Err err);
#define	PrintError(who, what, whom, err)	\
	clib_PrintError(((char *)(who)),((char *)(what)),((char *)(whom)),((Err)(err)))
#else
#define	PrintError(who, what, whom, err)	/* nothing! */
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OPERROR_H */

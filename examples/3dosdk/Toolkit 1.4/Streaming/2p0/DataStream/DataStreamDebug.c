/*******************************************************************************************
 *	File:			DataStreamDebug.h
 *
 *	Contains:		definitions for DataStreamDebug.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	11/11/93	jb		Use PrintfSysErr if we don't understand the
 *error code; Add include of "Debug3DO.h".
 *	11/9/93		jb		Added kDSRangeErr, kDSUnImplemented,
 *and kDSBranchNotDefined 10/4/93		jb		Added
 *kDSEOSRegistrationErr 8/16/93		jb		Added
 *kDSInvalidDSRequest
 *	8/5/93		jb		Remove kDSNoSemErr message -- no longer
 *used. 7/10/93		jb		Add kDSNoSemErr. Use DEBUG compile
 *switch to turn off all diagnostic output if it is set == zero. 6/15/93
 *jb		Add kDSClockNotValidErr 6/2/93		jb Add
 *kDSBadConnectPortErr
 *	5/24/93		jb		Add error mnemonic to error text
 *	5/21/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __DATASTREAMDEBUG_H__
#include "DataStreamDebug.h"
#endif

#ifndef __DEBUG3DO_H__
#include "Debug3DO.h"
#endif

/*******************************************************************************************
 * Routine to decypher a DataStream library result code into printable text and
 *print it on the debugging console.
 *******************************************************************************************/
void
PrintfDSError (int32 DSStatus)
{
#if DEBUG
  char *err;

  switch (DSStatus)
    {
    case kDSAbortErr:
      err = "kDSAbortErr - some error occurred";
      break;
    case kDSEndOfFileErr:
      err = "kDSEndOfFileErr - end of file reached";
      break;
    case kDSWasFlushedErr:
      err = "kDSWasFlushedErr - buffer was flushed";
      break;
    case kDSNotRunningErr:
      err = "kDSNotRunningErr - stream not running";
      break;
    case kDSWasRunningErr:
      err = "kDSWasRunningErr - stream already running";
      break;
    case kDSNoPortErr:
      err = "kDSNoPortErr - couldn't allocate a message port for stream";
      break;
    case kDSNoMsgErr:
      err = "kDSNoMsgErr - couldn't allocate message item for a data buffer";
      break;
    case kDSNotOpenErr:
      err = "kDSNotOpenErr - stream not open";
      break;
    case kDSNoMemErr:
      err = "kDSNoMemErr - couldn't allocate needed memory";
      break;
    case kDSNoSignalErr:
      err = "kDSNoSignalErr - couldn't allocate needed signal";
      break;
    case kDSSignalErr:
      err = "kDSSignalErr - problem sending/receiving a signal";
      break;
    case kDSNoReplyPortErr:
      err = "kDSNoReplyPortErr - message requires a reply port";
      break;
    case kDSBadConnectPortErr:
      err = "kDSBadConnectPortErr - invalid port specified for data "
            "connection";
      break;
    case kDSSubDuplicateErr:
      err = "kDSSubDuplicateErr - duplicate subscriber";
      break;
    case kDSSubMaxErr:
      err = "kDSSubMaxErr - subscriber table full, too many subscribers";
      break;
    case kDSSubNotFoundErr:
      err = "kDSSubNotFoundErr - specified subscriber not found";
      break;
    case kDSInvalidTypeErr:
      err = "kDSInvalidTypeErr - invalid subscriber data type specified";
      break;
    case kDSBadBufAlignErr:
      err = "kDSBadBufAlignErr - buffer list contains non QUADBYTE aligned "
            "buffer";
      break;
    case kDSBadChunkSizeErr:
      err = "kDSBadChunkSizeErr - chunk size in stream is a non-QUADBYTE "
            "multiple";
      break;
    case kDSInitErr:
      err = "kDSInitErr - some internal initialization failed";
      break;
    case kDSClockNotValidErr:
      err = "kDSClockNotValidErr - clock dependent call failed because clock "
            "not set";
      break;
    case kDSInvalidDSRequest:
      err = "kDSInvalidDSRequest - unknown request message send to server "
            "thread";
      break;
    case kDSEOSRegistrationErr:
      err = "kDSEOSRegistrationErr - EOS registrant replaced by new "
            "registrant";
      break;
    case kDSRangeErr:
      err = "kDSRangeErr - parameter out of range";
      break;
    case kDSUnImplemented:
      err = "kDSUnImplemented - requested function not implemented";
      break;
    case kDSBranchNotDefined:
      err = "kDSBranchNotDefined - branch destination not defined";
      break;

    default:
      err = NULL;
    }

  printf ("%s : %lx\n", err ? err : "unknown DataStream error", DSStatus);

  if (err == NULL)
    PrintfSysErr (DSStatus);
#endif
}

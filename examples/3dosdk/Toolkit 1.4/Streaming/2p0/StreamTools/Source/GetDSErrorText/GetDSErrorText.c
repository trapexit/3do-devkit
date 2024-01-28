/*******************************************************************************************
 *	File:			GetDSErrorText.c
 *
 *	Contains:		MPW tool to translate a 3DO Data Streaming error
 *code to error text
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	11/22/93		jb		Added high level Cinepak player
 *function error codes. 11/15/93		jb		New today.
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1"

#include "StdIO.h"
#include "StdLib.h"

typedef enum DSErrorCode
{
  kDSNoErr = 0,            /* no error, successful completion */
  kDSAbortErr = -1000,     /* some error occurred */
  kDSEndOfFileErr = -999,  /* end of file reached */
  kDSWasFlushedErr = -998, /* buffer was flushed */
  kDSNotRunningErr = -997, /* stream not running */
  kDSWasRunningErr = -996, /* stream already running */
  kDSNoPortErr = -995,     /* couldn't allocate a message port for stream */
  kDSNoMsgErr = -994,    /* couldn't allocate message item for a data buffer */
  kDSNotOpenErr = -993,  /* stream not open */
  kDSNoMemErr = -992,    /* couldn't allocate needed memory */
  kDSNoSignalErr = -991, /* couldn't allocate needed signal */
  kDSSignalErr = -990,   /* problem sending/receiving a signal */
  kDSNoReplyPortErr = -989,    /* message requires a reply port */
  kDSBadConnectPortErr = -988, /* invalid port specified for data connection */
  kDSSubDuplicateErr = -987,   /* duplicate subscriber */
  kDSSubMaxErr = -986,      /* subscriber table full, too many subscribers */
  kDSSubNotFoundErr = -985, /* specified subscriber not found */
  kDSInvalidTypeErr = -984, /* invalid subscriber data type specified */
  kDSBadBufAlignErr = -983, /* buffer list passed to DSHOpenStream contains */
                            /* ... a non QUADBYTE aligned buffer */
  kDSBadChunkSizeErr
  = -982,            /* chunk size in stream is a non-QUADBYTE multiple */
  kDSInitErr = -981, /* some internal initialization failed */
  kDSClockNotValidErr
  = -980, /* clock dependent call failed because clock not set */
  kDSInvalidDSRequest
  = -979, /* unknown request message send to server thread */
  kDSEOSRegistrationErr = -978, /* EOS registrant replaced by new registrant */
  kDSRangeErr = -977,           /* parameter out of range */
  kDSUnImplemented = -976,      /* requested function not implemented */
  kDSBranchNotDefined = -975,   /* branch destination not defined */

  /* High level Cinpak Player errors */

  kPSVersionErr = -2001, /* header version incompatible with player code */
  kPSMemFullErr = -2002, /* can't allocate memory to play stream */
  kPSUnknownSubscriber = -2003, /* required subscriber unknown to player */
  kPSHeaderNotFound = -2004     /* stream header required by not found */

};

/*******************************************************************************************
 * Routine to decypher a DataStream library result code into printable text and
 *print it on the debugging console.
 *******************************************************************************************/
void
PrintfDSError (long DSStatus)
{
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

      /* High level Cinpak Player errors */

    case kPSVersionErr:
      err = "kPSVersionErr - header version incompatible with player code";
      break;
    case kPSMemFullErr:
      err = "kPSMemFullErr - can't allocate memory to play stream";
      break;
    case kPSUnknownSubscriber:
      err = "kPSUnknownSubscriber - required subscriber unknown to player";
      break;
    case kPSHeaderNotFound:
      err = "kPSHeaderNotFound - stream header required by not found";
      break;

    default:
      err = "unknown DataStream error";
    }

  printf ("%s : 0x%lX (%ld)\n", err, DSStatus, DSStatus);
}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *commandNamePtr)
{
  fprintf (stderr, "%s version %s\n", commandNamePtr, PROGRAM_VERSION_STRING);
  fprintf (stderr, "usage: %s <Data Streaming error code>\n", commandNamePtr);
}

/*******************************************************************************************
 * Main program entrypoint
 *******************************************************************************************/
int
main (int argc, char *argv[])
{
  long dsResult;

  if (argc != 2)
    {
      Usage (argv[0]);
      return EXIT_FAILURE;
    }

  /* Convert the command line arg to binary. Note that the 'base', or 3rd
   * argument to strtol() set to zero allows for decimal, hex, octal, or
   * binary input.
   */
  sscanf (argv[1], "%li", &dsResult);

  /* Let library routine diagnose error code */
  PrintfDSError (dsResult);

  return EXIT_SUCCESS;
}

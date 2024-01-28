/*******************************************************************************************
 *	File:			SAudioControlMsgs.h
 *
 *	Contains:		Data Structures for passing control messages to
 *SAudioSubscriber.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/23/94		rdg		Version 2.9.3
 *						change typedef enum to enum to avoid
 *compler complaints 5/31/94		rdg		Version 2.9.2 Make
 *"channel" function names unique
 *	1/20/94		rdg		make C++ compatible
 *	10/14/93	rdg		Version 2.8
 *						Split source into separate
 *files.
 */

#ifndef __SACONTROLMSGS_H__
#define __SACONTROLMSGS_H__

#include "types.h"

/***********************************************/
/* Opcode values as passed in control messages */
/***********************************************/
enum SAudioControlOpcode
{
  kSAudioCtlOpLoadTemplates = 0,
  kSAudioCtlOpSetVol,
  kSAudioCtlOpSetPan,
  kSAudioCtlOpSetClockChan,
  kSAudioCtlOpGetVol,
  kSAudioCtlOpGetFreq,
  kSAudioCtlOpGetPan,
  kSAudioCtlOpGetClockChan,
  kSAudioCtlOpCloseChannel,
  kSAudioCtlOpFlushChannel,
  kSAudioCtlOpMute,
  kSAudioCtlOpUnMute
};

/**************************************/
/* Control block 					  */
/**************************************/
typedef union SAudioCtlBlock
{

  struct
  {                   /* kSAudioCtlOpLoadTemplates */
    long *tagListPtr; /* NULL terminated tag list pointer */
  } loadTemplates;

  struct
  {                     /* kSAudioCtlOpSetVol, kSAudioCtlOpGetVol */
    long channelNumber; /* which channel to control */
    long value;
  } amplitude;

  struct
  {                     /* kSAudioCtlOpSetPan, kSAudioCtlOpGetPan */
    long channelNumber; /* which channel to control */
    long value;
  } pan;

  struct
  {                     /* kSAudioCtlOpMuteSAudioChannel */
    long channelNumber; /* channel to make clock channel */
  } mute;

  struct
  {                     /* kSAudioCtlOpUnMuteSAudioChannel */
    long channelNumber; /* channel to make clock channel */
  } unMute;

  struct
  {                     /* kSAudioCtlOpSetClockChannel */
    long channelNumber; /* channel to make clock channel */
  } clock;

  struct
  {                     /* kSAudioCtlOpCloseChannel */
    long channelNumber; /* channel to close */
  } CloseSAudioChannel;

  struct
  {                     /* kSAudioCtlOpFlushChannel */
    long channelNumber; /* channel to flush */
  } FlushSAudioChannel;

} SAudioCtlBlock, *SAudioCtlBlockPtr;

#endif /* __SACONTROLMSGS_H__ */

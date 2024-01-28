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
 *
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
typedef enum SAudioControlOpcode
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
  {                     /* kSAudioCtlOpMuteChannel */
    long channelNumber; /* channel to make clock channel */
  } mute;

  struct
  {                     /* kSAudioCtlOpUnMuteChannel */
    long channelNumber; /* channel to make clock channel */
  } unMute;

  struct
  {                     /* kSAudioCtlOpSetClockChannel */
    long channelNumber; /* channel to make clock channel */
  } clock;

  struct
  {                     /* kSAudioCtlOpCloseChannel */
    long channelNumber; /* channel to close */
  } closeChannel;

  struct
  {                     /* kSAudioCtlOpFlushChannel */
    long channelNumber; /* channel to flush */
  } flushChannel;

} SAudioCtlBlock, *SAudioCtlBlockPtr;

#endif /* __SACONTROLMSGS_H__ */

/*********************************************************************************
 *	File:			SAErrors.h
 *
 *	Contains:		Error codes returned by SAudioSubscriber.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/23/94		rdg		Version 2.8.1
 *						change typedef enum to enum to avoid
 *compler complaints 1/20/94		rdg		make C++ compatible
 *	10/15/93	rdg		Version 2.8 -- New Today
 *						Split source into separate
 *files.
 */

#ifndef __SAERRORS_H__
#define __SAERRORS_H__

#include "types.h"

enum SAudioErrorCode
{
  SAudioErrTemplateNotFound
  = -7000,                     /* required instrument template not found */
  SAudioErrChanOutOfRange,     /* channel number not valid in context */
  SAudioErrVolOutOfRange,      /* Volume not within 0x0 - 0x7FFF */
  SAudioErrPanOutOfRange,      /* Pan not within 0x0 - 0x7FFF */
  SAudioErrAllocInstFailed,    /* Could not allocate some DSP instrument */
  SAudioErrUnsupSoundfileType, /* Format of audio data not playable by
                                  subscriber */
  SAudioErrConnectInstFailed,  /* Could not connect some DSP instrument to
                                  another */
  SAudioErrGrabKnobFailed, /* Could not grab a knob on some DSP instrument */
  SAudioErrCouldntAllocBufferPool,     /* Could not allocate a pool of buffer
                                          structures */
  SAudioErrNumAudioChannelsOutOfRange, /* Not mono or stereo audio data */
  SAudioErrUnableToFreeInstrument,     /* Could not free some DSP instrument */
  SAudioErrCouldntCreateContextPool, /* Create the memory pool for SASubscriber
                                        contexts */
  SAudioErrStreamVersionInvalid /* Subscriber not able to play this version of
                                   SAudio data */
};

#endif /* __SAERRORS_H__ */

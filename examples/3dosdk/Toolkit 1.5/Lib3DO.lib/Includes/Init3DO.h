/*****************************************************************************
 *	File:		Init3DO.h
 *
 *	Contains:	This file contains routines to initialize all the major
 *components of a 3DO application -- graphics, sound, DMA and file I/O.
 *
 *	Written by: Neil Cormia
 *
 *	Copyright:	(c) 1993-1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *	07/11/94  Ian	Standarized protective wrapper macro name to INIT3DO_H.
 *					Standardized comment blocks.
 *	05/12/94  Ian	Added prototype for CloseGraphics().
 *	04/05/93  JAY	adding function prototype for ChangeInitialDirectory()
 *	01/28/93  dsm	Creating a single global pointer to a ScreenContext
 *Structure containing what used to all the globals created by InitGraphics.
 *					Replaced the old style int1616 fixed point with
 *the new style frac16 fixed point math from OperaMath.h. 12/27/92  dsm Adding
 *constants for bit flags to be returned by StartUp so we can separately check
 *for success of opening Graphics, Audio, SportIO, and MacLink. 12/10/92  JML
 *gBitmaps and gBitmapItems are now external variables. 12/10/92  JML
 *nFrameByteCount is now an external variable.
 *	12/10/92  JML	nFrameBufferPages is now an external variable.
 *	12/09/92  JML	first checked in
 *
 *	Implementation notes:
 *
 *	This header is all but obsolete.  It includes the current proper
 *headers (DisplayUtils).  It exists for compatibility for old code.
 *
 *  About the stuff below identified as 'Obsolete stuff':
 *
 *	- This stuff will disappear from future releases of the library.
 *
 *	- OpenSPORT() and OpenMacLink() do nothing, just remove calls from your
 *code.
 *
 *	- All the remaining stuff is related to some bad logic for managing
 *simple sound effects.  If you're using this, please replace it with more
 *	  conventional sound management routines such as those documented in
 *the "Putting Sound Through the Speakers" section of the 3DO manuals.
 *
 *	- If you're using OpenAudio() and ShutDown() but not the other stuff,
 *just change your code to call OpenAudioFolio() and CloseAudioFolio()
 *	  respectively, and you're in business.
 *
 ****************************************************************************/

#pragma include_only_once

#ifndef INIT3DO_H
#define INIT3DO_H

#include "DisplayUtils.h"

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

  Err ChangeInitialDirectory (char *firstChoice, char *secondChoice,
                              Boolean always);

  /*----------------------------------------------------------------------------
   * Obsolete stuff.
   *--------------------------------------------------------------------------*/

  typedef struct SoundInfo
  {
    Item iSoundEffect;
    Item iSoundData;
    Item iSoundTemplate;
    Item iSoundAttachment;
    struct SoundInfo *next;
  } SoundInfo;

  Boolean OpenMacLink (void);
  Boolean OpenSPORT (void);

  Boolean OpenAudio (void);
  void ShutDown (void);

  Boolean SetChannel (Item iInstrument, int nChannel);
  Boolean SetMixer (int nChannel, int32 nVolume, int32 nBalance);
  Boolean SetMixerChannel (int nChannel, int32 nLeftLevel, int32 nRightLevel);
  Item LoadSoundEffect (char *sFilename, int nNumVoices);
  Item LoadSoundFX (char *sFilename, int nNumVoices, SoundInfo *theSoundInfo);
  void FreeSoundEffects (void);

#ifdef __cplusplus
}
#endif

extern Item TheMixer;

#endif

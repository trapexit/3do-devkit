#ifndef __INIT3DO_H
#define __INIT3DO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: init3do.h,v 1.6 1994/10/05 17:34:41 vertex Exp $
**
**  Lib3DO file containing routines to initialize all the major components
**  of a 3DO application -- graphics, sound, DMA and file I/O.
**
**  This header is all but obsolete.  It includes the current proper headers
**  (DisplayUtils).  It exists for compatibility for old code.
**
**  About the stuff below identified as 'Obsolete stuff':
**
**    - This stuff will disappear from future releases of the library.
**
**    - OpenSPORT() and OpenMacLink() do nothing, just remove calls from your code.
**
**    - All the remaining stuff is related to some bad logic for managing simple
**      sound effects.  If you're using this, please replace it with more
**      conventional sound management routines such as those documented in the
**      "Putting Sound Through the Speakers" section of the 3DO manuals.
**
**    - If you're using OpenAudio() and ShutDown() but not the other stuff, just
**      change your code to call OpenAudioFolio() and CloseAudioFolio()
**      respectively, and you're in business.
**
******************************************************************************/


#include "displayutils.h"

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

Err	ChangeInitialDirectory(char *firstChoice, char *secondChoice, Boolean always);

/*----------------------------------------------------------------------------
 * Obsolete stuff.
 *--------------------------------------------------------------------------*/

typedef struct SoundInfo {
	Item		iSoundEffect;
	Item		iSoundData;
	Item		iSoundTemplate;
	Item		iSoundAttachment;
	struct SoundInfo	*next;
} SoundInfo;

Boolean	OpenMacLink(void);
Boolean	OpenSPORT(void);

Boolean	OpenAudio(void);
void	ShutDown(void);

Boolean	SetChannel(Item iInstrument, int nChannel);
Boolean	SetMixer(int nChannel, int32 nVolume, int32 nBalance);
Boolean	SetMixerChannel(int nChannel, int32 nLeftLevel, int32 nRightLevel);
Item 	LoadSoundEffect(char* sFilename, int nNumVoices);
Item 	LoadSoundFX(char* sFilename, int nNumVoices, SoundInfo *theSoundInfo);
void 	FreeSoundEffects(void);

#ifdef __cplusplus
  }
#endif

extern Item	TheMixer;

#endif /* __INIT3DO_H */

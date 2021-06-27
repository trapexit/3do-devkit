#ifndef _EFFECTSHANDLER_H
#define _EFFECTSHANDLER_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: effectshandler.h,v 1.9 1995/01/08 04:20:45 ceckhaus Exp $
**
******************************************************************************/


#include "types.h"
#include "operror.h"
#include "debug3do.h"

/*** Error-handling definitions ***/

#if DEBUG
#define	EHERR( errNum, message )								\
			{													\
				PRT( ("%s %s", __FILE__, __LINE__) );			\
   				PrintError( NULL, message, NULL, (errNum) );	\
			}
#else
#define EHERR( errNum, message )
#endif

#define ER_EH						MakeErrId( 'E', 'h' )
#define MAKEEHERR( class, errNum )	MakeErr( ER_USER, ER_EH, ER_SEVERE, ER_E_USER, class, errNum )

/* Standard errors */
#define EHNOMEM_ERR				MAKEEHERR ( ER_C_STND, ER_NoMem )

/* Non-standard errors */
#define EHNOINIT_ERR			MAKEEHERR ( ER_C_NSTND, 1 ) 
#define EHALREADYINIT_ERR		MAKEEHERR ( ER_C_NSTND, 2 ) 
#define EHARG_ERR				MAKEEHERR ( ER_C_NSTND, 3 ) 
#define EHNOPLAYER_ERR			MAKEEHERR ( ER_C_NSTND, 4 )
#define EHNUMOUTPUTS_ERR		MAKEEHERR ( ER_C_NSTND, 5 )


/*** Effects-handling definitions ***/

#define		kEqualBalance	0x4000
#define		kMaxBalance		0x8000

typedef struct TMixerInfoTag
{
	Item	mi_Mixer;
	int32	mi_ChannelsUsed;
} TMixerInfo, *pTMixerInfo;

typedef enum TOutputTag
{
	kLeftOutput,
	kRightOutput
} TOutput;

typedef struct TSampleInfoTag
{
	Item		si_Sample;
	Item		si_Player;
	Item		si_Attachment;
	int32		si_Channel;
	Item		si_LeftGainKnob;
	Item		si_RightGainKnob;
} TSampleInfo, *pTSampleInfo;

extern Err			ehNewMixerInfo( pTMixerInfo *pNewMixerInfo, int32 channelsUsed, const char *mixerName );
extern void 		ehDisposeMixerInfo( pTMixerInfo pMixerInfo );

extern pTSampleInfo	ehNewSampleInfo( void );
extern void			ehDisposeSampleInfo( pTSampleInfo pSampleInfo );

extern Err			ehSetupSamplePlayer( pTSampleInfo pSampleInfo, const char *sampleName );
extern Err			ehConnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo, int32 channel );
extern Err			ehDisconnectSamplePlayer( pTMixerInfo pMixerInfo, pTSampleInfo pSampleInfo );
extern Err			ehLoadSoundEffect( pTSampleInfo *pNewSampleInfo, pTMixerInfo pMixerInfo, const char *sampleName, int32 channel );

extern void			ehSetChannelLevels( pTMixerInfo pMixerInfo, Item leftKnob, Item rightKnob, int32 volume, int32 balance);

#endif /* _EFFECTSHANDLER_H */

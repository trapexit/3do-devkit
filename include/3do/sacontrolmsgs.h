#ifndef __SACONTROLMSGS_H__
#define __SACONTROLMSGS_H__

#include "types.h"

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

typedef union SAudioCtlBlock {

  struct {						/* kSAudioCtlOpLoadTemplates */
    long*		tagListPtr;		/* NULL terminated tag list pointer */
  } loadTemplates;

  struct {						/* kSAudioCtlOpSetVol, kSAudioCtlOpGetVol */
    long		channelNumber;	/* which channel to control */
    long		value;
  } amplitude;

  struct {						/* kSAudioCtlOpSetPan, kSAudioCtlOpGetPan */
    long		channelNumber;	/* which channel to control */
    long		value;
  } pan;

  struct {						/* kSAudioCtlOpMuteSAudioChannel */
    long		channelNumber;	/* channel to make clock channel */
  } mute;

  struct {						/* kSAudioCtlOpUnMuteSAudioChannel */
    long		channelNumber;	/* channel to make clock channel */
  } unMute;

  struct {						/* kSAudioCtlOpSetClockChannel */
    long		channelNumber;	/* channel to make clock channel */
  } clock;

  struct {						/* kSAudioCtlOpCloseChannel */
    long		channelNumber;	/* channel to close */
  } CloseSAudioChannel;

  struct {						/* kSAudioCtlOpFlushChannel */
    long		channelNumber;	/* channel to flush */
  } FlushSAudioChannel;
			
} SAudioCtlBlock, *SAudioCtlBlockPtr;


#endif /* __SACONTROLMSGS_H__ */

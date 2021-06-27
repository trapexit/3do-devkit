
#pragma force_top_level
#pragma include_only_once


#ifndef __audio_h
#define __audio_h

/*
** Audio Folio Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/
/*
** Please consider the contents of all Audio Data structures as PRIVATE.
** Use GetAudioItemInfo and SetAudioItemInfo to change these structures.
** We reserve the right to change the definition of these structures.
*/

/*
** 930103 PLB Removed include of inthard.h
** 930208 PLB Added support for new Audio Timer, AudioCue
** 930210 PLB Added support for compressed samples
** 930527 PLB Changed PurgeInstruments to ScavengeInstrument
** 930604 PLB Add envelopes.
** 930614 PLB Moved structures to private file. Made tags enums.
*/

#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "item.h"
#include "operamath.h"
#include "device.h"
#include "driver.h"
#include "io.h"
#include "filesystem.h"
#include "stdio.h"

/* Since my format strings are similar but not identical, disable warnings
  on format strings */
// #pragma -v0

/**********************************************************************/
/**************************** Constants  ******************************/
/**********************************************************************/

/* This unique number matches number assigned by kernel. */
#define AUDIONODE   4
#define AUDIOFOLIONAME "audio"

/* AUDIODATADIR is very old and should no longer be used. */
// #define AUDIODATADIR "/remote"

/* OR these to select diagnostic messages with TraceAudio */
#define TRACE_TOP      (0x0001)
#define TRACE_INT      (0x0002)
#define TRACE_SAMPLE   (0x0004)
#define TRACE_ITEM     (0x0008)
#define TRACE_OFX      (0x0010)
#define TRACE_DSP      (0x0020)
#define TRACE_NOTE     (0x0040)
#define TRACE_HACK     (0x0080)
#define TRACE_KNOB     (0x0100)
#define TRACE_TIMER    (0x0200)
#define TRACE_ENVELOPE (0x0400)
#define TRACE_TUNING   (0x0800)
#define TRACE_ATTACHMENT (0x1000)

/*
** Item type numbers for Audio Folio
*/
#define AUDIO_TEMPLATE_NODE    (1)
#define AUDIO_INSTRUMENT_NODE  (2)
#define AUDIO_KNOB_NODE        (3)
#define AUDIO_SAMPLE_NODE      (4)
#define AUDIO_CUE_NODE         (5)
#define AUDIO_ENVELOPE_NODE    (6)
#define AUDIO_ATTACHMENT_NODE  (7)
#define AUDIO_TUNING_NODE      (8)

/* Special Cue Values for Attachments */
#define CUE_AT_LOOP    (-1)
#define CUE_AT_END     (-2)

/* Status levels for Instruments and Attachments */
#define AF_ABANDONED     (0)
#define AF_STOPPED       (1)
#define AF_RELEASED      (2)
#define AF_STARTED       (3)

/* Macro to make IFF chunk types */
#ifndef MAKE_ID
#define	MAKE_ID(a,b,c,d)	\
	((uint32) (a)<<24 | (uint32) (b)<<16 | (uint32) (c)<<8 | (uint32) (d))
#endif

/* Audio AIFC compression types. */
#define	ID_SDX2			MAKE_ID('S','D','X','2')
#define	ID_SDX3			MAKE_ID('S','D','X','3')
#define	ID_ADP4			MAKE_ID('A','D','P','4') /* Intel/DVI 4:1 ADPCM */

/*
** Define User Function offsets, must match array in audio_folio.c
*/
#define LOADINSTEMPLATE     (-1)
#define ALLOCINSTRUMENT     (-2)
#define LOADSAMPLE          (-3)
#define GRABKNOB            (-4)
#define SLEEPAUDIOTICKS     (-5)
#define DEBUGSAMPLE         (-6)
#define GETNUMKNOBS         (-7)
#define GETKNOBNAME         (-8)
#define GETAUDIOITEMINFO    (-9)
#define LOADINSTRUMENT     (-10)
#define UNLOADSAMPLE       (-11)
#define SCANSAMPLE         (-12)
#define DSPGETRSRCALLOC    (-13)
#define MAKESAMPLE         (-14)
#define GETAUDIORATE       (-15)
#define GETAUDIODURATION   (-16)
#define SLEEPUNTILTIME     (-17)
#define GETCUESIGNAL       (-18)
#define OWNAUDIOCLOCK      (-19)
#define DISOWNAUDIOCLOCK   (-20)
#define DSPGETINSRSRCUSED  (-21)
#define DSPGETTOTALRSRCUSED (-22)
#define UNLOADINSTEMPLATE  (-23)
#define UNLOADINSTRUMENT   (-24)
#define FINDAUDIODEVICE    (-25)
#define DEFINEINSTEMPLATE  (-26)
#define CONTROLAUDIODEVICE (-27)
#define GETNUMINSTRUMENTS  (-28)
#define GETMASTERTUNING    (-29)
#define CREATETUNING       (-30)
#define DELETETUNING       (-31)
#define ATTACHENVELOPE     (-32)
#define DETACHENVELOPE     (-33)
#define CREATEENVELOPE     (-34)
#define DELETEENVELOPE     (-35)
#define ATTACHSAMPLE       (-36)
#define DETACHSAMPLE       (-37)
#define LOADSAMPLEHERE     (-38)
#define CREATEDELAYLINE    (-39)
#define DELETEDELAYLINE    (-40)
#define DEFINESAMPLEHERE   (-41)
#define GETAUDIOTIME       (-42)
#define CONVERT12TET_F16   (-43)
#define CREATESAMPLE       (-44)
#define CREATEINSTEMPLATE  (-45)

/* Don't change this without also changing Assembler and OFX file generators. */
#define AF_MAX_NAME_SIZE 32

enum audio_folio_tags
{
	AF_TAG_AMPLITUDE = TAG_ITEM_LAST+1,   /* 10 */
	AF_TAG_RATE,
	AF_TAG_NAME,
	AF_TAG_DEVICE,
	AF_TAG_PITCH,
	AF_TAG_VELOCITY,
	AF_TAG_TEMPLATE,
	AF_TAG_INSTRUMENT,
	AF_TAG_FORMAT,
	AF_TAG_MIN,
	AF_TAG_MAX,           /* 20 */
	AF_TAG_DEFAULT,
	AF_TAG_WIDTH,
	AF_TAG_CHANNELS,
	AF_TAG_FRAMES,
	AF_TAG_BASENOTE,
	AF_TAG_DETUNE,
	AF_TAG_LOWNOTE,
	AF_TAG_HIGHNOTE,
	AF_TAG_LOWVELOCITY,
	AF_TAG_HIGHVELOCITY,  /* 30 */
	AF_TAG_SUSTAINBEGIN,
	AF_TAG_SUSTAINEND,
	AF_TAG_RELEASEBEGIN,
	AF_TAG_RELEASEEND,
	AF_TAG_NUMBYTES,
	AF_TAG_ADDRESS,
	AF_TAG_SAMPLE,
	AF_TAG_EXTERNAL,
	AF_TAG_PRIORITY,
	AF_TAG_SET_FLAGS,
	AF_TAG_CLEAR_FLAGS,
/* New Tags as of 3/15/93 */
	AF_TAG_FREQUENCY,
	AF_TAG_ENVELOPE,
	AF_TAG_HOOKNAME,
	AF_TAG_START_AT,
/* New Tags as of 5/4/93 */
	AF_TAG_SAMPLE_RATE,
/* New Tags af of V1.1.18, 5/10/93 */
	AF_TAG_COMPRESSIONRATIO,
	AF_TAG_COMPRESSIONTYPE,
	AF_TAG_NUMBITS,
/* New Tags af of V1.1.29, 6/7/93 */
	AF_TAG_NOTESPEROCTAVE,
	AF_TAG_BASEFREQ,
/* New Tags as of V1.1.33, 6/12/93 */
	AF_TAG_SUSTAINTIME,
	AF_TAG_RELEASETIME,
	AF_TAG_MICROSPERUNIT,
	AF_TAG_DATA_OFFSET,
	AF_TAG_DATA_SIZE,
/* New Tags as of V0.7.51, 7/21/93 */
	AF_TAG_DELAY_LINE,
/* New Tags as of V20.01.58, 8/9/93 */
	AF_TAG_RELEASEJUMP,
/* New Tags as of V20.16.62, 8/30/93 */
	AF_TAG_CURRENT,
/* New Tags as of V20.16.66, 10/11/93 */
	AF_TAG_STATUS,
/* New Tags as of V20.24.69, 11/19/93 */
	AF_TAG_TIME_SCALE,
	AF_TAG_START_TIME,
/* New Tags as of V20.25.71, 12/15/93 */
	AF_TAG_IMAGE_ADDRESS,
	AF_TAG_IMAGE_LENGTH,
	AF_TAG_LEAVE_IN_PLACE,
	AF_TAG_ALLOC_FUNCTION,
	AF_TAG_FREE_FUNCTION,
	AF_TAG_SCAN
};

#define	AF_INSF_AUTOABANDON   (0x0001)  /* Abandon when stopped. */
#define AF_INSF_LEGALFLAGS    (0x0001)

/* Envelope Flags */
#define AF_ENVF_FOLIO_OWNS     (0x0001)
#define AF_ENVF_FATLADYSINGS   (0x0002)
#define AF_ENVF_LEGALFLAGS     (0x0003)

/* Flags for Attachments */
#define AF_ATTF_NOAUTOSTART    (0x0001)
#define AF_ATTF_FATLADYSINGS   (0x0002)
#define AF_ATTF_LEGALFLAGS     (0x0003)

/* DSP Specific constants */
enum DSPPResourceTypes    /* !!!! These must match dspp_asm.fth */
{
	DRSC_N_MEM,
	DRSC_EI_MEM,
	DRSC_I_MEM,
	DRSC_EO_MEM,
	DRSC_RBASE4,
	DRSC_RBASE8,
	DRSC_IN_FIFO,
	DRSC_OUT_FIFO,
	DRSC_TICKS,
	DSPP_NUM_RSRC_TYPES    /* number of resources */
};


#define MAXDSPLOUDNESS (0x7FFF)  /* OBSOLETE! use MAXDSPAMPLITUDE */
#define MAXDSPAMPLITUDE (0x7FFF)

#define AF_A440_PITCH      (69)

/**********************************************************************/
/************************** Error Returns *****************************/
/**********************************************************************/

#define MAKEAERR(svr,class,err) MakeErr(ER_FOLI,ER_ADIO,svr,ER_E_SSTM,class,err)

#define AF_ERR_BADTAG     MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define AF_ERR_BADTAGVAL  MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define AF_ERR_BADPRIV    MAKEAERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define AF_ERR_BADSUBTYPE MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define AF_ERR_BADITEM    MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define AF_ERR_NOMEM      MAKEAERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define AF_ERR_BADPTR     MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadPtr)

#define AF_ERR_BASE (1)
#define AF_ERR_BADFILENAME  MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+0)
#define AF_ERR_NOFIFO     MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+1)
#define AF_ERR_NOSAMPLE   MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+2)

/* No knobs defined for this instrument. */
#define AF_ERR_NOKNOBS    MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+3)

/* Named thing not found. */
#define AF_ERR_BADNAME MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+4)

/* Item not attached to instrument. */
#define AF_ERR_NOINSTRUMENT MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+5)

/* Bad calculation type for converting knob value. Bad OFX file. */
#define AF_ERR_BADCALCTYPE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+6)

/* Knob connected to illegal resource. Bad OFX file. */
#define AF_ERR_BADKNOBRSRC MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+7)

#define AF_ERR_BADFILEIO   MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+8)

/* External reference in instrument not satisfied. */
#define AF_ERR_EXTERNALREF MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+9)

/* Illegal Audio Resource type. Bad OFX file. */
#define AF_ERR_BADRSRCTYPE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+10)

/* Relocation Resource type out of bounds. Bad OFX file. */
#define AF_ERR_BADRLOCINDX MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+11)

/* DSPP Code Relocation error. Bad OFX file. */
#define AF_ERR_RELOCATION MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+12)

/* Illegal Resource Attribute. OFX file error. */
#define AF_ERR_RSRCATTR	MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+13)

/* Item is still in use or attached. */
#define AF_ERR_INUSE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+14)

/* Already freed or detached. */
#define AF_ERR_DOUBLEFREE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+15)

/* Too many Markers in AIFF file. */
#define AF_ERR_OVERMARKERS MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+16)

/* General OFX File error. */
#define AF_ERR_BADOFX MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+17)

/* Could not allocate Audio Resource. */
#define AF_ERR_NORSRC MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+18)

/* Bad IFF File type */
#define AF_ERR_BADFILETYPE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+19)

/* File not open */
#define AF_ERR_FILENOTOPEN MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+20)

/* Could not allocate a signal */
#define AF_ERR_NOSIGNAL MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+21)

/* Value out of range. */
#define AF_ERR_OUTOFRANGE MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+22)

/* Unimplemented feature */
#define AF_ERR_UNIMPLEMENTED MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+23)

/* Data address NULL */
#define AF_ERR_NULLADDRESS MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+24)

/* DSPP Not Responding */
#define AF_ERR_TIMEOUT MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+25)

/* Illegal security violation. */
#define AF_ERR_SECURITY MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+26)

/* Caller does not own item. */
#define AF_ERR_NOTOWNER MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+27)

/* Thread did not call OpenAudioFolio(). */
#define AF_ERR_AUDIOCLOSED MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+28)

/**********************************************************************/
/************************** Data Structures  **************************/
/**********************************************************************/
/*
** Please consider the contents of all other Audio Data structures as PRIVATE.
** Use GetAudioItemInfo and SetAudioItemInfo to access these structures.
** We reserve the right to change the definition of these structures.
*/

typedef uint32 AudioTime;

typedef struct DataTimePair
{
	uint32     dtpr_Time; 
	int32      dtpr_Data;
} DataTimePair;

/**********************************************************************/
/************************** Macros  ***********************************/
/**********************************************************************/

#define CreateCue(Tags) CreateItem(MKNODEID(AUDIONODE,AUDIO_CUE_NODE),Tags)
#define DeleteCue(Cue) DeleteItem(Cue)

/**********************************************************************/
/************************** Prototypes ********************************/
/**********************************************************************/

#define AUDIOSWI 0x40000

/* Declare SWIs */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err __swi(AUDIOSWI+0) TweakKnob( Item KnobItem, int32 Value );
Err __swi(AUDIOSWI+1) StartInstrument( Item Instrument, TagArg *TagList);
Err __swi(AUDIOSWI+2) ReleaseInstrument( Item Instrument, TagArg *TagList);
Err __swi(AUDIOSWI+3) StopInstrument( Item Instrument, TagArg *TagList);
Err __swi(AUDIOSWI+4) TuneInsTemplate( Item InsTemplate, Item Tuning );
Err __swi(AUDIOSWI+5) TuneInstrument( Item Instrument, Item Tuning );
Err __swi(AUDIOSWI+6) ReservedAudioSWI1( void );
Err __swi(AUDIOSWI+7) TestHack( TagArg *args );
Err __swi(AUDIOSWI+8) ConnectInstruments( Item SrcIns, char *SrcName,
                                             Item DstIns, char *DstName);
uint32 __swi(AUDIOSWI+9) TraceAudio( int32 Mask );
int32 __swi(AUDIOSWI+10) AllocAmplitude( int32 Amplitude);
Err __swi(AUDIOSWI+11) FreeAmplitude( int32 Amplitude);
Err __swi(AUDIOSWI+12) DisconnectInstruments( Item SrcIns, char *SrcName,
                                                 Item DstIns, char *DstName);

Err __swi(AUDIOSWI+13) SignalAtTime( Item Cue, AudioTime Time );
/*  Err __swi(AUDIOSWI+14) PrivateInternal( Item AnItem ); timer support */
Err __swi(AUDIOSWI+15) SetAudioRate( Item Owner, frac16 Rate); 
Err __swi(AUDIOSWI+16) SetAudioDuration( Item Owner, uint32 Frames);
/****************************************************************/
/* New SWIs as of 3/15/93 */
Err __swi(AUDIOSWI+17) TweakRawKnob( Item KnobItem, int32 Value );
Err __swi(AUDIOSWI+18) StartAttachment( Item Attachment, TagArg *tp );
Err __swi(AUDIOSWI+19) ReleaseAttachment( Item Attachment, TagArg *tp );
Err __swi(AUDIOSWI+20) StopAttachment(  Item Attachment, TagArg *tp );
Err __swi(AUDIOSWI+21) LinkAttachments( Item At1, Item At2 );
Err __swi(AUDIOSWI+22) MonitorAttachment( Item Attachment, Item Cue, int32 CueAt );

Err __swi(AUDIOSWI+23) SetMasterTuning( frac16 Frequency );

/* Dynamic Voice Allocation support */
Err __swi(AUDIOSWI+24) AbandonInstrument( Item Instrument );
Item __swi(AUDIOSWI+25) AdoptInstrument( Item InsTemplate );
Item __swi(AUDIOSWI+26) ScavengeInstrument( Item InsTemplate, uint8 Priority,
	int32 MaxActivity, int32 IfSystemWide );

Err __swi(AUDIOSWI+27) SetAudioItemInfo( Item AnyItem, TagArg *tp );

/* Added 6/4/93 */
Err __swi(AUDIOSWI+28) PauseInstrument( Item Instrument );
Err __swi(AUDIOSWI+29) ResumeInstrument( Item Instrument );
int32 __swi(AUDIOSWI+30) WhereAttachment( Item Attachment );
uint32 __swi(AUDIOSWI+31) IncrementGlobalIndex( void );

/* Added 9/15/93  */
Err __swi(AUDIOSWI+32) BendInstrumentPitch( Item Instrument, frac16 BendFrac );

/* Added 12/21/93  */
Err __swi(AUDIOSWI+33) AbortTimerCue( Item Cue );

/****************************************************************/
Err OpenAudioFolio( void );
Err CloseAudioFolio( void );

/****************************************************************/
/******* Instruments ********************************************/
/****************************************************************/

Err UnloadInstrument( Item Instrument );
Err UnloadInsTemplate( Item InsTemplate );
Err FreeInstrument( Item Instrument );

/****************************************************************/
/******* Knobs **************************************************/
/****************************************************************/
Item  GrabKnob ( Item Instrument, char *Name );
Err  ReleaseKnob ( Item KnobItem );
char *GetKnobName ( Item Instrument, int32 KnobNumber);
int32 GetNumKnobs ( Item Instrument );

/****************************************************************/
/******* Samples ************************************************/
/****************************************************************/
Item   LoadSample ( char *Name );
Item   LoadSampleHere ( char *Name, void *(*CustomAllocMem)(), void (*CustomFreeMem)());
Err  DebugSample ( Item SampleItem );
Item   MakeSample ( uint32 NumBytes ,  TagArg *TagList );
Item   ScanSample ( char *Name, int32 BufferSize );
Err  UnloadSample ( Item SampleItem );

/****************************************************************/
/******* Audio Timer Functions **********************************/
/****************************************************************/
int32 GetCueSignal( Item Cue );
AudioTime GetAudioTime( void );
Err  SleepUntilTime(Item Cue, AudioTime Time);
Item  OwnAudioClock( void );
Err   DisownAudioClock( Item Owner );
frac16 GetAudioRate( void );
uint32 GetAudioDuration( void );

/* Obsolete */
Err SleepAudioTicks( int32 Ticks );

/* Modified user routines as of 3/15/93 ********************************/
Item  LoadInstrument( char *Name, Item AudioDevice, uint8 Priority);
Item  LoadInsTemplate( char *Name, Item AudioDevice);
Item  AllocInstrument( Item InsTemplate, uint8 Priority);

Item AttachSample( Item Instrument, Item Sample, char *FIFOName );
Err DetachSample( Item Attachment );

/* New user routines as of 3/15/93 *************************************/
Item  FindAudioDevice( TagArg *tp );
Item  DefineInsTemplate( uint8 *Definition, int32 NumBytes, Item Device, char *Name );
Err ControlAudioDevice( Item Device, void *SendBuffer, int32 SendLen,
	void *RecvBuffer, int32 RecvLen, TagArg *tp );
	
int32 GetNumInstruments( Item InsTemplate );

frac16 GetMasterTuning( void );
Item  CreateTuning( ufrac16 *Frequencies, int32 NumNotes, int32 NotesPerOctave, int32 BaseNote );
Err DeleteTuning( Item Tuning );

Err GetAudioItemInfo( Item AnyItem, TagArg *tp );


/* Envelopes */
Item  AttachEnvelope( Item Instrument, Item Envelope, char *EnvName );
Err DetachEnvelope( Item Attachment );
Item  CreateEnvelope( DataTimePair *Points, int32 Numpoints,
	int32 SustainBegin, int32 SustainEnd );
Err DeleteEnvelope( Item Envelope );


/* New user routines as of 7/20/93 *************************************/
Item   CreateDelayLine ( int32 NumBytes , int32 NumChannels, int32 IfLoop);
Err  DeleteDelayLine( Item DelayLIne );
Item   DefineSampleHere ( uint8 *AIFFImage, int32 NumBytes, void *(*CustomAllocMem)(), void (*CustomFreeMem)());

/* New user routines as of 11/29/93 *************************************/
Err Convert12TET_F16( int32 Semitones, int32 Cents, frac16 *FractionPtr );

/* New user routines as of 12/15/93 *************************************/
Item CreateSample ( TagArg *Tags);
Item CreateInsTemplate ( TagArg *Tags);

/******************************************************************/
/* Debug/Hack Routines - may not exist in final version. **********/
int32 DSPGetTicks( void );
int32 DSPGetMaxTicks( void );
int32 DSPGetRsrcAlloc (Item Instrument, int32 RsrcType, char *Name, int32 *Alloc);
void *DSPWhereDMA ( uint32 DMAChannel );
int32 DSPReadEO(int32 EO_Offset);
int32 DSPGetInsRsrcUsed( Item Instrument, int32 RsrcType );
int32 DSPGetTotalRsrcUsed( int32 RsrcType );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


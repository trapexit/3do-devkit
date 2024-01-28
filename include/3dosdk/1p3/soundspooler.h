#pragma include_only_once
#ifndef _soundspooler_h
#define _soundspooler_h


/***************************************************************
**
** SoundSooler Includes.
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************
** 940421 PLB Add ssplReset()
***************************************************************/

#include "types.h"
#include "list.h"

#define SBN_FLAG_INLIST (0x0001)

/* These structures should NOT be written directly by an application. */
typedef struct SoundBufferNode
{
	Node	sbn_Node;
	Item    sbn_Sample;        /* Sample item used to reference data. */
	Item    sbn_Attachment;
	Item    sbn_Cue;           /* Used for signaling completion. */
	int32   sbn_Signal;        /* Signal from Cue */
	uint32	sbn_Flags;
	char   *sbn_Address;       /* Address of sound data. */
	int32   sbn_NumBytes;      /* Number of bytes of sound data. */
	int32	sbn_SequenceNum;   /* For tracking buffers. */
	void   *sbn_UserData;
} SoundBufferNode;

#define SSPL_FLAG_STARTED (0x0001)

typedef struct SoundSpooler
{
	List	sspl_FreeBuffers;   /* List of buffers available for use. */
	List	sspl_ActiveBuffers; /* List of buffers queued up in audiofolio. */
	int32   sspl_NumBuffers;
	int32   sspl_SignalMask;    /* OR of all Cue signals. */
	Item    sspl_SamplerIns;    /* Appropriate sample player instrument */
	uint32  sspl_Flags;
} SoundSpooler;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err ssplAbort( SoundSpooler *sspl, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
Err ssplAttachInstrument( SoundSpooler *sspl, Item SamplerIns );
Err ssplDeleteSoundBufferNode(  SoundSpooler *sspl, SoundBufferNode *sbn );
Err ssplDeleteSoundSpooler( SoundSpooler *sspl );
Err ssplDetachInstrument( SoundSpooler *sspl );
Err ssplPause( SoundSpooler *sspl );
Err ssplReset( SoundSpooler *sspl, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
Err ssplResume( SoundSpooler *sspl );
Err ssplSetBufferAddressLength( SoundSpooler *sspl, SoundBufferNode *sbn, char *Data, int32 NumBytes );
Err ssplStartSpooler( SoundSpooler *sspl, int32 Amplitude);
Err ssplStopSpooler( SoundSpooler *sspl );
SoundBufferNode *ssplCreateSoundBufferNode( SoundSpooler *sspl );
SoundBufferNode *ssplRequestBuffer( SoundSpooler *sspl );
SoundSpooler *ssplCreateSoundSpooler( int32 NumBuffers, Item SamplerIns );
int32 ssplGetSequenceNum( SoundSpooler *sspl, SoundBufferNode *sbn );
int32 ssplPlayData( SoundSpooler *sspl, char *Data, int32 NumBytes );
int32 ssplProcessSignals( SoundSpooler *sspl, int32 SignalMask, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
int32 ssplSendBuffer( SoundSpooler *sspl, SoundBufferNode *sbn );
int32 ssplSpoolData( SoundSpooler *sspl, char *Data, int32 NumBytes, void *UserData );
void *ssplGetUserData( SoundSpooler *sspl, SoundBufferNode *sbn );
void ssplSetSequenceNum( SoundSpooler *sspl, SoundBufferNode *sbn, int32 sequenceNum );
void ssplSetUserData( SoundSpooler *sspl, SoundBufferNode *sbn, void *UserData );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

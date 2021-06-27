#ifndef __SOUNDSPOOLER_H
#define __SOUNDSPOOLER_H

#pragma force_top_level
#pragma include_only_once


/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: soundspooler.h,v 1.18 1994/10/05 21:08:32 peabody Exp $
**
**  Sound spooler
**
****************************************************************************/


#include "types.h"
#include "list.h"


/* -------------------- Structures */
/* These structures should NOT be written directly by an application. */

    /* forward typedefs */
typedef struct SoundBufferNode SoundBufferNode;
typedef struct SoundSpooler SoundSpooler;

    /* SoundBufferFunc callback function */
typedef int32 (*SoundBufferFunc)(SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg);


    /* SoundBufferNode */
struct SoundBufferNode
{
	Node	sbn_Node;
    Item    sbn_Sample;         /* Sample item used to reference data. */
    Item    sbn_Attachment;
    Item    sbn_Cue;            /* Used for signaling completion. */
    int32   sbn_Signal;         /* Signal from Cue */
    uint32  sbn_Flags;          /* SBN_FLAG_ below */
    char   *sbn_Address;        /* Address of sound data. */
    int32   sbn_NumBytes;       /* Number of bytes of sound data. */
    int32   sbn_SequenceNum;    /* For tracking buffers. */
	void   *sbn_UserData;
};

    /* sbn_Flags */
#define SBN_FLAG_INLIST (0x0001)


    /* SoundSpooler */
struct SoundSpooler
{
	List	sspl_FreeBuffers;   /* List of SoundBufferNodes available for use. */
	List	sspl_ActiveBuffers; /* List of SoundBufferNodes queued up in audiofolio. */
	int32   sspl_NumBuffers;
	int32   sspl_SignalMask;    /* OR of all Cue signals. */
	Item    sspl_SamplerIns;    /* Appropriate sample player instrument */
	uint32  sspl_Flags;         /* Private flags field. */
                                /* Optional callback function called for SoundBufferNode state changes */
	SoundBufferFunc sspl_SoundBufferFunc;
	List    sspl_RequestedBuffers; /* List of SoundBufferNodes that have been returned by ssplRequestBuffer() but not resubmitted or unrequested. */
};


/* -------------------- SoundBufferFunc message ids */

/* Message classes */

#define SSPL_SBMSGCLASS_MASK        0xff00      /* mask used to isolate class bits (see ssplGetSBMsgClass()) */
#define SSPL_SBMSGCLASS_START       0x0000
#define SSPL_SBMSGCLASS_END         0x0100


/* Start class messages */

    /* Initial Start - ssplStartSpooler() sends this message for the
       first buffer in active queue. */
#define SSPL_SBMSG_INITIAL_START    (SSPL_SBMSGCLASS_START | 0)

    /* Link Start - ssplProcessSignals() sends this message for the next
       buffer in the active queue after it removes a completed buffer. */
#define SSPL_SBMSG_LINK_START       (SSPL_SBMSGCLASS_START | 1)

    /* Starvation Start - ssplSendBuffer() sends this message for the
       buffer sent to it if that buffer causes the spooler to restart
       after being starved. */
#define SSPL_SBMSG_STARVATION_START (SSPL_SBMSGCLASS_START | 2)


/* Stop class messages */

    /* Complete - ssplProcessSignals() sends this message for every
       completed buffer that it removes from the active queue. */
#define SSPL_SBMSG_COMPLETE         (SSPL_SBMSGCLASS_END | 0)

    /* Abort - ssplReset() sends this message for every buffer (completed
       or otherwise) that it removes from the active queue. */
#define SSPL_SBMSG_ABORT            (SSPL_SBMSGCLASS_END | 1)


/* -------------------- Status flags returned by ssplGetSpoolerStatus() */
/*
    Note: These are NOT the private flags stored in sspl_Flags. They
    are returned by ssplGetSpoolerStatus() only.
*/

#define SSPL_STATUS_F_ACTIVE        0x01    /* Indicates that the spooler's
                                               active queue is not empty (there
                                               is a buffer in the active queue).
                                               Set by ssplSendBuffer(). Can be
                                               cleared by ssplProcessSignals()
                                               when all of the buffers in the
                                               active queue have completed.
                                               Always cleared by ssplReset() and
                                               ssplAbort(). */

#define SSPL_STATUS_F_STARTED       0x02    /* Indicates that spooler
                                               instrument has been started.
                                               Unless it has been paused, it is
                                               playing sound or will as soon as
                                               there is some to play. Set by
                                               ssplStartSpooler(). Cleared by
                                               ssplStopSpooler() (and
                                               ssplAbort()). */

#define SSPL_STATUS_F_PAUSED        0x04    /* Indicates that player is paused.
                                               Only meaningful if
                                               SSPL_STATUS_F_STARTED is also
                                               set. Set by ssplPause(). Cleared
                                               by ssplResume(),
                                               ssplStartSpooler(), and
                                               ssplStopSpooler(). */

/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* macros */
#define ssplGetSBMsgClass(msg) ((msg) & SSPL_SBMSGCLASS_MASK)
#define ssplIsSpoolerActive(spooler) (bool)((ssplGetSpoolerStatus(spooler) & SSPL_STATUS_F_ACTIVE) != 0)

    /* functions */
Err ssplAbort( SoundSpooler *sspl, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
Err ssplAttachInstrument( SoundSpooler *sspl, Item SamplerIns );
SoundSpooler *ssplCreateSoundSpooler( int32 NumBuffers, Item SamplerIns );
Err ssplDeleteSoundSpooler( SoundSpooler *sspl );
Err ssplDetachInstrument( SoundSpooler *sspl );
void ssplDumpSoundBufferNode( const SoundBufferNode *sbn );
void ssplDumpSoundSpooler( const SoundSpooler *sspl );
int32 ssplGetSequenceNum( SoundSpooler *sspl, SoundBufferNode *sbn );
int32 ssplGetSpoolerStatus (const SoundSpooler *);
void *ssplGetUserData( SoundSpooler *sspl, SoundBufferNode *sbn );
Err ssplPause( SoundSpooler *sspl );
int32 ssplPlayData( SoundSpooler *sspl, char *Data, int32 NumBytes );
int32 ssplProcessSignals( SoundSpooler *sspl, int32 SignalMask, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
SoundBufferNode *ssplRequestBuffer( SoundSpooler *sspl );
Err ssplReset( SoundSpooler *sspl, void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) );
Err ssplResume( SoundSpooler *sspl );
int32 ssplSendBuffer( SoundSpooler *sspl, SoundBufferNode *sbn );
Err ssplSetBufferAddressLength( SoundSpooler *sspl, SoundBufferNode *sbn, char *Data, int32 NumBytes );
Err ssplSetSoundBufferFunc( SoundSpooler *, SoundBufferFunc );
void ssplSetUserData( SoundSpooler *sspl, SoundBufferNode *sbn, void *UserData );
int32 ssplSpoolData( SoundSpooler *sspl, char *Data, int32 NumBytes, void *UserData );
Err ssplStartSpooler( SoundSpooler *sspl, int32 Amplitude);
Err ssplStartSpoolerTags (SoundSpooler *sspl, const TagArg *samplertags);
Err ssplStartSpoolerTagsVA (SoundSpooler *sspl, uint32 tag1, ...);
Err ssplStopSpooler( SoundSpooler *sspl );
Err ssplUnrequestBuffer( SoundSpooler *sspl, SoundBufferNode *sbn );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __SOUNDSPOOLER_H */

#pragma include_only_once
#ifndef _soundfile_h
#define _soundfile_h

/***************************************************************
**
** Includes for Playing a Sound File from Disk.
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include "types.h"

#define MAX_SOUNDFILE_BUFS (8)

typedef struct
{
	Item    sfb_Sample;
	Item    sfb_Attachment;
	Item    sfb_Cue;
	int32   sfb_Signal;
	char   *sfb_Address;
} SoundFileBuffer;

typedef struct
{
	SoundFileBuffer sfp_Buffers[MAX_SOUNDFILE_BUFS];
	int32   sfp_NumBuffers;
	int32   sfp_SignalMask;
	Item    sfp_FileItem;
	Item	sfp_IOReqItem;      /* For block reads. */
	Item    sfp_OutputIns;      /* Instrument used to output sound. */
	Item    sfp_SamplerIns;     /* Appropriate sample player instrument */
	uint32  sfp_Cursor;         /* Position of file reader. */
	uint32  sfp_BufIndex;       /* Next buffer to be loaded. */
	uint32  sfp_BufSize;        /* Size of the buffer in bytes. */
	int32   sfp_LastPos;        /* Byte position of DMA, updated periodically */
	uint32  sfp_NumToRead;      /* Number of bytes to read, total. */
	uint32  sfp_DataOffset;     /* Start of Data in file. */
	uint32  sfp_Flags;
	uint32  sfp_BuffersPlayed;  /* How many played so far. */
	uint32  sfp_BuffersToPlay;  /* How many total need to be played. */
} SoundFilePlayer;

/* Flags used by SoundFile Player */
#define SFP_OWN_BUFFERS  (0x00001)    /* True if the Buffer memory is owned by this system. */
#define SFP_NO_DIRECTOUT (0x00002)    /* If true, no automatic connection to directout.dsp */
#define SFP_IO_PENDING   (0x00004)    /* Set if an IO request has been sent and not received. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

SoundFilePlayer *CreateSoundFilePlayer ( int32 NumBuffers, int32 BufSize, void *Buffers[] );
SoundFilePlayer *OpenSoundFile (char *FileName, int32 NumBuffers, int32 BufSize );
int32 BumpSoundFile ( SoundFilePlayer *sfp );
int32 CloseSoundFile ( SoundFilePlayer *sfp );
int32 DeleteSoundFilePlayer ( SoundFilePlayer *sfp );
int32 LoadSoundFile (  SoundFilePlayer *sfp, char *FileName );
int32 ReadSoundFile( SoundFilePlayer *sfp, int32 Cursor, int32 BufIndex);
int32 RewindSoundFile ( SoundFilePlayer *sfp);
int32 ServiceSoundFile ( SoundFilePlayer *sfp, int32 SignalIn, int32 *SignalNeeded);
int32 StartSoundFile ( SoundFilePlayer *sfp , int32 Amplitude);
int32 StopSoundFile ( SoundFilePlayer *sfp );
int32 UnloadSoundFile ( SoundFilePlayer *sfp );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

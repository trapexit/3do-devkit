/* $Id: soundfile.h,v 1.15 1994/04/26 23:01:27 peabody Exp $ */
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
****************************************************************
** 940224 PLB Use new SoundSpooler
** 940426 WJB Removed BumpSoundFile() prototype.
**            Added inclusion of soundspooler.h.
***************************************************************/

#include "soundspooler.h"
#include "types.h"

#define MAX_SOUNDFILE_BUFS (8)

typedef struct
{
	SoundSpooler *sfp_Spooler;
	char   *sfp_BufferAddrs[MAX_SOUNDFILE_BUFS];
	Item    sfp_FileItem;
	Item	sfp_IOReqItem;      /* For block reads. */
	Item    sfp_SamplerIns;     /* Instrument used to play samples. */
	Item    sfp_OutputIns;      /* Instrument used to output sound. */
	uint32  sfp_Cursor;         /* Position of file reader. */
	uint32  sfp_NumBuffers;     /* Number of Buffers in use. */
	uint32  sfp_BufIndex;       /* Next buffer to be loaded. */
	uint32  sfp_BufSize;        /* Size of the buffer in bytes. */
	int32   sfp_LastPos;        /* Byte position of DMA, updated periodically */
	uint32  sfp_NumToRead;      /* Number of bytes to read, total. */
	uint32  sfp_DataOffset;     /* Start of Data in file. */
	uint32  sfp_BuffersPlayed;  /* How many played so far. */
	uint32  sfp_BuffersToPlay;  /* How many total need to be played. */
	uint8	sfp_Flags;          /* User settable flags. */
	uint8	sfp_PrivateFlags;   /* Private flags. Don't Touch! */
} SoundFilePlayer;


/* Flags used by SoundFile Player */
#define SFP_NO_SAMPLER   (0x01)    /* If true, does not load instrument to play sample. */
#define SFP_NO_DIRECTOUT (0x02)    /* If true, no automatic connection to directout.dsp */
#define SFP_LOOP_FILE    (0x04)    /* If true, loop entire file. */

#define SFP_PRIV_OWN_BUFFERS  (0x01)    /* True if the Buffer memory is owned by this system. */
#define SFP_PRIV_IO_PENDING   (0x02)    /* Set if an IO request has been sent and not received. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

SoundFilePlayer *CreateSoundFilePlayer ( int32 NumBuffers, int32 BufSize, void *Buffers[] );
SoundFilePlayer *OpenSoundFile (char *FileName, int32 NumBuffers, int32 BufSize );
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

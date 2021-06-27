#ifndef __SOUNDPLAYER_H
#define __SOUNDPLAYER_H

#pragma force_top_level
#pragma include_only_once


/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: soundplayer.h,v 1.38 1994/10/13 17:44:46 peabody Exp $
**
**  Advanced looping/branching sound player
**
**  By: Bill Barton
**
****************************************************************************/


#ifndef __TYPES_H
  #include "types.h"
#endif


/* -------------------- Typedefs */

    /*
        typedefs for handles to player data structures.
        The internal workings of these are private.
    */
typedef struct SPPlayer SPPlayer;
typedef struct SPSound SPSound;
typedef struct SPMarker SPMarker;
typedef struct SPAction SPAction;

    /* Decision function typedef */
typedef Err (*SPDecisionFunction) (SPAction *resultAction, void *decisionData, SPSound *sound, const char *markername);


/* -------------------- Special marker names */

    /*
        Special marker names.
        Use these constants to refer to these special markers.
    */
#define SP_MARKER_NAME_BEGIN         sp_markerNameBegin
#define SP_MARKER_NAME_END           sp_markerNameEnd
#define SP_MARKER_NAME_SUSTAIN_BEGIN sp_markerNameSustainBegin
#define SP_MARKER_NAME_SUSTAIN_END   sp_markerNameSustainEnd
#define SP_MARKER_NAME_RELEASE_BEGIN sp_markerNameReleaseBegin
#define SP_MARKER_NAME_RELEASE_END   sp_markerNameReleaseEnd

    /*
        Internal names stored in library to avoid string replication.
        Use the #defines above instead of these names directly.
    */
extern const char sp_markerNameBegin[];
extern const char sp_markerNameEnd[];
extern const char sp_markerNameSustainBegin[];
extern const char sp_markerNameSustainEnd[];
extern const char sp_markerNameReleaseBegin[];
extern const char sp_markerNameReleaseEnd[];


/* -------------------- Status flags returned by spGetPlayerStatus() */

#define SP_STATUS_F_BUFFER_ACTIVE   0x01    /* Indicates that there's something
                                               in the sound buffers waiting to
                                               be played. Set or cleared by
                                               spStartReading(), spService().
                                               Cleared by spStop(). This going
                                               false after starting is the best
                                               indicator of when playback is
                                               complete. */

#define SP_STATUS_F_READING         0x02    /* Indicates there's more data to
                                               read. Set by spStartReading().
                                               Cleared when no more to read (by
                                               spStartReading() or spService())
                                               or manually by spStop(). */

#define SP_STATUS_F_PLAYING         0x04    /* Indicates that playback has
                                               been started (but might be
                                               paused). Set by
                                               spStartPlaying(). Cleared by
                                               spStop(). */

#define SP_STATUS_F_PAUSED          0x08    /* Indicates that player has been
                                               paused. Set by spPause(). Cleared
                                               by spResume(), spStop(),
                                               spStartPlaying(). This flag is
                                               only meaningful when
                                               SP_STATUS_F_PLAYING is set. */


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* SPPlayer create, deletion, set attributes */
Err spCreatePlayer (SPPlayer **resultPlayer, Item samplerIns, uint32 numBuffers, uint32 bufSize, void * const buffers[]);
Err spDeletePlayer (SPPlayer *);
Err spSetDefaultDecisionFunction (SPPlayer *, SPDecisionFunction, void *decisionData);
#define spClearDefaultDecisionFunction(player) spSetDefaultDecisionFunction ((player), NULL, NULL)
int32 spGetPlayerSignalMask (const SPPlayer *);

    /* SPPlayer control */
Err spStartReading (SPSound *startSound, const char *startMarkerName);
Err spStartPlaying (SPPlayer *, const TagArg *samplerTags);
Err spStartPlayingVA (SPPlayer *, uint32 samplerTag1, ...);
Err spStop (SPPlayer *);
Err spPause (SPPlayer *);
Err spResume (SPPlayer *);
int32 spService (SPPlayer *, int32 signals);
int32 spGetPlayerStatus (const SPPlayer *);

    /* SPSound add, remove, status */
Err spAddSample (SPSound **resultSound, SPPlayer *, Item sample);
Err spAddSoundFile (SPSound **resultSound, SPPlayer *, const char *fileName);
Err spRemoveSound (SPSound *);
SPPlayer *spGetPlayerFromSound (const SPSound *);
bool spIsSoundInUse (const SPSound *);

    /* SPMarker add, remove, set attributes */
Err spAddMarker (SPSound *, uint32 position, const char *markerName);
Err spRemoveMarker (SPMarker *);
SPMarker *spFindMarkerName (const SPSound *, const char *markerName);
Err spSetMarkerDecisionFunction (SPSound *, const char *markerName, SPDecisionFunction, void *decisionData);
#define spClearMarkerDecisionFunction(sound,markerName) spSetMarkerDecisionFunction ((sound), (markerName), NULL, NULL)
SPSound *spGetSoundFromMarker (const SPMarker *);
uint32 spGetMarkerPosition (const SPMarker *);
const char *spGetMarkerName (const SPMarker *);

    /* SPMarker action control */
Err spContinueAtMarker (SPSound *, const char *markerName);
Err spStopAtMarker (SPSound *, const char *markerName);
Err spBranchAtMarker (SPSound *fromSound, const char *fromMarkerName, SPSound *toSound, const char *toMarkerName);
#define spLinkSounds(fromSound,toSound) spBranchAtMarker ((fromSound), SP_MARKER_NAME_END, (toSound), SP_MARKER_NAME_BEGIN)
#define spLoopSound(sound) spLinkSounds((sound),(sound))

    /* SPAction functions */
Err spSetBranchAction (SPAction *, SPSound *toSound, const char *toMarkerName);
Err spSetStopAction (SPAction *);

    /* Debug (careful not to include these in a released application) */
void spDumpPlayer (const SPPlayer *);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __SOUNDPLAYER_H */

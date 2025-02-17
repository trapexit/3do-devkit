#pragma include_only_once
#ifndef __score_h
#define __score_h

/*
** Score Player Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "debug.h"
#include "folio.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "semaphore.h"
#include "stdarg.h"
#include "string.h"
#include "task.h"
#include "types.h"

#define MAXSCOREVOICES (16)

typedef struct PIMapEntry
{
  Item pimp_InsTemplate;
  uint8 pimp_Priority;
  uint8 pimp_MaxVoices;
  uint8 pimp_Reserved2;
  uint8 pimp_Reserved3;
} PIMapEntry;

#define NTTR_FLAG_INUSE (0x01) /* In channel list */
#define NTTR_FLAG_NOTEON (0x02)
#define NTTR_FLAG_FREE (0x04) /* In free list */

typedef struct NoteTracker
{
  Node nttr_Node;
  int8 nttr_Note;
  int8 nttr_MixerChannel;
  uint8 nttr_Flags;
  int8 nttr_Channel; /* MIDI */
  Item nttr_Instrument;
} NoteTracker;

/* One for each of the 16 MIDI Channels. */
typedef struct ScoreChannel
{
  int8 schn_DefaultProgram; /* MIDI Programs */
  int8 schn_CurrentProgram;
  uint8 schn_Priority;
  uint8 schn_NumVoices;
  int32 schn_Volume;      /* 14 bit volume set by control 7 */
  int32 schn_Pan;         /* 14 bit pan set by control 10 */
  int32 schn_LeftVolume;  /* calculated from Volume, Pan, Max */
  int32 schn_RightVolume; /* calculated from Volume, Pan, Max */
  List schn_NoteList;
  frac16 schn_PitchBend; /* in MIDI units, 0x2000 is no bend. */
} ScoreChannel;

#define SCON_FLAG_VERBOSE (0x01)    /* If set, warn when no program defined. */
#define SCON_FLAG_USE_INSPRI (0x02) /* Use priority from PIMap. */
#define SCON_FLAG_NO_TEMPLATE_OK                                              \
  (0x04) /* Continue execution if no template match. */

/* Global context for score to be played in. */
typedef struct ScoreContext
{
  uint8 scon_PIMapSize; /* Number of Entries in PIMap */
  uint8 scon_MaxVoices;
  uint8 scon_Flags;
  uint8 scon_Reserved3;
  PIMapEntry *scon_PIMap;
  int32 scon_MaxVolume; /* Per Voice */
  Item scon_MixerIns;
  Item scon_LeftGains[MAXSCOREVOICES]; /* Gain Knobs */
  Item scon_RightGains[MAXSCOREVOICES];
  NoteTracker *scon_NoteTrackers;
  List scon_FreeNoteTrackers;
  ScoreChannel scon_Channels[NUMMIDICHANNELS];
  int32 scon_BendRange; /* in semitones, used to interpret MIDI bend */
  int32 (*scon_PurgeHook) (uint8 Priority,
                           int32 MaxActivity); /* Called when desparate. */
} ScoreContext;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  ScoreContext *CreateScoreContext (int32 MaxNumPrograms);
  char *SelectSamplePlayer (Item Sample, int32 IfVariable);
  Err ChangeScoreControl (ScoreContext *scon, int32 Channel, int32 Index,
                          int32 Value);
  Err ChangeScoreProgram (ScoreContext *ScoreCon, int32 Channel,
                          int32 ProgramNum);
  Err ChangeScorePitchBend (ScoreContext *scon, int32 Channel, int32 Bend);
  Err ConvertPitchBend (int32 Bend, int32 SemitoneRange,
                        frac16 *BendFractionPtr);
  Err DeleteScoreContext (ScoreContext *scon);
  Err DisableScoreMessages (int32 Flag);
  Err InitScoreContext (ScoreContext *ScoreCon);
  Err InitScoreDynamics (ScoreContext *scon, int32 MaxScoreVoices);
  Err InitScoreMixer (ScoreContext *scon, char *MixerName, int32 MaxNumVoices,
                      int32 Amplitude);
  Err InterpretMIDIEvent (Sequence *SeqPtr, MIDIEvent *MEvCur,
                          ScoreContext *scon);
  Err InterpretMIDIMessage (ScoreContext *ScoreCon, char *MIDIMsg,
                            int32 IfMute);
  Err LoadPIMap (ScoreContext *scon, char *FileName);
  Err MFDefineCollection (MIDIFileParser *mfpptr, char *Image, int32 NumBytes,
                          Collection *ColPtr);
  Err MFLoadCollection (MIDIFileParser *mfpptr, char *filename,
                        Collection *ColPtr);
  Err MFLoadSequence (MIDIFileParser *mfpptr, char *filename,
                      Sequence *SeqPtr);
  Err MFUnloadCollection (Collection *ColPtr);
  Err NoteOffIns (Item Instrument, int32 Note, int32 Velocity);
  Err NoteOnIns (Item Instrument, int32 Note, int32 Velocity);
  Err ReleaseScoreNote (ScoreContext *scon, int32 Channel, int32 Note,
                        int32 Velocity);
  Err SetPIMapEntry (ScoreContext *scon, int32 ProgramNum, Item InsTemplate,
                     int32 MaxVoices, int32 Priority);
  Err StartScoreNote (ScoreContext *scon, int32 Channel, int32 Note,
                      int32 Velocity);
  Err TermScoreContext (ScoreContext *ScoreCon);
  Err TermScoreMixer (ScoreContext *ScoreCon);
  Err UnloadPIMap (ScoreContext *scon);
  Err FreeChannelInstruments (ScoreContext *scon, int32 Channel);

  int32 PurgeScoreInstrument (ScoreContext *scon, uint8 Priority,
                              int32 MaxLevel);

  Err SetScoreBendRange (ScoreContext *scon, int32 BendRange);
  int32 GetScoreBendRange (ScoreContext *scon);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tsp_switcher.c,v 1.30 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/tsp_switcher
|||	tsp_switcher - Advanced sound player example that switches between
sounds
|||	    based on control pad input.
|||
|||	  Synopsis
|||
|||	    tsp_switcher
|||
|||	  Description
|||
|||	    Loops one of three sound files off of disc. The user can select a
|||	    different sound to loop by pressing the A, B, or C buttons on the
control
|||	    pad. The last button pressed corresponds to the sound being played.
|||
|||	    This program demonstrates how one might use the advanced sound
player as
|||	    an engine for doing environmentally-sensitive soundtrack playback.
Note
|||	    that the sound being played doesn't change to the newly selected
one until
|||	    the end of the current sound is reached. This demonstrates, for
example
|||	    how a score might be made to change at the next musically sensible
point.
|||	    See tsp_rooms for another way to do this.
|||
|||	    Control pad operation:
|||
|||	    * ASelect sound #1.
|||
|||	    * BSelect sound #2.
|||
|||	    * CSelect sound #3.
|||
|||	    * XStop, quit when done playing.
|||
|||	    * Shift-XQuit immediately.
|||
|||	    * StartToggle pause on/off.
|||
|||	  Associated Files
|||
|||	    tsp_switcher.c,
|||
|||	    $samples
|||
|||	  Location
|||
|||	    examples/Audio/Advanced_Sound_Player
|||
|||	  See Also
|||
|||	    spCreatePlayer(), tsp_rooms, tsp_algorithmic
|||
**/

#include <audio.h>
#include <event.h>
#include <mem.h>
#include <soundplayer.h>
#include <stdio.h>

/* -------------------- Parameters */

/* sound files */
#define SOUND_1 "$samples/PitchedL/PianoGrandFat/GrandPianoFat.C1M44k.aiff"
#define SOUND_2 "$samples/GMPercussion44k/ChineseCymLite.M44k.aiff"
#define SOUND_3 "$samples/GMPercussion44k/CuicaOpen.M44k.aiff"

/* audio buffers */
#define NUMBUFS 4
#define BUFSIZE 22000

/* -------------------- Macros */

#define TestLeadingEdge(newset, oldset, mask) ((newset) & ~(oldset) & (mask))
#define TestTrailingEdge(newset, oldset, mask)                                \
  TestLeadingEdge ((oldset), (newset), (mask))

/*
    Macro to simplify error checking.
    expression is only evaluated once and should return an int32
    (or something that can be cast to it sensibly)
*/
#define TRAP_ERROR(_exp, _desc)                                               \
  do                                                                          \
    {                                                                         \
      const int32 _result = (_exp);                                           \
      if (_result < 0)                                                        \
        {                                                                     \
          PrintError (NULL, _desc, NULL, _result);                            \
          goto clean;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

/* -------------------- Code */

void testplayer (void);
Err endofsound_decision (SPAction *resultAction, SPSound *const *nextSoundP,
                         SPSound *sound, const char *markerName);

int
main (int argc, char *argv[])
{
#ifdef MEMDEBUG
  TRAP_ERROR (CreateMemDebug (
                  MEMDEBUGF_ALLOC_PATTERNS | MEMDEBUGF_FREE_PATTERNS
                      | MEMDEBUGF_PAD_COOKIES | MEMDEBUGF_CHECK_ALLOC_FAILURES
                      | MEMDEBUGF_KEEP_TASK_DATA,
                  NULL),
              "CreateMemDebug");
#endif

  printf ("%s: start.\n", argv[0]);

  TRAP_ERROR (InitEventUtility (1, 0, LC_ISFOCUSED), "init event utility");
  TRAP_ERROR (OpenAudioFolio (), "open audio folio");

  testplayer ();

clean:
  CloseAudioFolio ();
  KillEventUtility ();

  printf ("%s: done.\n", argv[0]);

#ifdef MEMDEBUG
  DumpMemDebug (NULL);
  DeleteMemDebug ();
#endif

  return 0;
}

void
testplayer (void)
{
  Item timercue = 0;
  int32 timersig;
  Item samplerins = 0;
  Item outputins = 0;
  SPPlayer *player = NULL;
  SPSound *sound1 = NULL, *sound2 = NULL, *sound3 = NULL;
  SPSound *nextsound = NULL;

  /*
      set up controller polling timer cue
      (!!! really should use a more event driven approach to
           input instead of having to poll the control port)
  */

  TRAP_ERROR (timercue = CreateCue (NULL), "create timer cue");
  TRAP_ERROR (timersig = GetCueSignal (timercue), "get timer signal");

  /* set up instruments and connections */

  TRAP_ERROR (samplerins = LoadInstrument ("fixedmonosample.dsp", 0, 100),
              "Load sample player instrument");
  TRAP_ERROR (outputins = LoadInstrument ("directout.dsp", 0, 100),
              "Load directout.dsp");

  TRAP_ERROR (
      ConnectInstruments (samplerins, "Output", outputins, "InputLeft"),
      "Connect sampler to output");
  TRAP_ERROR (
      ConnectInstruments (samplerins, "Output", outputins, "InputRight"),
      "Connect sampler to output");

  TRAP_ERROR (StartInstrument (outputins, NULL), "Start output");

  /* set up player */

  TRAP_ERROR (spCreatePlayer (&player, samplerins, NUMBUFS, BUFSIZE, NULL),
              "create SPPlayer");

  TRAP_ERROR (spAddSoundFile (&sound1, player, SOUND_1), "add sound " SOUND_1);
  TRAP_ERROR (spAddSoundFile (&sound2, player, SOUND_2), "add sound " SOUND_2);
  TRAP_ERROR (spAddSoundFile (&sound3, player, SOUND_3), "add sound " SOUND_3);

  /* Install decision function at end of each sound. */

  TRAP_ERROR (spSetMarkerDecisionFunction (
                  sound1, SP_MARKER_NAME_END,
                  (SPDecisionFunction)endofsound_decision, &nextsound),
              "decision 1.end");
  TRAP_ERROR (spSetMarkerDecisionFunction (
                  sound2, SP_MARKER_NAME_END,
                  (SPDecisionFunction)endofsound_decision, &nextsound),
              "decision 2.end");
  TRAP_ERROR (spSetMarkerDecisionFunction (
                  sound3, SP_MARKER_NAME_END,
                  (SPDecisionFunction)endofsound_decision, &nextsound),
              "decision 3.end");

  /* play */

  nextsound = sound1; /* set initial state sound selector */
  printf ("tsp_switcher: reading sound $%08lx...", nextsound);
  TRAP_ERROR (spStartReading (nextsound, SP_MARKER_NAME_BEGIN),
              "Start reading");
  printf ("playing\n");
  TRAP_ERROR (spStartPlayingVA (player, AF_TAG_AMPLITUDE, 0x7fff, TAG_END),
              "Start reading");

  TRAP_ERROR (SignalAtTime (timercue, GetAudioTime () + 4), "start timer");

  /* process signals */

  {
    const int32 playersigs = spGetPlayerSignalMask (player);
    ControlPadEventData cped;
    int32 joy, oldjoy = 0;
    bool paused = FALSE;

    while (spGetPlayerStatus (player) & SP_STATUS_F_BUFFER_ACTIVE)
      {
        const int32 sigs = WaitSignal (playersigs | timersig);

        TRAP_ERROR (spService (player, sigs), "Service");

        if (sigs & timersig)
          {
            TRAP_ERROR (SignalAtTime (timercue, GetAudioTime () + 4),
                        "restart timer");
          }

        TRAP_ERROR (GetControlPad (1, FALSE, &cped), "get control pad");
        joy = cped.cped_ButtonBits;

        /* check buttons */

        /* Stop - set next sound to stop, quit when done playing */
        if (TestLeadingEdge (joy, oldjoy, ControlX))
          {
            /* Shift + Stop - quit immediately */
            if (joy & (ControlLeftShift | ControlRightShift))
              break;
            nextsound = NULL;
          }

        /* A - set next sound to sound1 */
        if (TestLeadingEdge (joy, oldjoy, ControlA))
          nextsound = sound1;

        /* B - set next sound to sound2 */
        if (TestLeadingEdge (joy, oldjoy, ControlB))
          nextsound = sound2;

        /* C - set next sound to sound3 */
        if (TestLeadingEdge (joy, oldjoy, ControlC))
          nextsound = sound3;

        /*
            If we're done reading and user requested another sound
            before this loop exits, restart reading the requested
            sound.
        */
        if (!(spGetPlayerStatus (player) & SP_STATUS_F_READING) && nextsound)
          {

            printf ("tsp_switcher: restart reading\n");
            TRAP_ERROR (spStartReading (nextsound, SP_MARKER_NAME_BEGIN),
                        "Restart reading");
          }

        /* Start - Pause/Resume */
        if (TestLeadingEdge (joy, oldjoy, ControlStart))
          {
            paused = !paused;
            if (paused)
              TRAP_ERROR (spPause (player), "pause");
            else
              TRAP_ERROR (spResume (player), "resume");
          }

        oldjoy = joy;
      }
  }

  /* stop */

  printf ("tsp_switcher: stopping\n");
  TRAP_ERROR (AbortTimerCue (timercue), "abort timercue");
  TRAP_ERROR (spStop (player), "Stop");

clean:
  spDeletePlayer (player);
  UnloadInstrument (samplerins);
  UnloadInstrument (outputins);
  DeleteCue (timercue);
}

Err
endofsound_decision (SPAction *resultAction, SPSound *const *nextSoundP,
                     SPSound *sound, const char *markerName)
{
  printf ("tsp_switcher: decision: sound=$%08lx nextsound=$%08lx\n", sound,
          *nextSoundP);

  return *nextSoundP ? spSetBranchAction (resultAction, *nextSoundP,
                                          SP_MARKER_NAME_BEGIN)
                     : spSetStopAction (resultAction);
}

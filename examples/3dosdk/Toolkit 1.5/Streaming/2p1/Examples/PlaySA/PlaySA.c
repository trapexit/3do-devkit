/*******************************************************************************************
 *	File:			PlaySA.c   (Play Streamed Audio)
 *
 *	Contains:		example of using high level stream playback
 *routines for audio
 *
 *	Usage:			TestSA <streamFile>
 *					"A" button:		Toggles stream
 *stop/start
 *					"B" button:		Advances current logical
 *channel number (0 - 3). "C" button:		Rewinds the stream
 *					"LeftShift-A"   Toggles mute on channel
 *0 "LeftShift-B"   Toggles mute on channel 1 "LeftShift-C"   Toggles mute on
 *channel 2 "RightShift-C"  Toggles mute on channel 3
 *
 *					"Start" button:	exits the program
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/03/94		dtc		Version 1.2d1
 *							Fixed documentation on usage of "B"
 *button. Added include <stdio.h> 08/18/94		dtc
 *Version 1.2 Call KillJoyPad to free resources allocated by GetJoyPad and
 *							replace kprintf with
 *printf. Fixed documetation on usage of "B" button. 10/31/93		rdg
 *v1.1  Modify to use logical channel 3 10/26/93		rdg
 *Modify to use more complete joypad implementation Add button combinations for
 *muting channels 10/25/93		rdg		New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.2"

/* If true, this will cause a Close Channel message to be sent to
   channels 0-4 of the audio subscriber when the stream is rewound
   with the "C" button.  Close channel causes all resources which
   have been allocated for the channel to be freed.  This allows
   you to change data types on the fly without having to keep DSP
   instruments loaded that you no longer need.
   ее NOTE: This option can sometimes cause audible pops because
                        the instruments are freed before the envelope can
                        ramp all the way to zero.
 */
#define CLOSE_AUDIO_ON_REWIND 0

/* This value is the number of audio ticks PlaySSNDStream() will sleep
 * between callbacks to the user function.  In other words, how often
 * you will be called back.  Use to keep the task from
 * busy waiting; just reading the control pad as fast as possible.
 */
#define UI_CALL_BACK_INTERVAL 5

#include "Init3DO.h"
#include "JoyPad.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "PlaySSNDStream.h"
#include "SAudioSubscriber.h"

#include <stdio.h>

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean StartUp (void);
static void Usage (char *programName);
long DoUserInterface (PlayerPtr ctx);

/**********************************************************/
/* Context Block for maintaining state in the UI function */
/**********************************************************/
typedef struct AudioCB
{
  JoyPadState jpState; /* JoyPad context */
  long curChannel;     /* Channel on which to send volume and pan messages */
  long volume;
  long pan;
  ulong muteStatusBits; /* Bits represent which channels are currently muted */
  SAudioCtlBlock ctlBlock; /* For sending control messages to the subscriber */

} AudioCB, *AudioCBPtr;

AudioCB UICtx;

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean
StartUp (void)
{
  if (!OpenMacLink ())
    ;

  if (!OpenSPORT ())
    return false;

  if ((OpenAudioFolio () != 0))
    return false;

  if ((OpenMathFolio () != 0))
    return false;

  return true;
}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *programName)
{
  printf ("%s version %s\n", programName, PROGRAM_VERSION_STRING);
  printf ("usage: %s <streamFile> \n", programName);
  printf ("\t\"A\" button:	Toggles stream stop/start\n");
  printf ("\t\"B\" button:	Toggles current logical channel number "
          "between 0 and 3.\n");
  printf ("\t\"C\" button:	Rewinds the stream.\n");
  printf ("\t\"LeftShift-A\"  Toggles mute on channel 0.\n");
  printf ("\t\"LeftShift-B\"  Toggles mute on channel 1.\n");
  printf ("\t\"LeftShift-C\"  Toggles mute on channel 2.\n");
  printf ("\t\"RightShift-C\"  Toggles mute on channel 3.\n");
}

/*********************************************************************************************
 * This function is called each time through the player's main loop and is used
 *to implement a user interface. Returning a zero causes playback to continue.
 *Any other value causes playback to cease and immediately return from the
 *PlaySSNDStream() function.
 *********************************************************************************************/
long
DoUserInterface (PlayerPtr ctx)
{
  long status;
  AudioCBPtr UICtx;

  /* For readability, get the user interface context block from the player
   * context */
  UICtx = (AudioCBPtr)ctx->userCB;

  /* Read the control pad. If the start button is pressed, then
   * return a non-zero value to cause the stream to stop playing.
   */
  GetJoyPad (&UICtx->jpState, 1);
  if (UICtx->jpState.xBtn)
    return 1;

  /* Now test for button presses and do appropriate stuff */

  /* The "A" button toggles running the stream */
  if (UICtx->jpState.aBtn && !UICtx->jpState.leftShift)
    {
      /* If we're transitioning from stopped to running, start the
       * stream. Otherwise, stop the stream.
       */
      if (!(ctx->streamCBPtr->streamFlags & STRM_RUNNING))
        {
          status = DSStartStream (ctx->messageItem, NULL, ctx->streamCBPtr, 0);
          CHECK_DS_RESULT ("DSStartStream", status);
          printf ("Stream started.\n");
        }
      else
        {
          /* Note: we don't use the SOPT_FLUSH option when we stop
           * here so that the "A" button acts like a pause/resume
           * key. Using the SOPT_FLUSH option will cause buffered
           * data to be flushed, so that once resumed, any queued
           * audio will vanish. This is not what is usually desired
           * for pause/resume.
           */
          status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr, 0);
          CHECK_DS_RESULT ("DSStopStream", status);
          printf ("Stream stopped.\n");
        }

      printf (" stream flags = %lx\n", ctx->streamCBPtr->streamFlags);
    }

  /* The "B" button advances the "current channel" i.e. which channel has the
   * volume and pan UI focus.  Channels 0 - 3 are active.
   */
  if (UICtx->jpState.bBtn && !UICtx->jpState.leftShift)
    {
      UICtx->curChannel += 1;

      /* Wrap from 3 to 0 */
      if (UICtx->curChannel == 4)
        UICtx->curChannel = 0;

      printf ("Current channel = %ld.\n", UICtx->curChannel);
    }

  /* The "C" rewinds and re-starts the stream */
  if (UICtx->jpState.cBtn && !UICtx->jpState.leftShift
      && !UICtx->jpState.rightShift)
    {
      /* Stop the current stream and flush the subscriber to make sure
       * it doesn't hang onto any buffers.
       */
      status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr,
                             SOPT_FLUSH);
      CHECK_DS_RESULT ("DSStopStream", status);

      printf ("Rewinding stream...\n");

#if CLOSE_AUDIO_ON_REWIND
      /* Send close message to all 4 channels */
      for (UICtx->curChannel = 0; UICtx->curChannel < 4; UICtx->curChannel++)
        {
          UICtx->ctlBlock.closeChannel.channelNumber = UICtx->curChannel;
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpCloseChannel,
                              &UICtx->ctlBlock);
          CHECK_DS_RESULT ("DSControl - closing channel", status);

          /* ReEnable audio channels so they will receive data */
          status = DSSetChannel (ctx->messageItem, NULL, ctx->streamCBPtr,
                                 SNDS_CHUNK_TYPE, UICtx->curChannel,
                                 CHAN_ENABLED);
          CHECK_DS_RESULT ("DSSetChannel", status);
        }

      /* Reset the current channel to 0 */
      UICtx->curChannel = 0;
      printf ("Current channel = %ld.\n", UICtx->curChannel);
#endif

      /* Rewind the stream to 0 */
      status = DSGoMarker (ctx->messageItem, NULL, ctx->streamCBPtr,
                           (unsigned long)0, GOMARKER_ABSOLUTE);
      CHECK_DS_RESULT ("DSGoMarker", status);

      /* Restart the stream now that we have rewound. */
      status = DSStartStream (ctx->messageItem, NULL, ctx->streamCBPtr, 0);
      CHECK_DS_RESULT ("DSStartStream", status);
    }

  /* The Up Arrow increases the amplitude of the "current channel". */
  if (UICtx->jpState.upArrow)
    {
      UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpGetVol, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
      UICtx->volume = UICtx->ctlBlock.amplitude.value;

      if (UICtx->volume < 0x7A00)
        {
          UICtx->volume += 0x500;

          UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.amplitude.value = UICtx->volume;
        }
      else
        {
          UICtx->volume = 0x7FFF;

          UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.amplitude.value = UICtx->volume;
        }

      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpSetVol, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* The Down Arrow Decreases the amplitude of the "current channel". */
  if (UICtx->jpState.downArrow)
    {
      UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpGetVol, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
      UICtx->volume = UICtx->ctlBlock.amplitude.value;

      if (UICtx->volume > 0x600)
        {
          UICtx->volume -= 0x500;

          UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.amplitude.value = UICtx->volume;
        }
      else
        {
          UICtx->volume = 0x0;

          UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.amplitude.value = UICtx->volume;
        }

      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpSetVol, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* The Left Arrow pans the "current channel" to the left. */
  if (UICtx->jpState.leftArrow)
    {
      UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpGetPan, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
      UICtx->pan = UICtx->ctlBlock.pan.value;

      if (UICtx->pan > 0x600)
        {
          UICtx->pan -= 0x500;

          UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.pan.value = UICtx->pan;

          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpSetPan,
                              &UICtx->ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
        }
    }

  /* The Right Arrow pans the "current channel" to the right. */
  if (UICtx->jpState.rightArrow)
    {
      UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpGetPan, &UICtx->ctlBlock);
      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
      UICtx->pan = UICtx->ctlBlock.pan.value;

      if (UICtx->pan < 0x7A00)
        {
          UICtx->pan += 0x500;

          UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
          UICtx->ctlBlock.pan.value = UICtx->pan;

          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpSetPan,
                              &UICtx->ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
        }
    }

  /* The combination LeftShift-A, toggles the mute on channel 0 */
  if (UICtx->jpState.leftShift && UICtx->jpState.aBtn)
    {
      UICtx->ctlBlock.mute.channelNumber = 0;

      if (UICtx->muteStatusBits & 0x1)
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpUnMute,
                              &UICtx->ctlBlock);

          /* Clear the mute status bit for this channel */
          UICtx->muteStatusBits &= ~0x1;
        }
      else
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpMute,
                              &UICtx->ctlBlock);

          /* Set the mute status bit for this channel */
          UICtx->muteStatusBits |= 0x1;
        }

      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* The combination LeftShift-B, toggles the mute on channel 1 */
  if (UICtx->jpState.leftShift && UICtx->jpState.bBtn)
    {
      UICtx->ctlBlock.mute.channelNumber = 1;

      if (UICtx->muteStatusBits & 0x2)
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpUnMute,
                              &UICtx->ctlBlock);
          /* Clear the mute status bit for this channel */
          UICtx->muteStatusBits &= ~0x2;
        }
      else
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpMute,
                              &UICtx->ctlBlock);

          /* Set the mute status bit for this channel */
          UICtx->muteStatusBits |= 0x2;
        }

      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* The combination LeftShift-C, toggles the mute on channel 2 */
  if (UICtx->jpState.leftShift && UICtx->jpState.cBtn)
    {
      UICtx->ctlBlock.mute.channelNumber = 2;

      if (UICtx->muteStatusBits & 0x4)
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpUnMute,
                              &UICtx->ctlBlock);

          /* Clear the mute status bit for this channel */
          UICtx->muteStatusBits &= ~0x4;
        }
      else
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpMute,
                              &UICtx->ctlBlock);

          /* Set the mute status bit for this channel */
          UICtx->muteStatusBits |= 0x4;
        }

      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* The combination RightShift-C, toggles the mute on channel 3 */
  if (UICtx->jpState.rightShift && UICtx->jpState.cBtn)
    {
      UICtx->ctlBlock.mute.channelNumber = 3;

      if (UICtx->muteStatusBits & 0x8)
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpUnMute,
                              &UICtx->ctlBlock);

          /* Clear the mute status bit for this channel */
          UICtx->muteStatusBits &= ~0x8;
        }
      else
        {
          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpMute,
                              &UICtx->ctlBlock);

          /* Set the mute status bit for this channel */
          UICtx->muteStatusBits |= 0x8;
        }

      if (status < 0)
        {
          printf ("\n error sending control message!\n");
          printf (" status = %ld.\n", status);
        }
    }

  /* No problems, don't want to exit stream */
  return 0;
}

/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int
main (int argc, char **argv)
{
  long status;

  /* Sanity check, need at least the name of a file */
  if (argc < 2)
    {
      printf ("Error: %s - invalid parameter list, need a stream file name.\n",
              argv[0]);
      Usage (argv[0]);
      exit (1);
    }

  /* Initialize the library code */
  if (!StartUp ())
    {
      printf ("StartUp() initialization failed!\n");
      exit (1);
    }

  /* Let the JoyPad code know we expect the shift buttons to be held down */
  SetJoyPadContinuous (PADSHIFT, 1);

  /* Play the stream, specify how often to call back */
  status = PlaySSNDStream (argv[1], (PlaySSNDUserFn)DoUserInterface,
                           (void *)&UICtx, (int32)UI_CALL_BACK_INTERVAL);
  CHECK_DS_RESULT ("PlaySSNDStream", status);

  if (status == 1)
    printf ("Stream stopped by control pad input\n");

  /* Free resources allocated by GetJoyPad (specifically by InitEventUtility
   * which is called by GetJoyPad).  KillJoypad returns an error code, but
   * we're not checking for it.
   */
  KillJoypad ();

  exit (0);
}

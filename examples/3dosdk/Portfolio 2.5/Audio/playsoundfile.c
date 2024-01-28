
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: playsoundfile.c,v 1.31 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/playsoundfile
|||	playsoundfile - Plays AIFF sound files from disk.
|||
|||	  Synopsis
|||
|||	    playsoundfile \<sample file>...
|||
|||	  Description
|||
|||	    Calls SoundFilePlayer routines to play one or more AIFF files.
|||
|||	  Arguments
|||
|||	     sample file                 Name of an AIFF file to play.
|||
|||	  Associated Files
|||
|||	    playsoundfile.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
|||	  See Also
|||
|||	    spoolsoundfile, tsp_spoolsoundfile
|||
**/
#include "audio.h"
#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "music.h"
#include "operror.h"
#include "types.h"
#include <stdio.h>

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

#define MAXAMPLITUDE (0x7FFF)

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, Result);                           \
      goto error;                                                             \
    }

#define CHECKPTR(val, name)                                                   \
  if (val == 0)                                                               \
    {                                                                         \
      Result = -1;                                                            \
      ERR (("Failure in %s\n", name));                                        \
      goto error;                                                             \
    }

/* Adjust these parameters to suit your application. */
/* Note that blocksize may not be 2048 on future systems. */
#define NUMBLOCKS (16)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)
#define NUMBUFFS (4)
#define USEOWNBUFFERS

int32 PlaySoundFile (SoundFilePlayer *sfp, char *FileName);

static ControlPadEventData cped;

int
main (int argc, char *argv[])
{
  int32 Result = 0;
  char *theName;
  SoundFilePlayer *sfp;
  int32 i;
  void *Buffers[MAX_SOUNDFILE_BUFS], **PassBuffers;

  PRT (("Usage: %s <samplefile>\n", argv[0]));

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintError (0, "init event utility", 0, Result);
      goto error;
    }

#ifdef USEOWNBUFFERS
  for (i = 0; i < NUMBUFFS; i++)
    {
      Buffers[i] = AllocMemFromMemLists (
          KernelBase->kb_CurrentTask->t_FreeMemoryLists, BUFSIZE,
          MEMTYPE_AUDIO);
      if (Buffers[i] == NULL)
        {
          ERR (("Buffer allocation failed!\n"));
          goto error;
        }
    }
  PassBuffers = Buffers;
#else
  PassBuffers = NULL;
#endif

  /* Play the sound file. */
  sfp = CreateSoundFilePlayer (NUMBUFFS, BUFSIZE, PassBuffers);
  CHECKPTR (sfp, "CreateSoundFilePlayer");

  /* Optionally do not connect to a directout.dsp */
  /*	sfp->sfp_Flags |= SFP_NO_DIRECTOUT; */

  for (i = 1; i < argc; i++)
    {
      /* Get sample name from command line. */
      theName = (char *)argv[i];
      Result = PlaySoundFile (sfp, theName);
    }

  DeleteSoundFilePlayer (sfp);

  /* Cleanup the EventBroker. */
  Result = KillEventUtility ();
  if (Result < 0)
    {
      PrintError (0, "kill event utility", 0, Result);
    }

#ifdef USEOWNBUFFERS
  for (i = 0; i < NUMBUFFS; i++)
    {
      FreeMemToMemLists (KernelBase->kb_CurrentTask->t_FreeMemoryLists,
                         Buffers[i], BUFSIZE);
    }
#endif

error:
  CloseAudioFolio ();
  PRT (("Playback complete.\n"));

  return ((int)Result);
}

/**************************************************************************
** PlaySoundFile
**************************************************************************/

int32
PlaySoundFile (SoundFilePlayer *sfp, char *FileName)
{
  int32 Result;
  int32 SignalsIn = 0, SignalsNeeded = 0;

  Result = LoadSoundFile (sfp, FileName);
  CHECKRESULT (Result, "LoadSoundFile");

  Result = StartSoundFile (sfp, MAXAMPLITUDE);
  CHECKRESULT (Result, "StartSoundFile");

  /* Keep playing until no more samples. */
  do
    {
      Result = ServiceSoundFile (sfp, SignalsIn, &SignalsNeeded);
      CHECKRESULT (Result, "ServiceSoundFile");

      /* Get User input. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          PrintError (0, "read control pad in", "PlaySoundFile", Result);
        }

      if (SignalsNeeded)
        SignalsIn = WaitSignal (SignalsNeeded);
    }
  while ((SignalsNeeded) && !(cped.cped_ButtonBits & ControlStart));

  Result = StopSoundFile (sfp);
  CHECKRESULT (Result, "StopSoundFile");

  Result = UnloadSoundFile (sfp);
  CHECKRESULT (Result, "UnloadSoundFile");

  return 0;

error:
  return (Result);
}

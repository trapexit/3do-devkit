/***************************************************************
**
** Play Sound File from Disk.
** This is a primitive model for demo purposes.  The final version will
** work from a separate thread or task to allow background play.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include "audio.h"
#include "debug.h"
#include "filefunctions.h"
#include "music.h"
#include "operror.h"
#include "types.h"

/* If you print while playing the file it will skip. */
#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

#define MAXAMPLITUDE (0x7FFF)
#define AUDIODIR "/remote"

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (Result);                                                  \
      goto error;                                                             \
    }

#define CHECKPTR(val, name)                                                   \
  if (val == 0)                                                               \
    {                                                                         \
      Result = -1;                                                            \
      ERR (("Failure in %s\n", name));                                        \
      goto error;                                                             \
    }

#define NUMBLOCKS (64)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)

int32 LoopSoundFile (char *FileName, int32 BufSize);

int
main (int argc, char *argv[])
{
  int32 Result;
  char *theName;
  Item NewDir;

  PRT (("Usage: %s <samplefile>\n", argv[0]));

  /* Get sample name from command line. */
  if (argc > 1)
    {
      theName = (char *)argv[1];
    }
  else
    {
      theName = "/remote/aiff/Yosemite.aiff";
    }

  /* Set directory for audio data files. */
  NewDir = ChangeDirectory (AUDIODIR);
  if (NewDir > 0)
    {
      PRT (("Directory changed to %s for Mac based file-system.\n", AUDIODIR));
    }
  else
    {
      ERR (("Directory %s not found! Err = $%x\n", AUDIODIR, NewDir));
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Play the sound file. */
  Result = LoopSoundFile (theName, BUFSIZE);

  CloseAudioFolio ();
  PRT (("Playback complete.\n"));

  return ((int)Result);
}

/**************************************************************************
** PlaySoundFile - hangs until finished so rewrite and stick in a thread
**************************************************************************/

int32
LoopSoundFile (char *FileName, int32 BufSize)
{
  int32 Result;
  SoundFilePlayer *sfp;
  int32 doit;

  sfp = OpenSoundFile (FileName, 2, BufSize);
  CHECKPTR (sfp, "OpenSoundFile");

  Result = StartSoundFile (sfp, MAXAMPLITUDE);
  CHECKRESULT (Result, "StartSoundFile");

  /* Keep tasking player until no more samples. */
  doit = TRUE;
  while (doit)
    {
      while (sfp->sfp_LastPos < sfp->sfp_NumToRead)
        {
          DBUG (("LastPos = %d\n", sfp->sfp_LastPos));
          SleepAudioTicks (1); /* Or do other stuff but don't take too long. */
          Result = BumpSoundFile (sfp);
          CHECKRESULT (Result, "BumpSoundFile");
        }
      SleepAudioTicks (4);

      Result = StopSoundFile (sfp);
      CHECKRESULT (Result, "StopSoundFile");
      RewindSoundFile (sfp);
      DBUG (("NumToRead = %d\n", sfp->sfp_NumToRead));
      Result = StartSoundFile (sfp, MAXAMPLITUDE);
      CHECKRESULT (Result, "StartSoundFile");
    }

  Result = StopSoundFile (sfp);
  CHECKRESULT (Result, "StopSoundFile");
  Result = CloseSoundFile (sfp);
  CHECKRESULT (Result, "CloseSoundFile");

  return 0;

error:
  return (Result);
}

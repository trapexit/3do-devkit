/*
        File:		CSound.cp

        Contains:	Sound class.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CSound.h"
#include "audio.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "CPlusSwiHack.h"

CSound::CSound (char *pSndFileName, long vol)
{
  OpenAudioFolio ();
  fSndFile = OpenSoundFile (pSndFileName, 2, BUFSIZE);
  fVolume = vol;
  this->Stop ();
  this->Rewind ();
}

CSound::~CSound (void)
{
  this->Stop ();
  if (fSndFile)
    CloseSoundFile (fSndFile);
}

long
CSound::GetVolume (void)
{
  return fVolume;
}

void
CSound::Loop (long nTimes)
{
  this->Rewind ();
  while (nTimes--)
    {
      this->Play ();
      while (fSndFile->sfp_LastPos < fSndFile->sfp_NumToRead)
        {
          BumpSoundFile (fSndFile);
        }
      this->Stop ();
      this->Rewind ();
    }
}

void
CSound::Play (void)
{
  if (fSndFile)
    StartSoundFile (fSndFile, fVolume);
}

void
CSound::SetVolume (long vol)
{
  fVolume = vol;

  if (fSndFile)
    StartSoundFile (fSndFile, fVolume);
}

void
CSound::Stop (void)
{
  if (fSndFile)
    StopSoundFile (fSndFile);
}

void
CSound::Rewind (void)
{
  if (fSndFile)
    RewindSoundFile (fSndFile);
}

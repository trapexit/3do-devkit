/*
        File:		CInstrument.cp

        Contains:	Instrument class.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <2>	  1/26/94	CDE		Cleanup for 3DO.
        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CInstrument.h"

#include "CPlusSwiHack.h"

CInstrument::CInstrument (char *pSndFileName)
{
  OpenAudioFolio ();
  fSndItem = LoadSoundEffect (pSndFileName, 1);
  if (fSndItem < 0)
    {
      DIAGNOSTIC ("Sound Effect not found");
      fSndItem = 0;
    }
  fIsPlaying = 0;
  fIsPaused = 0;
  this->SetRate (0x4000);
  this->SetVolume (0x7FFF);
  this->SetChannel (0);
  fSndTag[2].ta_Tag = 0;
  fSndTag[2].ta_Arg = 0;
}

CInstrument::CInstrument (char *pSndFileName, long rate, long vol)
{
  OpenAudioFolio ();
  fSndItem = LoadSoundEffect (pSndFileName, 1);
  if (fSndItem < 0)
    {
      DIAGNOSTIC ("Sound Effect not found");
      fSndItem = 0;
    }
  fIsPlaying = 0;
  fIsPaused = 0;
  this->SetRate (rate);
  this->SetVolume (vol);
  this->SetChannel (0);
  fSndTag[2].ta_Tag = 0;
  fSndTag[2].ta_Arg = 0;
}

CInstrument::~CInstrument (void)
{
  this->Stop ();
  if (fSndItem)
    UnloadInstrument (fSndItem);
}

void
CInstrument::Pause (void)
{
  if (!this->IsPaused () && this->IsPlaying ())
    {
      PauseInstrument (fSndItem);
      fIsPaused = 1;
    }
}

void
CInstrument::Play (void)
{
  if (!this->IsPlaying ())
    {
      StartInstrument (fSndItem, fSndTag);
      fIsPlaying = 1;
    }
}

void
CInstrument::Stop (void)
{
  if (this->IsPlaying ())
    {
      StopInstrument (fSndItem, fSndTag);
      fIsPlaying = 0;
    }
}

void
CInstrument::Reset (void)
{
  if (this->IsPlaying ())
    {
      this->Stop ();
      this->Play ();
    }
}

void
CInstrument::Resume (void)
{
  if (this->IsPaused () && this->IsPlaying ())
    {
      ResumeInstrument (fSndItem);
      fIsPaused = 0;
    }
}

void
CInstrument::SetChannel (long chn)
{
  ::SetChannel (fSndItem, (int)chn);
  this->Reset ();
}

void
CInstrument::SetRate (long rate)
{
  fSndTag[0].ta_Tag = AF_TAG_RATE;
  fSndTag[0].ta_Arg = (void *)rate;
  this->Reset ();
}

void
CInstrument::SetVolume (long vol)
{
  fSndTag[1].ta_Tag = AF_TAG_AMPLITUDE;
  fSndTag[1].ta_Arg = (void *)vol;
  this->Reset ();
}

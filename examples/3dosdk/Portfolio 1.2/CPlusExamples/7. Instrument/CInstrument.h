/*
        File:		CInstrument.h

        Contains:	Instrument class (header).

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
#ifndef _CINSTRUMENT_H_
#define _CINSTRUMENT_H_

#include "Portfolio.h"
#include "Utils3DO.h"

class CInstrument
{
public:
  CInstrument (char *pSndFileName);
  CInstrument (char *pSndFileName, long rate, long vol);
  ~CInstrument (void);

  long GetChannel (void);
  long GetRate (void);
  long GetVolume (void);
  Boolean IsPaused (void);
  Boolean IsPlaying (void);
  Boolean IsValid (void);
  void Pause (void);
  void Play (void);
  void Reset (void);
  void Resume (void);
  void SetChannel (long chn);
  void SetRate (long rate);
  void SetVolume (long vol);
  void Stop (void);

private:
  Item fSndItem;
  TagArg fSndTag[3]; // 0 - rate, 1 - ampl
  long fChannel;
  Boolean fIsPlaying;
  Boolean fIsPaused;
};

inline long
CInstrument::GetChannel (void)
{
  return fChannel;
}

inline long
CInstrument::GetRate (void)
{
  return (long)fSndTag[0].ta_Arg;
}

inline long
CInstrument::GetVolume (void)
{
  return (long)fSndTag[1].ta_Arg;
}

inline Boolean
CInstrument::IsValid (void)
{
  return fSndItem != 0L;
}

inline Boolean
CInstrument::IsPaused (void)
{
  return (fIsPaused != 0L);
}

inline Boolean
CInstrument::IsPlaying (void)
{
  return (fIsPlaying != 0L && fSndItem);
}

#endif
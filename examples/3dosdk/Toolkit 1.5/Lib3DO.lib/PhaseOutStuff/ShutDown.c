/*****************************************************************************
 *	File:			ShutDown.c
 *
 *	Contains:		Mis-named routine; really just closes the audio
 *folio.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/28/93  JAY	remove ResetSystemGraphics(). It went away in Dragon
 *Tail release. 07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Init3DO.h"
#include "audio.h"

void
ShutDown (void)
{
  CloseAudioFolio ();
}

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: playbgndmusic.c,v 1.9 1994/12/26 22:37:23 ceckhaus Exp $
**
******************************************************************************/
/**
|||	playbgndmusic - Play an audio loop as a separate background task
|||
|||	    Opens the audio folio ( because each task must open the folios it uses ), then
|||		calls the library function LoopStereoSoundFile to endlessly play
|||		the background music file.
|||
|||	  Associated Files
|||
|||	    playbgndmusic.make					makefile for this program
|||	    playbgndmusic.task					the output file of the build, which must be
|||	    									placed in ufosound.
|||	    The data file is:
|||
|||	    - ufosound/backgroundmusic.aiff		the sound file played by this task
**/

#include "types.h"
#include "audio.h"
#include "debug3do.h"
#include "loopstereosoundfile.h"

int main(int argc, char* argv[])
    {

    int32 Result;

    if ( (Result = OpenAudioFolio()) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't open the audio folio\n") );
        goto CLEANUP;
        }

    /*
		Play the stereo sample in an endless loop (the "JSData" component is
		omitted from the pathname because this task is launched from that
		directory)
	*/
	LoopStereoSoundFile( "ufosound/backgroundmusic.aiff" );

CLEANUP:
    CloseAudioFolio();

    return 0;
    }

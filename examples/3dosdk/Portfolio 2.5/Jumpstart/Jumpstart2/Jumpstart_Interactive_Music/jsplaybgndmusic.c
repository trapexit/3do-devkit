
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsplaybgndmusic.c,v 1.13 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsplaybgndmusic
|||	jsplaybgndmusic - Plays an audio loop as a separate background task.
|||
|||	  Synopsis
|||
|||	    jsplaybgndmusic
|||
|||	  Description
|||
|||	    Plays an audio loop as a separate background task.
|||
|||	    Opens the audio folio (because each task must open the folios it uses),
|||	    opens the sound file, and enters a loop in which the sound is played (by
|||	    the inner loop) and restarted when it finishes.
|||
|||	  Associated Files
|||
|||	    jsplaybgndmusic.c, jsplaybgndmusic.make
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jsinteractivemusic
|||
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
	LoopStereoSoundFile( "sound/backgroundmusic.aiff" );

CLEANUP:
    CloseAudioFolio();

    return 0;
    }

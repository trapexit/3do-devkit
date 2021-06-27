
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: joystick_example.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/joystick_example
|||	joystick_example - Demonstrates programming for joystick.
|||
|||	  Synopsis
|||
|||	    joystick_example
|||
|||	  Description
|||
|||	    Demonstrates simple interaction with the joystick using data filtering and
|||	    calibration. Lets you move a cel (a cloud) around the screen with the
|||	    joystick and leaves a still of it on screen when you press the trigger.
|||
|||	    Joytick controls Stick move in x and y Zzoom in/out U/D/L/R switchdistort
|||	    cel Triggerleave copy of cel on screen Aclear screen Brecalibrate stick
|||	    (to recalibrate, hit B with stick in neutral position and move the stick
|||	    and z throttle to the limits of travel) Cquit
|||
|||	  Caveats
|||
|||	    This is a simplified version of the broker shell example that just does
|||	    joystick interaction.
|||
|||	  Associated Files
|||
|||	    joystick_example.c, cloud.cel
|||
|||	  Location
|||
|||	    examples/EventBroker/Joystick
|||
**/

#if 0
The basic workings of the broker shell are as follows:

/* make a communications connection to the event broker */
BS_ConnectBroker();

/* listen for the devices you want */
/* events from */
BS_WatchJoyStick();

/* you can interleave Ignore and Watch calls */
/* to pay attention to whatever you want at a given */
/* time.with */
BS_IgnoreJoyStick()

/* then wait for event broker events in a cpu-friendly */
/* fashion with */
BS_NiceWaitEvent();

/* BS_NiceWaitEvent() returns a mask that lets you know */
/* which devices have received data, and if you */
/* have recieved a signal besides an event broker message signal */
/* you can get the signals you received with */
BS_GetOtherSignals();

/* the mask BS_PORT_CHANGE */
/* lets you know that there has been a change on the */
/* control port device daisy chain */
/* you can get a tally of devices with */
BS_GetPeripheralCount();

/* you can find out if you are watching a device */
/* currently and what events you are getting from it */
/* with */
BS_WatchingJoyStick();


/* with your received data you can */
/* debounce, filter, fold mutilate, */
/* and spindle it with */
BS_ProcessJoyStickData();

/* shut down the connection to the broker */
/* and free up temporary data allocations */
BS_DisconnectBroker();

#endif


#include "types.h"
#include "string.h"
#include "mem.h"
#include "displayutils.h"
#include "celutils.h"
#include "utils3do.h"
#include "stdio.h"

/* includes the bs_<device>.h files */
/* as well as event.h */
#include "broker_shell.h"

/* Error Condition Masks for */
/* the question "Why did I bail?" */
#define GRAPHICSMASK 	1
#define AUDIOMASK 		2
#define SPORTIOMASK 	4
#define MACLINKMASK 	8
#define EVENTMASK 		16

ScreenContext *mySc;
Item myVBLIOReq;
Item VRAMIOReq;
CCB*	imageCCB;
ubyte	*gBackPic = NULL;

CCB *
LoadACel(char *fn)
{
	imageCCB=LoadCel(fn,MEMTYPE_CEL);

	if(!imageCCB)
	{
		printf("Error in Load Cel call.\n");
		exit(0);
	}

	LAST_CEL((imageCCB));
	FastMapCelInit(imageCCB);

	return imageCCB;
}

/*
 * get all the folios loaded, plus the rest of the global state set up.
 */
long
setupFolios(void)
{
	long retval = 0;

	myVBLIOReq = CreateVBLIOReq();
	VRAMIOReq = CreateVRAMIOReq();

	mySc = (ScreenContext *) AllocMem(sizeof(ScreenContext),MEMTYPE_CEL );

	if (mySc == NULL) {
		DIAGNOSTIC("winopen:Cannot Allocate memory for ScreenContext ");
		exit(1);
	}

	if (CreateBasicDisplay(mySc,DI_TYPE_NTSC,2) < 0)
		{
		DIAGNOSTIC("Cannot initialize graphics");
		}
	else retval |= GRAPHICSMASK;

	if ((gBackPic = (ubyte *)AllocMem( (int)(mySc->sc_nFrameByteCount),
			GetBankBits( GrafBase->gf_ZeroPage )
			| MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL)) == NULL) {
				DIAGNOSTIC ( "unable to allocate gBackPic" );
	}
	SetVRAMPages( VRAMIOReq, gBackPic, 0, mySc->sc_nFrameBufferPages, ~0 );

	if ( DisplayScreen( mySc->sc_Screens[mySc->sc_curScreen], 0 ) < 0 ) {
			DIAGNOSTIC( "DisplayScreen() failed");
	}

	imageCCB = LoadACel("$boot/cloud.cel");

	return retval;
}

/*
 * utility to fake the double buffering
 */
void
CopyPic( void *dest, void *src )
{
	CopyVRAMPages(	VRAMIOReq, dest, src,
					mySc->sc_nFrameBufferPages, -1 );
}

void
drawIt(Point quad[4],int drawCopyF,int clearF)
{
	/* copy stashed image into frame buffer */
	CopyPic( mySc->sc_Bitmaps[mySc->sc_curScreen]->bm_Buffer, gBackPic );

	FastMapCel(imageCCB, quad);
	DrawCels(mySc->sc_BitmapItems[mySc->sc_curScreen], imageCCB);

	/* clear the stashed iamge */
	if(clearF) {
		SetVRAMPages( VRAMIOReq, gBackPic, 0, mySc->sc_nFrameBufferPages, -1 );
	}

	/* save our current frame buffer into the stash */
	if (drawCopyF) {
		CopyPic( gBackPic, mySc->sc_Bitmaps[mySc->sc_curScreen]->bm_Buffer );
	}
}

int
updateCel(
	int32 x,int32 y,int32 z,
	uint32 buttonBits,
	Point quad[4],
	int *fireF,
	int *clearF)
{
  static int32  dx = 0, dy = 0;

	if(buttonBits&StickRight)
		dx += 2;

	if(buttonBits&StickLeft)
		dx -= 2;

	if(buttonBits&StickDown)
		dy += 2;

	if(buttonBits&StickUp)
		dy -= 2;

	quad[0].pt_X = x + 0;		quad[0].pt_Y = y + 0;
	quad[1].pt_X = x + z + dx;	quad[1].pt_Y = y + 0;
	quad[2].pt_X = x + z + dx;	quad[2].pt_Y = y + z + dy;
	quad[3].pt_X = x + 0;		quad[3].pt_Y = y + z + dy;

	*fireF=(int)(buttonBits&StickFire)&&1;
	*clearF=(int)(buttonBits&StickA)&&1;

	if(buttonBits&StickC)
		return 0;
	return 1;
}

static void
handleOtherSignal(uint32 sigs)
{
	printf("joystick example: recieved signal(s): 0x%x\n",sigs);
}

static void
handlePortChange(void)
{
  int32 js;

	printf("joystick example sez -- there are now:\n");
	printf("     %d joy stick(s)\n",js=BS_GetPeripheralCount(BS_STICK));

	/* pay attention to what is connected */
	/* you can interleave watch and ignore commands as */
	/* often as you like.  calling 2 ignore commands in a row */
	/* for the same device does not harm anything and */
	/* it only takes one watch command to see it again (the ignores don't stack) */
	/*  */
	/* you can also reconfigure your watching (i.e. call BS_WatchJoyStick(1) after */
	/* you've already called BS_WatchJoyStick(0) to get button up events too) */
	/* without having to call an ignore in the middle. */
	/* if you call the two watches to the same device with the same argument */
	/* there are no side effects. */

	if(js)
	{
		if(!BS_WatchingJoyStick())
			BS_WatchJoyStick(0);
	}
	else
	{
		printf("joystick_example sez: no joysticks plugged in!\n");
		if(BS_WatchingJoyStick())
			BS_IgnoreJoyStick();
	}
}

int
main(int argc, char **argv)
{
  /* quadrangle for mapping cel with FastMapCel */
  Point quad[4];

  /* some silly flags */
  int fireF=0,clearF=0;

  /* exit code */
  int32 eCode=0;

  /* Stick Data storage */
  BS_StickData stick1;

  /* 10 of these ought to be safe. */
  BS_StickData *sticks[10];

  /* Debouncing choices */
  uint32 sDebounceBits=StickA|StickB|StickC|StickFire;

  /* another silly flag */
  int cont=1;

  	/* Luuuuucy, you got some 'SPLAINING to dooo... */
	printf("joystick module demo\n");
	printf("you will want to start by calibrating the stick.\n");
	printf("Joystick controls:\n");
	printf("     Stick: move in x and y\n");
	printf("     Z: zoom in/out\n");
	printf("     U/D/L/R switch: deform cel\n");
	printf("     Trigger: leave copy of cel on screen\n");
	printf("     A: clear screen\n");
	printf("     B: recalibrate stick\n");
	printf("        to recalibrate, hit 'B' with stick in neutral position\n");
	printf("        and move the stick and z throttle to the limits of travel\n");
	printf("     C: quit\n");


    /* set the pointers in the array all to NULL */
	/* this is ABSOLUTELY NECESSARY */
	memset(sticks,0,sizeof(sticks));

	/* we are only interested in the first stick */
	/* this is a device-specific index, not */
	/* daisy-chain spot specific */
	/* the BS_NiceWaitEvent() call */
	/* will put data from the first stick into the structure */
	/* pointed to by the first entry in the sticks array, data from */
	/* the second stick will go into the second entry and so on. */
	/* if a null array pointer is passed, then stick events will be */
	/* passed over.  this is not the same as ignoring because the messages */
	/* are still getting received from the broker.  it is more efficient */
	/* to use an Ignore call for a device you don't want to hear from. */
	/* if a null pointer is found in the structure pointer array, then */
	/* data from that numbered device is passed over.  i.e. we have put */
	/* a valid pointer in the first slot of ''sticks'' so the first stick's data */
	/* will go there.  there is a null pointer in the second through tenth */
	/* slots so any other stick's data will be passed over. */
	/* if there is an eleventh stick on the chain, we will get a memory */
	/* error.  the wait routine DOES NOT KNOW how big your array is. */
	/* it blindly checks the index in the array for a pointer */
	/* 10 of everything ought to be safe for this demo. */
	sticks[0]=&stick1;

  	/* clear out the data structure */
	/* set some default congigs */
  	BS_InitStickData(sticks[0]);

 	/* set up our graphics folios */
	/* and load our data and all that for */
	/* this little demo (a derivative of Peter Broadwell's "daisy" demo) */
	setupFolios();

	/* connects us to the event broker and */
	/* configures it to be in observer mode */
	if(!BS_ConnectEventBroker())
	{
		printf("joystick example:  could not connect to event broker.\n");
		exit(0);
	}

	/* prints up a tally of sticks on the */
	/* serial port.  also turns on listening */
	/* for the ones that are there */
	handlePortChange();

	/* The Watch function below is commented out */
	/* because it is done in handlePortChange() */
	/* but I wanted it here to make the order of */
	/* operations more visible. */

	/* OK, we want joystick info */
	/* the 0 means "don't debounce all buttons" */
	/* i.e. subscribe to both button up as well */
	/* as button down events */
	/* BS_WatchJoyStick(0); */


	while (cont)
	{
		/* waits for message from event broker saying we got */
		/* info on a device we are interested in */
		/* the eCode has bits turned on for the devices */
		/* that produced events in this go-round */
		/*  */
		/* we are passing in our arrays of device data structure pointers */
		/* to be filled with event data. */
		/* the lightgun spot has a null pointer */
		/* because we are never calling a Watch for it and */
		/* we are not interested in it. */
		if((eCode=BS_NiceWaitEvent(sticks,NULL,NULL,NULL))>=0)
		{
			if(eCode)
			{
				/* we popped out of WaitSignal() */
				/* for some signal other than an event */
				/* broker message.  what could it be? */
				/* was it one you alloced and are using */
				/* for something else?  if so, you still */
				/* have access to it. */
				if(eCode&BS_OTHER_SIGNAL)
				{
					handleOtherSignal(BS_GetOtherSignals());
				}

				/* the device daisy chain */
				/* has changed. */
				if(eCode&BS_PORT_CHANGE)
				{
					handlePortChange();
				}

				/* This is a bit gross and this is the WRONG PLACE TO DO THIS */
				/* you DON'T want to do this in your main event loop */
				/* sorry, but this was added late and had to be in somewhere */
				/* so that you'd know about the StickCapability bits. */
				/* StickCapability is two bits */
				/* Stick4Way: */
				/*    does this stick have the 4-way switch and Z throttle */
				/* StickTurbulence: */
				/*    does this stick have "turbulence" motors to make it "buck" */
				/* these things are not on the basic stick. */
				/* the 4way switch and z throttle are on the extended stick. */
				/* a turbulence stick is not currently available */
				/* this demo wants both of these. */
				if(!(BS_GetStickRawButtons(sticks[0])&Stick4Way))
				{
					printf("bs_example: error -- not an extended stick!\n");
					exit(0);
				}

				/* process, filter, debounce joystick data */
				BS_ProcessStickData(
					/* process data from this stick */
					sticks[0],
					/* debounce these buttons */
					sDebounceBits,
					/* recalibrate if we hit the B button on the stick */
					(int)(BS_GetStickButtons(sticks[0])&StickB));

				/* move/reshape our quad */
				/* and maybe clear the screen */
				/* or switch the device we're */
				/* listening to */
				/* based on the device buttons and */
				/* positions */
				cont=updateCel(
					BS_GetStickX(sticks[0]),
					BS_GetStickY(sticks[0]),
					BS_GetStickZ(sticks[0]),
					BS_GetStickButtons(sticks[0]),
					quad,&fireF,&clearF);
			}

			/* map the cel onto the quad */
			/* and redraw */
			drawIt(quad,fireF,clearF);

		}
		else
			printf("eCode=%d\n",eCode);
	}

	/* shut down the event broker like */
	/* the nice hacker you are */
	BS_DisconnectBroker();

	printf("Bye.\n");

	/* lllaaaater, dude. */
	exit(0);
}

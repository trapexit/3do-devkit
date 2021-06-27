
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bs_example.c,v 1.10 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/bs_example
|||	bs_example - Peripheral example program.
|||
|||	  Synopsis
|||
|||	    bs_example
|||
|||	  Description
|||
|||	    This demonstration program lets you move a cel (a cloud) around the screen
|||	    with a joystick and leave an image of it on the screen when you press the
|||	    trigger. You can also distort the cel and clear the screen. The program
|||	    supports control pad, joystick, and mouse interaction.
|||
|||	    This example illustrates how to use the broker shell peripheral
|||	    interaction utilities.
|||
|||	    The connection/configuration/disconnection routines can be used as-is in
|||	    final code.
|||
|||	    The data interaction/processing is less efficient than it could be for the
|||	    sake of modularity, comprehensibility, and flexibility. If you unpack the
|||	    BS_NiceWaitEvent() and BS_ProcessXxxData() routines and perform that
|||	    processing in a more direct way performance will improve. You should also
|||	    eliminate any debouncing checks that you don't use.
|||
|||	    Joystick controls Stickmove in x and yZ zoom in/out U/D/L/R switchdeform
|||	    cel Triggerleave copy of cel on screen Aclear screen B recalibrate stick
|||	    (to recalibrate, hit B with stick in neutral position and move the stick
|||	    and z throttle to the limits of travel) Cquit RightShiftswitch to mouse
|||
|||	    Mouse controls LeftButtonleave copy of cel on screen MiddleButtonclear
|||	    screen Movementmove cel in x and y
|||
|||	    Pad 1 controls (used with mouse): RightButtonswitch to joystick D Pad
|||	    distort cel C Buttonquit RightShifttoggle mouse edge conditions between
|||	    bound & wrap
|||
|||	    Pad 2 controls (only if there are 2 pads and no mouse): D Padmove cel A
|||	    Buttonleave copy of cel on screen B Buttonclear screen C Buttonswitch to
|||	    joystick control RightShifttoggle movement speed between high & low
|||
|||	  Caveats
|||
|||	    Lightgun support is not yet included in this example.
|||
|||	  Associated Files
|||
|||	    cloud.cel
|||
|||	  Location
|||
|||	    examples/EventBroker/Control_Port_Peripherals
|||
**/

#if 0
The basic workings of the broker shell are as follows:

/* make a communications connection to the event broker */
BS_ConnectBroker();

/* listen for the devices you want */
/* events from */
BS_WatchCPad(); and or
BS_WatchJoyStick(); and or
BS_WatchMouse(); and or
BS_WatchLGun(); /* NOT WORKING NOW!! */

/* you can interleave Ignore and Watch calls */
/* to pay attention to whatever you want at a given */
/* time.with  */
BS_IgnoreCPad() and BS_IgnoreLGun() and
   BS_IgnoreMouse() and BS_IgnoreJoyStick()

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
BS_WatchingJoyStick(); and/or
BS_WatchingCPad(); and/or
BS_WatchingMouse(); and/or
BS_WatchingLightGun();

/* with your received data you can */
/* debounce, filter, fold mutilate, */
/* and spindle it with */
BS_ProcessJoyStickData(); and or
BS_ProcessCPadData(); and or
BS_ProcessMouseData(); and or
BS_ProcessLGunData(); /* NOT WORKING!! */

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
ubyte gMouseF=0;
ubyte gFakeF=0;

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

	if (!CreateBasicDisplay(mySc,DI_TYPE_NTSC,2))
		{
		DIAGNOSTIC("Cannot initialize graphics");
		}
	else retval |= GRAPHICSMASK;

	if ((gBackPic = (ubyte *)AllocMem( (int)(mySc->sc_nFrameByteCount),
			GETBANKBITS( GrafBase->gf_ZeroPage )
			| MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL)) == NULL) {
				DIAGNOSTIC ( "unable to allocate gBackPic" );
	}
	SetVRAMPages( VRAMIOReq, gBackPic, 0, mySc->sc_nFrameBufferPages, -1 );

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
	int32 mx,int32 my,
	uint32 mBits,
	uint32 cpBits,
	Point quad[4],
	int *fireF,
	int *clearF)
{
  static int32  dx = 0, dy = 0;

	if(gMouseF)
	{
		if(cpBits&ControlRight)
			dx+=2;
		if(cpBits&ControlLeft)
			dx-=2;

		if(cpBits&ControlUp)
			dy+=2;
		if(cpBits&ControlDown)
			dy-=2;

		quad[0].pt_X = mx + 0;		quad[0].pt_Y = my + 0;
		quad[1].pt_X = mx + BS_ZRES/2 + dx;	quad[1].pt_Y = my + 0;
		quad[2].pt_X = mx + BS_ZRES/2 + dx;	quad[2].pt_Y = my + BS_ZRES/2 + dy;
		quad[3].pt_X = mx + 0;		quad[3].pt_Y = my + BS_ZRES/2 + dy;

		/* if we're faking the mouse with a control pad */
		if(gFakeF)
		{
			*fireF=(int)(mBits&ControlA)&&1;
			*clearF=(int)(mBits&ControlB)&&1;
			if(mBits&ControlC)
			{
				if(BS_GetPeripheralCount(BS_STICK))
					gMouseF=0;
				else
					printf("peripheral example:  no joystick to give control to.\n");
			}
		}
		else
		{
			*fireF=(int)(mBits&MouseLeft)&&1;
			*clearF=(int)(mBits&MouseMiddle)&&1;
			if(mBits&MouseRight)
			{
				if(BS_GetPeripheralCount(BS_STICK))
					gMouseF=0;
				else
					printf("peripheral example:  no joystick to give control to.\n");
			}
		}

		if(cpBits&ControlC)
			return 0;

		return 1;
	}

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

	if(buttonBits&StickRightShift)
	{
		if((BS_GetPeripheralCount(BS_MOUSE)&&BS_GetPeripheralCount(BS_CPAD))||
		   (BS_GetPeripheralCount(BS_CPAD)>1))
			gMouseF=1;
		else
			printf("peripheral example: no mouse to give control to.\n");
	}

	if(buttonBits&StickC)
		return 0;
	return 1;
}

static void
handleOtherSignal(uint32 sigs)
{
	printf("peripheral example: recieved signal(s): 0x%x\n",sigs);
}

static void
handlePortChange(void)
{
  int32 cp,js,m;

	printf("peripheral example sez -- there are now:\n");
	printf("     %d control pad(s)\n",cp=BS_GetPeripheralCount(BS_CPAD));
	printf("     %d joy stick(s)\n",js=BS_GetPeripheralCount(BS_STICK));
	printf("     %d mouse/mice\n",m=BS_GetPeripheralCount(BS_MOUSE));
	printf("     %d light gun(s)\n",BS_GetPeripheralCount(BS_LGUN));

	/* pay attention to what is connected */
	/* you can interleave watch and ignore commands as */
	/* often as you like.  calling 2 ignore commands in a row */
	/* for the same device does not harm anything and */
	/* it only takes one watch command to see it again (the ignores don't stack) */
	/*  */
	/* you can also reconfigure your watching (i.e. call BS_WatchMouse(1) after */
	/* you've already called BS_WatchMouse(0) to get button up events too) */
	/* without having to call an ignore in the middle. */
	/* if you call the two watches to the same device with the same argument */
	/* there are no side effects. */
	if(m)
	{
		BS_WatchMouse(0);
		gFakeF=0;
	}
	else
	{
		gMouseF=0;
		BS_IgnoreMouse();
	}

	if(cp)
	{
		BS_WatchCPad(0);

		/* if we have 2 control pads and no mice */
		/* fake the mouse by using axis emulation */
		/* in the second control pad */
		if((!m)&&(cp>1))
			gFakeF=1;
	}
	else
	{
		gMouseF=0;
		gFakeF=0;
		BS_IgnoreCPad();
	}

	if(js)
		BS_WatchJoyStick(0);
	else
	{
		/* default to mouse control even */
		/* if there is no mouse */
		/* this is for the Capability check */
		/* in the main loop (see below) */
		gMouseF=1;
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

  /* Mouse and Stick Data storage */
  BS_MouseData mouse1;
  BS_StickData stick1;
  BS_CPadData pad1,pad2;

  /* 10 of these ought to be safe. */
  BS_MouseData *mice[10];
  BS_StickData *sticks[10];
  BS_CPadData  *pads[10];

  /* Debouncing choices */
  uint32 sDebounceBits=StickA|StickB|StickC|StickFire;
  uint32 mDebounceBits=MouseLeft|MouseRight|MouseMiddle;

  /* another silly flag */
  int cont=1;

  	/* Luuuuucy, you got some 'SPLAINING to dooo...	 */
	printf("peripheral module demo\n");
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
	printf("     RightShift: switch to mouse\n");
	printf("\nMouse controls:\n");
	printf("     LeftButton: leave copy of cel on screen\n");
	printf("     MiddleButton: clear screen\n");
	printf("     Movement:  move cel in x and y\n");
	printf("\nPad 1 controls (used with mouse):\n");
	printf("     RightButton:  switch to joystick\n");
	printf("     D Pad: deform cel\n");
	printf("     C Button: quit\n");
	printf("     RightShift:  toggle mouse edge conditions between bound & wrap\n");
	printf("\nPad 2 controls (only if there are 2 pads and no mice):\n");
	printf("     D Pad:  move cel\n");
	printf("     A Button: leave copy of cel on screen\n");
	printf("     B Button: clear screen\n");
	printf("     C Button: switch to joystick control\n");
	printf("     RightShift: toggle movement speed between high & low\n");


    /* set the pointers in the arrays all to NULL */
	/* this is ABSOLUTELY NECESSARY */
  	memset(mice,0,sizeof(mice));
	memset(sticks,0,sizeof(sticks));
	memset(pads,0,sizeof(pads));

	/* we are only interested in the first mouse and first pad */
	/* or the first joystick */
	/* this is a device-specific index, not */
	/* daisy-chain spot specific */
	/* the BS_NiceWaitEvent() call */
	/* will put data from the first mouse into the structure */
	/* pointed to by the first entry in the mice array, data from */
	/* the second mouse will go into the second entry and so on. */
	/* if a null array pointer is passed, then mouse events will be */
	/* passed over.  this is not the same as ignoring because the messages */
	/* are still getting received from the broker.  it is more efficient */
	/* to use an Ignore call for a device you don't want to hear from. */
	/* if a null pointer is found in the structure pointer array, then */
	/* data from that numbered device is passed over.  i.e. we have put */
	/* a valid pointer in the first slot of ''mice'' so the first mouse's data */
	/* will go there.  there is a null pointer in the second through tenth */
	/* slots so any other mouse's data will be passed over. */
	/* if there is an eleventh mouse on the chain, we will get a memory */
	/* error.  the wait routine DOES NOT KNOW how big your array is. */
	/* it blindly checks the index in the array for a pointer */
	/* 10 of everything ought to be safe for this demo. */
	mice[0]=&mouse1;
	sticks[0]=&stick1;
	pads[0]=&pad1;

  	/* clear out the data structure */
	/* set some default congigs */
  	BS_InitStickData(sticks[0]);
	BS_InitMouseData(mice[0]);
	BS_InitCPadData(pads[0]);
	BS_InitCPadData(&pad2);

	BS_SetMouseX(mice[0],BS_XRES/2);
	BS_SetMouseY(mice[0],BS_YRES/2);
	BS_SetCPadX(&pad2,BS_XRES/2);
	BS_SetCPadY(&pad2,BS_YRES/2);

 	/* set up our graphics folios */
	/* and load our data and all that for */
	/* this little demo (a derivative of Peter Broadwell's "daisy" demo) */
	setupFolios();

	/* connects us to the event broker and */
	/* configures it to be in observer mode */
	if(!BS_ConnectEventBroker())
	{
		printf("peripheral example:  could not connect to event broker.\n");
		exit(0);
	}

	/* prints up a tally of devices on the */
	/* serial port.  also turns on listening */
	/* for the ones that are there */
	handlePortChange();

	/* use second pad as fake mouse */
	/* if there is a second pad and no mouse */
	/* when calling BS_ProcessCPadData() for this pad, the third argument, */
	/* axisEmulF, will be 1. */
	if(gFakeF)
		pads[1]=&pad2;

	/* The Watch functions below are commented out */
	/* because they are done in handlePortChange() */
	/* but I wanted them here to make the order of */
	/* operations more visible. */

	/* OK, we want joystick info */
	/* the 0 means "don't debounce all buttons" */
	/* i.e. subscribe to both button up as well */
	/* as button down events */
	/* BS_WatchJoyStick(0); */

	/* And we want mouse info */
	/* Same here as above. */
	/* you can call the "Watch" functions */
	/* later on with 1 to make the event broker */
	/* only pass on button down events for this device */
	/* calling the "watch" routines with a negative value */
	/* means to quit watching these devices. */
	/* the "Ignore" calls are just macros to the "watch" */
	/* functions with an arg of -1 */
	/* BS_WatchMouse(0); */

	/* same here */
	/* BS_WatchCPad(0); */

	while (cont)
	{
		/* waits for message from event broker saying we got  */
		/* info on a device we are interested in */
		/* the eCode has bits turned on for the devices */
		/* that produced events in this go-round */
		/*  */
		/* we are passing in our arrays of device data structure pointers */
		/* to be filled with event data. */
		/* the lightgun spot has a null pointer */
		/* because we are never calling a Watch for it and */
		/* we are not interested in it. */
		if((eCode=BS_NiceWaitEvent(sticks,pads,mice,NULL))>=0)
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
					/* are we using a cpad to imitate the mouse? */
					if(gFakeF)
					{
						if(!pads[1])
							pads[1]=&pad2;
					}
					else
					{
						if(pads[1])
							pads[1]=NULL;
					}
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
				if(!gMouseF&&!((BS_GetStickRawButtons(sticks[0])&Stick4Way)))
				{
					printf("bs_example: error -- not an extended stick!\n");
					exit(0);
				}

				if(gMouseF)
				{
					if(gFakeF)
					{
						/* use control pad to fake the mouse x,y movement */
						BS_ProcessCPadData(pads[1],
							ControlA|ControlB|ControlC|ControlRightShift,
							1);
						if(BS_GetCPadButtons(pads[1])&ControlRightShift)
						{
							if(BS_GetCPadDX(pads[1])==2)
							{
								BS_SetCPadDX(pads[1],1);
								BS_SetCPadDY(pads[1],1);
							}
							else
							{
								BS_SetCPadDX(pads[1],2);
								BS_SetCPadDY(pads[1],2);
							}
						}
					}
					else
					{
						/* debounce and filter mouse data */
						BS_ProcessMouseData(
							/* process data from this mouse */
							mice[0],
							/* debounce these bits */
							mDebounceBits);
					}

					/* debounce the C button on the pad */
					BS_ProcessCPadData(pads[0],ControlC|ControlRightShift,0);

					if(BS_GetCPadButtons(pads[0])&ControlRightShift)
					{
						if(gFakeF)
							BS_SetCPadWrapF(pads[1],!BS_GetCPadWrapF(pads[1]));
						else
							BS_SetMouseWrapF(mice[0],BS_GetMouseWrapF(mice[0]));
					}
				}
				else
				{
					/* process, filter, debounce joystick data */
					BS_ProcessStickData(
						/* process data from this stick */
						sticks[0],
						/* debounce these buttons */
						sDebounceBits,
						/* recalibrate if we hit the B button on the stick */
						(int)(BS_GetStickButtons(sticks[0])&StickB));
				}

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
					(gFakeF?BS_GetCPadX(pads[1]):BS_GetMouseX(mice[0])),
					(gFakeF?BS_GetCPadY(pads[1]):BS_GetMouseY(mice[0])),
					(gFakeF?BS_GetCPadButtons(pads[1]):BS_GetMouseButtons(mice[0])),
					BS_GetCPadButtons(pads[0]),
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

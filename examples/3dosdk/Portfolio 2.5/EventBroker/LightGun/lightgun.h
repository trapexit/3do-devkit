
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: lightgun.h,v 1.3 1994/11/19 02:08:27 mattm Exp $
**
**  Constants, Data Structures, and external function declerations for
**  the lightgun dropin code.
**
******************************************************************************/


#include "item.h"
#include "msgport.h"
#include "event.h"
#include "init3do.h"

/***** Constants *****/

#define	LG_SUCCESS				0
#define	LG_ERROR				-1

#define LG_CALIBRATION_BUTTON	0x48000000

/* In order to provide the necessary precision, DEFAULT_YSCANTIME is increased by a 	 */
/* multiple of 10.  This is taken into considertation when calculating x and y positions */

/* The default values for a PAL display will be added at a later date */

#define	NTSC_DEFAULT_XSCANTIME		1030
#define NTSC_DEFAULT_YSCANTIME		12707
#define NTSC_DEFAULT_TIMEOFFSET		-12835

#define PAL_DEFAULT_XSCANTIME		1051
#define PAL_DEFAULT_YSCANTIME		12796
#define PAL_DEFAULT_TIMEOFFSET		-58995

#define PAL_WIDTH					388
#define NTSC_WIDTH					320

#define EVENT_MSGPORTITEM_NAME		"EventPort"

#define LG_CONTROLPAD_EVENT			0x01
#define LG_LIGHTGUN_EVENT			0x02


/***** Data Structures *****/

typedef struct {
	LightGunEventData	eventData;
	int32				x, y;
} LG_EventData;


/***** External Function Declerations *****/

extern int32 LG_ConnectEventBroker(MsgPort **msgPortPtr,
		Item *msgPortItemPtr, Item *msgItemPtr, uint32 deviceFlags);
extern int32 LG_WaitEvent(ScreenContext *screen, MsgPort *msgPort,
		Item msgPortItem, Item msgItem, LG_EventData *lgd,
		ControlPadEventData *cpd);
extern int32 LG_CalibrateGun(ScreenContext *screen, MsgPort *msgPort,
		Item msgPortItem, Item msgItem, int32 x, int32 y);

#ifndef __DEVICE_H
#define __DEVICE_H

/*****

$Id: device.h,v 1.8 1994/01/21 02:06:25 limes Exp $

$Log: device.h,v $
 * Revision 1.8  1994/01/21  02:06:25  limes
 * recover from RCS bobble:
 *
 * +  * Revision 1.9  1994/01/20  01:52:26  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.8  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.9  1994/01/20  01:52:26  sdas
 * C++ compatibility - updated
 *
 * Revision 1.8  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.7  1993/08/13  03:42:25  dale
 * Find convenience macro
 *
 * Revision 1.6  1993/06/09  01:25:57  andy
 * Added TimerDevice typedev
 *
 * Revision 1.5  1993/06/09  00:43:15  andy
 * Added reserved field to driver to pad it to 16 byte multiple.
 *
 * Revision 1.4  1993/04/02  01:11:29  dale
 * needs kernelnodes.h now
 *
 * Revision 1.3  1993/04/01  04:50:38  dale
 * new device OpenNamedDevice
 *
 * Revision 1.2  1993/03/26  18:27:37  dale
 * dev_DeleteDev
 *
 * Revision 1.1  1993/03/16  06:52:45  dale
 * Initial revision
 *
 * Revision 1.11  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.10  1993/02/11  04:58:58  dale
 * name update for M6
 *
 * Revision 1.9  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.8  1992/12/08  10:01:02  dale
 * new devInit entry for device initialization.`
 *
 * Revision 1.7  1992/10/24  01:10:34  dale
 * rcs again
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "nodes.h"
#include "list.h"
#include "kernelnodes.h"

typedef struct Device
{
	ItemNode	dev;
	struct Driver *dev_Driver;
	int32	dev_OpenCnt;
				/* device dependent initialization */
	int32	(*dev_CreateIOReq)(struct IOReq *iorP);
	int32	(*dev_DeleteIOReq)(struct IOReq *iorP);
	int32	(*dev_Open)(struct Device *devP);	/* called when Opened */
	void	(*dev_Close)(struct Device *devP);	/* called when Closed */
	int32	dev_IOReqSize;	/* size of ioreqs for this device */
	List	dev_IOReqs;	/* ioreqs created for this device */
	uint8	dev_MaxUnitNum;	/* maximum allowed unit for this device */
	uint8	dev_Reserved[3];
	int32	(*dev_Init)(struct Device *devP);	/* called at creation */
	int32	(*dev_DeleteDev)(struct Device *devP);	/* called before deleting this dev */
	uint32	dev_Reserved1;
} Device;

enum device_tags
{
	CREATEDEVICE_TAG_DRVR = TAG_ITEM_LAST+1,
	CREATEDEVICE_TAG_CRIO,		/* createIO routine */
	CREATEDEVICE_TAG_DLIO,		/* deleteIO routine */
	CREATEDEVICE_TAG_OPEN,		/* open routine */
	CREATEDEVICE_TAG_CLOSE,		/* close routine */
	CREATEDEVICE_TAG_IOREQSZ,	/* optional request size */
	CREATEDEVICE_TAG_INIT		/* InitRoutine to call */
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern Item CreateDevice(char *name, uint8 pri, Item driver);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define DeleteDevice(x)	DeleteItem(x)

/* easy def for openning device without regard to version */
#define FindDevice(n)     FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define OpenNamedDevice(x,a)	OpenItem(FindDevice(x),a)


typedef struct TimerDevice
{
	Device	timerdev_dev;
	uint32  timerdev_VBlankCountOverFlow;
	uint32  timerdev_VBlankCount;
} TimerDevice;

#endif	/* __DEVICE_H */

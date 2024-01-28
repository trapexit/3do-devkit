#ifndef __DRIVER_H
#define __DRIVER_H

/*****

$Id: driver.h,v 1.12 1994/01/21 02:07:52 limes Exp $

$Log: driver.h,v $
 * Revision 1.12  1994/01/21  02:07:52  limes
 * recover from rcs bobble:
 *
 * +  * Revision 1.13  1994/01/20  01:53:26  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.12  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.13  1994/01/20  01:53:26  sdas
 * C++ compatibility - updated
 *
 * Revision 1.12  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.11  1993/12/04  00:21:47  limes
 * "storage-class without declarator is spurious": had a typedef for
 * a structure declaration but not the name of the type being defined.
 *
 * Revision 1.10  1993/11/24  05:27:27  limes
 * The line "struct IOReq;" may fool the current compiler into silence,
 * but it is not legal C according to at least one other compiler (funny,
 * it seems legal to me). Anyway, this can be expanded to "typedef struct
 * IOReq IOReq" as long as we coordinate with io.h properly. Failure to
 * do this causes the compiler to bitch about, get this, implicit cast
 * from one pointer type to an *unequal* pointer type. Isn't that what
 * ANSI prototypes are for, so the compiler knows and can do the right
 * thing?
 *
 * Revision 1.9  1993/08/13  03:42:25  dale
 * Find convenience macro
 *
 * Revision 1.8  1993/06/23  02:36:38  andy
 * fixed yp the Family Code and Usage comments.
 *
 * Revision 1.7  1993/06/14  07:05:39  andy
 * explicit definition of DI_OTHER
 *
 * Revision 1.6  1993/06/09  04:20:39  andy
 * Added Media Change Counter
 *
 * Revision 1.5  1993/06/09  00:43:15  andy
 * added new fields to DeviceStatus...split usage flags and family codes out.
 *
 * Revision 1.4  1993/06/02  21:14:20  dplatt
 * Add standard definitions for bits in DeviceStatus.ds_DeviceFlagWord;
 * add driver definitions for DeviceStatus.ds_DriverIdentity.
 *
 * Revision 1.3  1993/03/18  23:59:30  dale
 * new flag def
 *
 * Revision 1.2  1993/03/18  00:12:20  dale
 * new Xbus Status return structure
 *
 * Revision 1.1  1993/03/16  06:51:51  dale
 * Initial revision
 *
 * Revision 1.11  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.11  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.10  1992/12/23  01:05:41  dale
 * better definition for DeviceStatus
 *
 * Revision 1.9  1992/12/22  07:33:29  dale
 * new field in CmdStatus
 *
 * Revision 1.8  1992/11/22  21:28:44  dale
 * fix warnings
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
#include "msgport.h"

#ifndef	IOReq_typedef
#define	IOReq_typedef
typedef struct IOReq IOReq;
#endif

typedef struct Driver
{
	ItemNode	drv;
	MsgPort	*drv_MsgPort;
	int32	drv_OpenCnt;
	void	(*drv_AbortIO)(struct IOReq *iorP);
	int32	(*drv_DispatchIO)(struct IOReq *iorP);
	Item	(*drv_Init)(struct Driver *drvP); /* change to device init */
	int32	(**drv_CmdTable)(struct IOReq *iorP);
	uint8	drv_MaxCommands;
	uint8	drv_pad[3];
} Driver;

enum driver_tags
{
	CREATEDRIVER_TAG_ABORTIO = TAG_ITEM_LAST+1,
	CREATEDRIVER_TAG_MAXCMDS,
	CREATEDRIVER_TAG_CMDTABLE,
	CREATEDRIVER_TAG_MSGPORT,
	CREATEDRIVER_TAG_INIT,
	CREATEDRIVER_TAG_DISPATCH
};

 
#ifdef __cplusplus
extern "C" {
#endif

extern Item CreateDriver(char *name, uint8 pri, Item (*init)());

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#define DeleteDriver(x)	DeleteItem(x)

/* the location of the Device Status structure may move yet! */

/* This is the version-0 form of this structure.  Later versions may
   have additional fields. */

typedef struct DeviceStatus
{
        uint8   ds_DriverIdentity;
	uint8   ds_DriverStatusVersion;
	uint8	ds_FamilyCode;
	uint8   ds_headerPad;
	uint32  ds_MaximumStatusSize;
	uint32  ds_DeviceBlockSize;
	uint32  ds_DeviceBlockCount;
	uint32  ds_DeviceFlagWord;
	uint32	ds_DeviceUsageFlags;
	uint32  ds_DeviceLastErrorCode;
	uint32	ds_DeviceMediaChangeCntr;
	uint32  ds_Reserved;
} DeviceStatus;

/*
  Values for ds_DriverIdentity
*/

#define DI_OTHER	       0x00

#define DI_XBUS                0x01
#define DI_TIMER               0x02
#define DI_RAM                 0x03
#define DI_CONTROLPORT         0x04
#define DI_FILE                0x05
#define DI_MEI_CDROM           0x06
#define DI_SPORT               0x07
#define DI_HOLLYWOOD           0x08


/*
  Values for ds_FamilyCode.  This byte field contains the family
  identifier for this device.
*/

#define DS_DEVTYPE_OTHER       0

#define DS_DEVTYPE_CDROM       1
#define DS_DEVTYPE_HARDDISK    2
#define DS_DEVTYPE_FILE        3
#define DS_DEVTYPE_TAPE        4
#define DS_DEVTYPE_MODEM       5
#define DS_DEVTYPE_NETWORK     6
#define DS_DEVTYPE_CODEC       7
 

/*
  Values for ds_DeviceUsageFlags.  This field contains device 
  characteristic/usage information.  (NOTE: ds_DeviceFlagWord
  is for driver specific flag usage.)
*/

#define DS_USAGE_FILESYSTEM    0x80000000
#define DS_USAGE_PRIVACCESS    0x40000000
#define DS_USAGE_READONLY      0x20000000
#define DS_USAGE_OFFLINE       0x10000000
 
#define DS_USAGE_FAST          0x00000002
#define DS_USAGE_SLOW          0x00000001

typedef struct RamDeviceStatus
{
	DeviceStatus ramdev_ds;
	uint32		ramdev_DeviceAddress;	/* physical start of ramdisk */
} RamDeviceStatus;

typedef struct XBusDeviceStatus
{
	DeviceStatus	xbus_ds;
	uint32		xbus_ManuIdNum;
	uint32		xbus_ManuDevNum;
	uint32		xbus_ManuRevNum;
	uint32		xbus_ManuFlags;
	uint8		xbus_Flags;
	uint8		reserved[3];
} XBusDeviceStatus;

/* xbus_Flags */
#define	XBUS_OLDSTAT	1	/* status returned without prepending cmd byte */
#define XBUS_OLDMEI	2	/* ancient pre-ces poll register */
#define XBUS_ROMDRIVER	4	/* driver downloaded from device */
#define XBUS_DEVDEAD	0x80	/* this device is not active */

/* ManuFlags */
#define DEVHAS_DRIVER	0x100	/* Driver downloaded from Device rom */

#define FindDriver(n)     FindNamedItem(MKNODEID(KERNELNODE,DRIVERNODE),(n))

#endif	/* __DRIVER_H */

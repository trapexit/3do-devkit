#ifndef __DRIVER_H
#define __DRIVER_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: driver.h,v 1.25 1994/11/01 19:04:42 bungee Exp $
**
**  Kernel driver management definitions
**
******************************************************************************/


#include "types.h"
#include "nodes.h"
#include "msgport.h"

#ifndef	IOReq_typedef
#define	IOReq_typedef
typedef struct IOReq IOReq;
#endif


typedef struct Driver
{
	ItemNode drv;
	uint32   drv_Private;
	int32    drv_OpenCnt;
} Driver;




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
#define DI_LCCD_CDROM          0x09


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



/*****************************************************************************/


#endif	/* __DRIVER_H */

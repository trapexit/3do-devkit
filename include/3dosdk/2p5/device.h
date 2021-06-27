#ifndef __DEVICE_H
#define __DEVICE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: device.h,v 1.20 1994/12/09 21:22:41 vertex Exp $
**
**  Kernel device management definitions
**
******************************************************************************/


#include "types.h"
#include "item.h"
#include "nodes.h"
#include "list.h"
#include "kernelnodes.h"


typedef struct Device
{
        ItemNode       dev;
        struct Driver *dev_Driver;
        int32          dev_OpenCnt;
        uint32         dev_Private0[13];
        uint8          dev_MaxUnitNum;  /* maximum allowed unit for this device */
        uint8          dev_Private1[3];
        uint32         dev_Private2[3];
} Device;


/* easy def for opening device without regard to version */
#define FindDevice(n)		FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define FindAndOpenDevice(n)	FindAndOpenNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define OpenNamedDevice(n,a)	FindAndOpenNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define CloseNamedDevice(dev)	CloseItem(dev)


/*****************************************************************************/


#endif	/* __DEVICE_H */

/* $Id: hardware.h,v 1.10 1994/02/09 02:04:35 limes Exp $ */

/* *************************************************************************
 * 
 * Opera Hardware Definitions Include File
 * 
 * Copyright (C) 1992, New Technologies Group, Inc.
 * Confidential and Proprietary  -  All Rights Reserved
 * 
 * This file works with any tab space from 1 to 8.
 * 
 * HISTORY
 * Date   Author           Description
 * ------ ---------------- -------------------------------------------------
 * 930228 -RJ              Added SKIPX def's to cel preamble
 * 921204 -RJ Mical        Burst this file out of sherrie.h
 * 
 * ********************************************************************** */

#ifndef __HARDWARE_H
#define __HARDWARE_H

#include "types.h"

/* === VDL DMA control === */
/* Bit fields 0xF8000000 are reserved */
#define VDL_640SC         0x04000000
#define VDL_DISPMOD_MASK  0x03800000
#define VDL_SLIPEN        0x00400000
#define VDL_ENVIDDMA      0x00200000
#define VDL_SLIPCOMMSEL   0x00100000
#define VDL_480RES        0x00080000
#define VDL_RELSEL        0x00040000
#define VDL_PREVSEL       0x00020000
#define VDL_LDCUR         0x00010000
#define VDL_LDPREV        0x00008000
#define VDL_LEN_MASK      0x00007E00
#define VDL_LINE_MASK     0x000001FF

#define VDL_LINE_SHIFT     0
#define VDL_LEN_SHIFT      9

#define VDL_LEN_PREFETCH   4

/* VDL_DISPMOD_MASK definitions */
#define VDL_DISPMOD_320   0x00000000
#define VDL_DISPMOD_384   0x00800000
#define VDL_DISPMOD_512   0x01000000
#define VDL_DISPMOD_640   0x01800000
#define VDL_DISPMOD_1024  0x02000000
#define VDL_DISPMOD_res5  0x02800000
#define VDL_DISPMOD_res6  0x03000000
#define VDL_DISPMOD_res7  0x03800000


/* === VDL Palette data === */
#define VDL_CONTROL     0x80000000
#define VDL_RGBCTL_MASK 0x60000000
#define VDL_PEN_MASK    0x1F000000
#define VDL_R_MASK      0x00FF0000
#define VDL_G_MASK      0x0000FF00
#define VDL_B_MASK      0x000000FF

#define VDL_B_SHIFT       0
#define VDL_G_SHIFT       8
#define VDL_R_SHIFT       16
#define VDL_PEN_SHIFT     24
#define VDL_RGBSEL_SHIFT  29

/* VDL_RGBCTL_MASK definitions */
#define VDL_FULLRGB     0x00000000
#define VDL_REDONLY     0x60000000
#define VDL_GREENONLY   0x40000000
#define VDL_BLUEONLY    0x20000000


/* === VDL display control word === */
#define VDL_DISPCTRL      0xC0000000
#define VDL_BACKGROUND    0x20000000
#define VDL_NULLAMY       0x10000000
#define VDL_PALSEL        0x08000000
#define VDL_S640SEL       0x04000000
#define VDL_CLUTBYPASSEN  0x02000000
#define VDL_SLPDCEL       0x01000000
#define VDL_FORCETRANS    0x00800000
#define VDL_BACKTRANS     0x00400000
#define VDL_WINSWAPHV     0x00200000
#define VDL_WINVSUB_MASK  0x00180000 /* See definitions below */
#define VDL_WINHSUB_MASK  0x00060000 /* See definitions below */
#define VDL_WINBLSB_MASK  0x00018000 /* See definitions below */
#define VDL_WINVINTEN     0x00004000
#define VDL_WINHINTEN     0x00002000
#define VDL_RANDOMEN      0x00001000
#define VDL_WINREPEN      0x00000800
#define VDL_SWAPHV        0x00000400
#define VDL_VSUB_MASK     0x00000300 /* See definitions below */
#define VDL_HSUB_MASK     0x000000C0 /* See definitions below */
#define VDL_BLSB_MASK     0x00000030 /* See definitions below */
#define VDL_VINTEN        0x00000008
#define VDL_HINTEN        0x00000004
#define VDL_COLORSONLY    0x00000002
#define VDL_ONEVINTDIS    0x00000001

/* VDL_BLSB_MASK definitions */
#define VDL_BLSB_NOP    0x00000030
#define VDL_BLSB_BLUE   0x00000020 /* Normal */
#define VDL_BLSB_GREEN  0x00000010
#define VDL_BLSB_ZERO   0x00000000

/* VDL_HSUB_MASK definitions */
#define VDL_HSUB_NOP    0x000000C0
#define VDL_HSUB_FRAME  0x00000080 /* Normal */
#define VDL_HSUB_ONE    0x00000040
#define VDL_HSUB_ZERO   0x00000000

/* VDL_VSUB_MASK definitions */
#define VDL_VSUB_NOP    0x00000300
#define VDL_VSUB_FRAME  0x00000200 /* Normal */
#define VDL_VSUB_ONE    0x00000100
#define VDL_VSUB_ZERO   0x00000000

/* VDL_WBLSB_MASK definitions */
#define VDL_WINBLSB_NOP    0x00018000
#define VDL_WINBLSB_BLUE   0x00010000 /* Normal */
#define VDL_WINBLSB_GREEN  0x00008000
#define VDL_WINBLSB_ZERO   0x00000000

/* VDL_HSUB_MASK definitions */
#define VDL_WINHSUB_NOP    0x00060000
#define VDL_WINHSUB_FRAME  0x00040000 /* Normal */
#define VDL_WINHSUB_ONE    0x00020000
#define VDL_WINHSUB_ZERO   0x00000000

/* VDL_VSUB_MASK definitions */
#define VDL_WINVSUB_NOP    0x00180000
#define VDL_WINVSUB_FRAME  0x00100000 /* Normal */
#define VDL_WINVSUB_ONE    0x00080000
#define VDL_WINVSUB_ZERO   0x00000000


/* === AMY control word === */
#define VDL_AMYCTRL  0x80000000


/* === Special VDL 'NOP' === */
#define VDL_NOP      0xE1000000
#define VDL_NULLVDL  VDL_NOP
#define VDL_AMY_NOP  VDL_AMYCTRL+0
#define VDL_AMYNULL  VDL_AMY_NOP


/* === CCB control word flags === */
#define CCB_SKIP        0x80000000
#define CCB_LAST        0x40000000
#define CCB_NPABS       0x20000000
#define CCB_SPABS       0x10000000
#define CCB_PPABS       0x08000000
#define CCB_LDSIZE      0x04000000
#define CCB_LDPRS       0x02000000
#define CCB_LDPPMP      0x01000000
#define CCB_LDPLUT      0x00800000
#define CCB_CCBPRE      0x00400000
#define CCB_YOXY        0x00200000
#define CCB_ACSC        0x00100000
#define CCB_ALSC        0x00080000
#define CCB_ACW         0x00040000
#define CCB_ACCW        0x00020000
#define CCB_TWD         0x00010000
#define CCB_LCE         0x00008000
#define CCB_ACE         0x00004000
#define CCB_reserved13  0x00002000
#define CCB_MARIA       0x00001000
#define CCB_PXOR        0x00000800
#define CCB_USEAV       0x00000400
#define CCB_PACKED      0x00000200
#define CCB_POVER_MASK  0x00000180
#define CCB_PLUTPOS     0x00000040
#define CCB_BGND        0x00000020
#define CCB_NOBLK       0x00000010
#define CCB_PLUTA_MASK  0x0000000F

#define CCB_POVER_SHIFT  7
#define CCB_PLUTA_SHIFT  0

#define PMODE_PDC   ((0x00000000)<<CCB_POVER_SHIFT) /* Normal */
#define PMODE_ZERO  ((0x00000002)<<CCB_POVER_SHIFT)
#define PMODE_ONE   ((0x00000003)<<CCB_POVER_SHIFT)


/* === Cel first preamble word flags === */
#define PRE0_LITERAL    0x80000000
#define PRE0_BGND       0x40000000
#define PREO_reservedA  0x30000000
#define PRE0_SKIPX_MASK 0x0F000000
#define PREO_reservedB  0x00FF0000
#define PRE0_VCNT_MASK  0x0000FFC0
#define PREO_reservedC  0x00000020
#define PRE0_LINEAR     0x00000010
#define PRE0_REP8       0x00000008
#define PRE0_BPP_MASK   0x00000007

#define PRE0_SKIPX_SHIFT 24
#define PRE0_VCNT_SHIFT  6
#define PRE0_BPP_SHIFT   0

/* PRE0_BPP_MASK definitions */
#define PRE0_BPP_1   0x00000001
#define PRE0_BPP_2   0x00000002
#define PRE0_BPP_4   0x00000003
#define PRE0_BPP_6   0x00000004
#define PRE0_BPP_8   0x00000005
#define PRE0_BPP_16  0x00000006

/* Subtract this value from the actual vertical source line count */
#define PRE0_VCNT_PREFETCH    1


/* === Cel second preamble word flags === */
#define PRE1_WOFFSET8_MASK   0xFF000000
#define PRE1_WOFFSET10_MASK  0x03FF0000
#define PRE1_NOSWAP          0x00004000
#define PRE1_TLLSB_MASK      0x00003000
#define PRE1_LRFORM          0x00000800
#define PRE1_TLHPCNT_MASK    0x000007FF

#define PRE1_WOFFSET8_SHIFT   24
#define PRE1_WOFFSET10_SHIFT  16
#define PRE1_TLLSB_SHIFT      12
#define PRE1_TLHPCNT_SHIFT    0

#define PRE1_TLLSB_0     0x00000000
#define PRE1_TLLSB_PDC0  0x00001000 /* Normal */
#define PRE1_TLLSB_PDC4  0x00002000
#define PRE1_TLLSB_PDC5  0x00003000

/* Subtract this value from the actual word offset */
#define PRE1_WOFFSET_PREFETCH 2
/* Subtract this value from the actual pixel count */
#define PRE1_TLHPCNT_PREFETCH 1


/* === CECONTROL flags === */
#define B15POS_MASK   0xC0000000
#define B0POS_MASK    0x30000000
#define SWAPHV        0x08000000
#define ASCALL        0x04000000
#define CECONTROL_u25 0x02000000
#define CFBDSUB       0x01000000
#define CFBDLSB_MASK  0x00C00000
#define PDCLSB_MASK   0x00300000

#define B15POS_SHIFT 30
#define B0POS_SHIFT  28
#define CFBD_SHIFT   22
#define PDCLSB_SHIFT 20

/* B15POS_MASK definitions */
#define B15POS_0    0x00000000
#define B15POS_1    0x40000000
#define B15POS_PDC  0xC0000000

/* B0POS_MASK definitions */
#define B0POS_0     0x00000000
#define B0POS_1     0x10000000
#define B0POS_PPMP  0x20000000
#define B0POS_PDC   0x30000000

/* CFBDLSB_MASK definitions */
#define CFBDLSB_0      0x00000000
#define CFBDLSB_CFBD0  0x00400000
#define CFBDLSB_CFBD4  0x00800000
#define CFBDLSB_CFBD5  0x00C00000

/* PDCLSB_MASK definitions */
#define PDCLSB_0     0x00000000
#define PDCLSB_PDC0  0x00100000
#define PDCLSB_PDC4  0x00200000
#define PDCLSB_PDC5  0x00300000


/* === Packed cel data control tokens === */
#define PACK_EOL          0x00000000
#define PACK_LITERAL      0x00000001
#define PACK_TRANSPARENT  0x00000002
#define PACK_PACKED       0x00000003


/* === PPMPC control word flags === */
/* You compose a PPMP value by building up PPMPC definitions and then 
 * using the PPMP_0_SHIFT or PPMP_1_SHIFT values to build up the 
 * value to be used for the CCB's PPMP
 */

/* These define the shifts required to get your PPMPC value into either 
 * the 0 half or the 1 half of the PPMP
 */
#define PPMP_0_SHIFT 0
#define PPMP_1_SHIFT 16

#define PPMPC_1S_MASK  0x00008000
#define PPMPC_MS_MASK  0x00006000
#define PPMPC_MF_MASK  0x00001C00
#define PPMPC_SF_MASK  0x00000300
#define PPMPC_2S_MASK  0x000000C0
#define PPMPC_AV_MASK  0x0000003E
#define PPMPC_2D_MASK  0x00000001

#define PPMPC_MS_SHIFT  13
#define PPMPC_MF_SHIFT  10
#define PPMPC_SF_SHIFT  8
#define PPMPC_2S_SHIFT  6
#define PPMPC_AV_SHIFT  1

/* PPMPC_1S_MASK definitions */
#define PPMPC_1S_PDC   0x00000000
#define PPMPC_1S_CFBD  0x00008000

/* PPMPC_MS_MASK definitions */
#define PPMPC_MS_CCB         0x00000000
#define PPMPC_MS_PIN         0x00002000
#define PPMPC_MS_PDC_MFONLY  0x00004000
#define PPMPC_MS_PDC         0x00004000

/* PPMPC_MF_MASK definitions */
#define PPMPC_MF_1  0x00000000
#define PPMPC_MF_2  0x00000400
#define PPMPC_MF_3  0x00000800
#define PPMPC_MF_4  0x00000C00
#define PPMPC_MF_5  0x00001000
#define PPMPC_MF_6  0x00001400
#define PPMPC_MF_7  0x00001800
#define PPMPC_MF_8  0x00001C00

/* PPMPC_SF_MASK definitions */
#define PPMPC_SF_2   0x00000100
#define PPMPC_SF_4   0x00000200
#define PPMPC_SF_8   0x00000300
#define PPMPC_SF_16  0x00000000

/* PPMPC_2S_MASK definitions */
#define PPMPC_2S_0     0x00000000
#define PPMPC_2S_CCB   0x00000040
#define PPMPC_2S_CFBD  0x00000080
#define PPMPC_2S_PDC   0x000000C0

/* PPMPC_2D_MASK definitions */
#define PPMPC_2D_1  0x00000000
#define PPMPC_2D_2  0x00000001


/* === JOYSTICK/JOYSTICK1 flags === */
#define JOYSTART  0x00000080
#define JOYFIREC  0x00000040
#define JOYFIREA  0x00000020
#define JOYFIREB  0x00000010
#define JOYDOWN   0x00000008
#define JOYUP     0x00000004
#define JOYRIGHT  0x00000002
#define JOYLEFT   0x00000001

#define JOYSELECT JOYFIREC

#define JOYMOVE     (JOYLEFT+JOYRIGHT+JOYUP+JOYDOWN)
#define JOYBUTTONS  (JOYFIREA+JOYFIREB+JOYFIREC+JOYSTART)


 
#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

/* === Finally, a kernel call that uses the hardware (?) === */
uint32 __swi(KERNELSWI+17) ReadHardwareRandomNumber(void);

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* of #ifdef __HARDWARE_H */


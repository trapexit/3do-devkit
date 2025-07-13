#pragma once

/* === PIXC / PPMPC control word flags === */

/*
 * PPMP = Pen and Palette Manipulator Processor
 * PPMPC = Pen and Palette Manipulator Processor Control
 * PIXC = Pixel Processor Control
 * Like many things with Opera there are multiple names for the same
 * thing. PIXC and PPMPC both refer to the same CEL Engine
 * feature. PPMP was used earlier in the design of the hardware and
 * PIXC later. This header adds PIXC_ variants to all PPMP_ #define's
 * for consistency and keeps the PPMP_ ones for backward
 * compatibility.
 */

/*
 * You compose a PPMP value by building up PPMPC definitions and then
 * using the PPMP_0_SHIFT or PPMP_1_SHIFT values to build up the
 * value to be used for the CCB's PPMP
 */

/*
 * These define the shifts required to get your PPMPC value into either
 * the 0 half or the 1 half of the PPMP
 */

#define PPMP_0_SHIFT 0
#define PIXC_0_SHIFT PPMP_0_SHIFT
#define PPMP_1_SHIFT 16
#define PIXC_1_SHIFT PPMP_1_SHIFT

#define PPMPC_1S_MASK 0x00008000
#define PIXC_1S_MASK  PPMPC_1S_MASK
#define PPMPC_MS_MASK 0x00006000
#define PIXC_MS_MASK  PPMPC_MS_MASK
#define PPMPC_MF_MASK 0x00001C00
#define PIXC_MF_MASK  PPMPC_MF_MASK
#define PPMPC_SF_MASK 0x00000300
#define PIXC_SF_MASK  PPMPC_SF_MASK
#define PPMPC_2S_MASK 0x000000C0
#define PIXC_2S_MASK  PPMPC_2S_MASK
#define PPMPC_AV_MASK 0x0000003E
#define PIXC_AV_MASK  PPMPC_AV_MASK
#define PPMPC_2D_MASK 0x00000001
#define PIXC_2D_MASK  PPMPC_2D_MASK

#define PPMPC_MS_SHIFT 13
#define PIXC_MS_SHIFT  PPMPC_MS_SHIFT
#define PPMPC_MF_SHIFT 10
#define PIXC_MF_SHIFT  PPMPC_MF_SHIFT
#define PPMPC_SF_SHIFT 8
#define PIXC_SF_SHIFT  PPMPC_SF_SHIFT
#define PPMPC_2S_SHIFT 6
#define PIXC_2S_SHIFT  PPMPC_2S_SHIFT
#define PPMPC_AV_SHIFT 1
#define PIXC_AV_SHIFT  PPMPC_AV_SHIFT

/* PPMPC_1S_MASK definitions */
#define PPMPC_1S_PDC  0x00000000
#define PIXC_1S_PDC   PPMPC_1S_PDC
#define PPMPC_1S_CFBD 0x00008000
#define PIXC_1S_CFBD  PPMPC_1S_CFBD

/* PPMPC_MS_MASK definitions */
#define PPMPC_MS_CCB        0x00000000
#define PIXC_MS_CCB         PPMPC_MS_CCB
#define PPMPC_MS_PIN        0x00002000
#define PIXC_MS_PIN         PPMPC_MS_PIN
#define PPMPC_MS_PDC        0x00004000
#define PIXC_MS_PDC         PPMPC_MS_PDC
#define PPMPC_MS_PDC_MFONLY 0x00006000
#define PIXC_MS_PDC_MFONLY  PPMPC_MS_PDC_MFONLY

/* PPMPC_MF_MASK definitions */
#define PPMPC_MF_1 0x00000000
#define PIXC_MF_1  PPMPC_MF_1
#define PPMPC_MF_2 0x00000400
#define PIXC_MF_2  PPMPC_MF_2
#define PPMPC_MF_3 0x00000800
#define PIXC_MF_3  PPMPC_MF_3
#define PPMPC_MF_4 0x00000C00
#define PIXC_MF_4  PPMPC_MF_4
#define PPMPC_MF_5 0x00001000
#define PIXC_MF_5  PPMPC_MF_5
#define PPMPC_MF_6 0x00001400
#define PIXC_MF_6  PPMPC_MF_6
#define PPMPC_MF_7 0x00001800
#define PIXC_MF_7  PPMPC_MF_7
#define PPMPC_MF_8 0x00001C00
#define PIXC_MF_8  PPMPC_MF_8

/* PPMPC_SF_MASK definitions */
#define PPMPC_SF_2  0x00000100
#define PIXC_SF_2   PPMPC_SF_2
#define PPMPC_SF_4  0x00000200
#define PIXC_SF_4   PPMPC_SF_4
#define PPMPC_SF_8  0x00000300
#define PIXC_SF_8   PPMPC_SF_8
#define PPMPC_SF_16 0x00000000
#define PIXC_SF_16  PPMPC_SF_16

/* PPMPC_2S_MASK definitions */
#define PPMPC_2S_0    0x00000000
#define PIXC_2S_0     PPMPC_2S_0
#define PPMPC_2S_CCB  0x00000040
#define PIXC_2S_CCB   PPMPC_2S_CCB
#define PPMPC_2S_CFBD 0x00000080
#define PIXC_2S_CFBD  PPMPC_2S_CFBD
#define PPMPC_2S_PDC  0x000000C0
#define PIXC_2S_PDC   PPMPC_2S_PDC

/* PPMPC_AV_MASK definitions (only valid if CCB_USEAV set in ccb_Flags) */
#define	PPMPC_AV_SF2_1	 0x00000000
#define	PIXC_AV_SF2_1	 PPMPC_AV_SF2_1
#define	PPMPC_AV_SF2_2	 0x00000010
#define	PIXC_AV_SF2_2	 PPMPC_AV_SF2_2
#define	PPMPC_AV_SF2_4	 0x00000020
#define	PIXC_AV_SF2_4	 PPMPC_AV_SF2_4
#define	PPMPC_AV_SF2_PDC 0x00000030
#define	PIXC_AV_SF2_PDC  PPMPC_AV_SF2_PDC

#define	PPMPC_AV_SF2_MASK 0x00000030
#define	PIXC_AV_SF2_MASK  PPMPC_AV_SF2_MASK

#define	PPMPC_AV_DOWRAP	   0x00000008
#define	PIXC_AV_DOWRAP	   PPMPC_AV_DOWRAP
#define	PPMPC_AV_SEX_2S	   0x00000004 /*  Sign-EXtend, okay?  */
#define	PIXC_AV_SEX_2S	   PPMPC_AV_SEX_2S
#define	PPMPC_AV_INVERT_2S 0x00000002
#define	PIXC_AV_INVERT_2S  PPMPC_AV_INVERT_2S

/* PPMPC_2D_MASK definitions */
#define PPMPC_2D_1 0x00000000
#define PIXC_2D_1  PPMPC_2D_1
#define PPMPC_2D_2 0x00000001
#define PIXC_2D_2  PPMPC_2D_2

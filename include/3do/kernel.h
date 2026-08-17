#pragma force_top_level
#pragma once

#include "types.h"
#include "item.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "setjmp.h"

struct KernelBase
{
  Folio       kb;
  List       *kb_RomTags;
  List       *kb_MemFreeLists;
  List       *kb_MemHdrList;
  List       *kb_FolioList;	/* Libraries */
  List       *kb_Drivers;
  List       *kb_Devices;
  List       *kb_TaskWaitQ;	/* Tasks waiting for some event */
  List       *kb_TaskReadyQ;    /* Tasks waiting for CPU time */
  List       *kb_MsgPorts;	/* will we be message based? */
  List       *kb_Semaphores;	/* will we be message based? */
  Task       *kb_CurrentTask;   /* Currently executing Task */
  Node      **kb_InterruptHandlers;
  u32      kb_TimerBits;	/* allocated timers/ctrs */
  u32      kb_ElapsedQuanta; /* timerticks for current task */
  u32     *kb_VRAMHack;
  ItemEntry **kb_ItemTable;	/* table of ptrs to ItemEntries */
  s32       kb_MaxItem;
  u32      kb_CPUFlags;	/* various flags for operation */
  u8       kb_MaxInterrupts;
  u8       kb_Forbid;	/* software lockout for task swapping */
  u8       kb_FolioTableSize;
  u8       kb_PleaseReschedule;
  u32     *kb_MacPkt;
  u32      kb_Flags;
  u32      kb_Reserved;
  u32      kb_numticks;	/* convert secs to ticks numerator */
  u32      kb_denomticks;	/* convert secs to ticks denominator */
  u32      kb_Obsolete;	/* shadow copy of Madam->Msysbits */
  u8       kb_FolioTaskDataCnt; /* lwords */
  u8       kb_FolioTaskDataSize; /* lwords */
  u8       kb_DRAMSetSize;
  u8       kb_VRAMSetSize;
  Folio     **kb_DataFolios;
  jmp_buf    *kb_CatchDataAborts; /* setjmp buf */
  u32      kb_QuietAborts;   /* no messages for these bits */
  u32     *kb_RamDiskAddr;   /* kernel needs to help RamDevice */
  s32       kb_RamDiskSize;
  List       *kb_ExtendedErrors; /* list of extended err tables */
  u8       kb_MadamRev;
  u8       kb_ClioRev;
  u8       kb_Resbyte0;
  u8       kb_Resbyte1;
  Item        kb_DevSemaphore;  /* Device List Semaphore */
  List       *kb_SystemStackList; /* List of System stacks available */
  u32      kb_NumTaskSwitches; /* total # of switch since bootup  */
  u32     *kb_VRAM0;         /* memory reserved by kernel */
  u32      kb_VRAM0Size;
  u32     *kb_VRAM1;
  u32      kb_VRAM1Size;
  char       *kb_BootVolumeName;
  List       *kb_Tasks;
  u32      kb_MemEnd;	/* Address of end-of-memory */
};

extern struct KernelBase *KernelBase;

#define CURRENTTASK            (KernelBase->kb_CurrentTask)
#define GetCurrentSignals()    GetTaskSignals(CURRENTTASK)
#define ClearCurrentSignals(s) ((GetCurrentSignals() & (s)) ? WaitSignal(GetCurrentSignals() & (s)) : 0)

/* kb_CPUFlags */
#define KB_BIGENDIAN	1	/* we are Big Endian, 0=Little Endian */
#define KB_32BITMODE	2	/* 32 Bit Address Operation, 0=26 bit */
#define KB_ARM600	4	/* this is an ARM600 */
#define KB_SHERRY	8	/* new hardware? */
#define KB_SHERRIE	KB_SHERRY

#define KB_BLUE		0x10
#define KB_RED		0x20
#define KB_REDWW	0x40	/* Red Wire Wrap, not same as silicon */

#define KB_GREEN	0x80	/* temporary compatibility alias */

#define KB_WIREWRAP	0x100	/* Green (or above) wirewrap flag */

#define	KB_ROMOVERCD	0x0800	/* System was booted ROM-over-CD */
#define KB_CONTROLPORT	0x1000	/* control port found (MEI) */
#define	KB_ROMAPP	0x2000	/* this is the ROM app (crippled) kernel */
#define KB_NODBGRPOOF	0x4000	/* debugger is gone , just kidding */
#define KB_NODBGR	0x8000	/* debugger is gone */

#define KB_BROOKTREE	0x00010000	/* Brooktree (not philips) */
#define KB_SERIALPORT   0x00100000	/* diagnostic serial port */
#define KB_REBOOTED	0x01000000

/* other kernelbase flags */
#define KB_TASK_DBG	1	/* debug out for createtask */

#define REDMHZx1000	50000	/* 50mhz * 1000 red silicon */
#define MHZx1000	49091	/* 49.0908mhz * 1000 */
#define REDWWMHZx1000	26820	/* 26.82mhz */

#define CHIP_RED_REV   0
#define CHIP_GREEN_REV 1

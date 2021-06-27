/*****

$Id

$Log

*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  poddriver.h - definitions for the interface between the Event Broker
                and the microdrivers for the Control Port devices.
*/

#ifndef EVENT_H
# include "event.h"
#endif

enum PodLoginLogoutPhase {
  POD_Offline     = 0,
  POD_LoggingIn   = 1,
  POD_LoadDriver  = 2,
  POD_InitFailure = 3,
  POD_Online      = 4,
  POD_LoggingOut  = 5
};

#define POD_OldControlPad      0xFF
#define POD_LoginLogoutDelay   10

struct PodDriver;

typedef struct Pod {
  NamelessNode      pod;
  uint8             pod_Version;
  uint8             pod_Number;
  uint8             pod_Position;
  uint8             pod_LoginLogoutTimer;
  uint8             pod_LoginLogoutPhase;
  uint8             pod_Blipvert; /* want to write new output */
  uint8             pod_Flipvert; /* output includes flip-bits */
  uint8             pod_SuccessfulLogin; /* actually came on-line */  
  uint8             rfu[4];
  uint8             pod_GenericNumber[16];
  uint32            pod_Type;
  uint32            pod_BitsIn;
  uint32            pod_BitsOut;
  uint32            pod_Flags;
  uint32            pod_InputByteOffset;
  uint32            pod_OutputBitOffset;
  uint32            pod_EventsReady[8];
  Item              pod_LockHolder;
  struct PodDriver *pod_Driver;
  uint32            pod_PrivateData[8];
} Pod;

typedef struct PodDriver {
  Node             pd;
  int32            pd_DeviceType;
  void            *pd_DriverArea;
  uint32           pd_DriverAreaSize;
  uint32           pd_Flags;
  uint32           pd_UseCount;
  uint32           pd_FamilyCode;
  uint32	   pd_FamilyVersion;
  Err            (*pd_DriverEntry)();
} PodDriver;

#define PD_LOADED_FROM_ROM      0x80000000
#define PD_SHARED               0x40000000
#define PD_LOADED_INTO_RAM      0x20000000
#define PD_LOADED_FROM_FILE     0x10000000

typedef struct BufferSegment {
  uint32         bs_SegmentBytesUsed;
  uint32         bs_SegmentBitsFilled;
  uint8         *bs_SegmentBase;
} BufferSegment;

typedef struct ManagedBuffer {
  uint32           mb_BufferTotalSize;
  uint32           mb_BufferSegmentSize;
  uint32           mb_NumSegments;
  BufferSegment    mb_Segment[3];
} ManagedBuffer;

#define MB_INPUT_SEGMENT           0
#define MB_OUTPUT_SEGMENT          1
#define MB_FLIPBITS_SEGMENT        2

enum PodDriverCommandCode {
  PD_InitDriver           = 1,
  PD_InitPod              = 2,
  PD_ParsePodInput        = 3,
  PD_AppendEventFrames    = 4,
  PD_ProcessCommand       = 5,
  PD_ConstructPodOutput   = 6,
  PD_TeardownPod          = 7,
  PD_ShutdownDriver       = 8,
  PD_ReadData             = 9,
  PD_WriteData            = 10,
  PD_ReconnectPod         = 11
};

typedef struct PodInterface {
  uint32                      pi_Version;
  enum PodDriverCommandCode   pi_Command;
  Pod                        *pi_Pod;
  ManagedBuffer              *pi_ControlPortBuffers;
  uint8                      *pi_CommandIn;
  uint32                      pi_CommandInLen;
  uint8                      *pi_CommandOut;
  uint32                      pi_CommandOutLen;
  EventFrame                 *pi_NextFrame;
  void                       *pi_EndOfFrameArea;
  uint32                     *pi_TriggerMask;
  uint32                     *pi_CaptureMask;
  uint32                      pi_VCNT;
  uint8                       pi_RecoverFromLostEvents;
  Err                       (*pi_PackBits)(uint32 dataBits,
					   uint32 bitCount,
					   uint32 leftJustified,
					   ManagedBuffer *buf,
					   uint32 bufferSegment);
  void                    * (*pi_malloc)(int32 numBytes);
  void                      (*pi_free)(void *p);
  EventFrame              * (*pi_InitFrame)(uint32 eventNum,
					    uint32 frameSize,
					    EventFrame **next,
					    void **end);
} PodInterface;

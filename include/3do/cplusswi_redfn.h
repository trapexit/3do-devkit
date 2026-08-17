#pragma once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: cplusswi_redfn.h,v 1.1 1994/11/30 02:37:16 vertex Exp $
**
**  Do not include this file manually. It is intended to be included
**  automatically by the C++ support script.
**
******************************************************************************/

#ifdef __REDFN_AUDIO /* audio.h */
#define AUDIOSWI 0x40000
Err    __swi(AUDIOSWI+0) TweakKnob( Item KnobItem, s32 Value );
Err    __swi(AUDIOSWI+1) StartInstrument( Item Instrument, TagArg *TagList);
Err    __swi(AUDIOSWI+2) ReleaseInstrument( Item Instrument, TagArg *TagList);
Err    __swi(AUDIOSWI+3) StopInstrument( Item Instrument, TagArg *TagList);
Err    __swi(AUDIOSWI+4) TuneInsTemplate( Item InsTemplate, Item Tuning );
Err    __swi(AUDIOSWI+5) TuneInstrument( Item Instrument, Item Tuning );
Err    __swi(AUDIOSWI+8) ConnectInstruments( Item SrcIns, char *SrcName, Item DstIns, char *DstName);
u32 __swi(AUDIOSWI+9) TraceAudio( s32 Mask );
s32  __swi(AUDIOSWI+10) AllocAmplitude( s32 Amplitude);
Err    __swi(AUDIOSWI+11) FreeAmplitude( s32 Amplitude);
Err    __swi(AUDIOSWI+12) DisconnectInstruments( Item SrcIns, char *SrcName, Item DstIns, char *DstName);
Err    __swi(AUDIOSWI+13) SignalAtTime( Item Cue, AudioTime Time );
Err    __swi(AUDIOSWI+15) SetAudioRate( Item Owner, frac16 Rate);
Err    __swi(AUDIOSWI+16) SetAudioDuration( Item Owner, u32 Frames);
Err    __swi(AUDIOSWI+17) TweakRawKnob( Item KnobItem, s32 Value );
Err    __swi(AUDIOSWI+18) StartAttachment( Item Attachment, TagArg *tp );
Err    __swi(AUDIOSWI+19) ReleaseAttachment( Item Attachment, TagArg *tp );
Err    __swi(AUDIOSWI+20) StopAttachment(  Item Attachment, TagArg *tp );
Err    __swi(AUDIOSWI+21) LinkAttachments( Item At1, Item At2 );
Err    __swi(AUDIOSWI+22) MonitorAttachment( Item Attachment, Item Cue, s32 CueAt );
Err    __swi(AUDIOSWI+24) AbandonInstrument( Item Instrument );
Item   __swi(AUDIOSWI+25) AdoptInstrument( Item InsTemplate );
Item   __swi(AUDIOSWI+26) ScavengeInstrument( Item InsTemplate, u8 Priority, s32 MaxActivity, s32 IfSystemWide );
Err    __swi(AUDIOSWI+27) SetAudioItemInfo( Item AnyItem, TagArg *tp );
Err    __swi(AUDIOSWI+28) PauseInstrument( Item Instrument );
Err    __swi(AUDIOSWI+29) ResumeInstrument( Item Instrument );
s32  __swi(AUDIOSWI+30) WhereAttachment( Item Attachment );
Err    __swi(AUDIOSWI+32) BendInstrumentPitch( Item Instrument, frac16 BendFrac );
Err    __swi(AUDIOSWI+33) AbortTimerCue( Item Cue );
Err    __swi(AUDIOSWI+34) EnableAudioInput( s32 OnOrOff, TagArg *Tags );
Err    __swi(AUDIOSWI+36) ReadProbe( Item Probe, s32 *ValuePtr );
u16 __swi(AUDIOSWI+38) GetAudioFrameCount( void );
s32  __swi(AUDIOSWI+39) GetAudioCyclesUsed( void );
#endif

/*****************************************************************************/

#ifdef __REDFN_DEBUG /* debug.h */
extern void __swi(0x1000e) kprintf(char *fmt, ... );
extern int  __swi(0x10000+30) MayGetChar(void);
extern void __swi(0x101) Debug(void);
#endif

/*****************************************************************************/

#ifdef __REDFN_FILEFUNCTIONS /* filefunctions.h */
#define FILEFOLIOSWI 0x00030000
extern Item  __swi(FILEFOLIOSWI+0) OpenDiskFile(char *path);
extern s32 __swi(FILEFOLIOSWI+1) CloseDiskFile(Item fileItem);
extern Item  __swi(FILEFOLIOSWI+4) MountFileSystem(Item deviceItem, s32 unit, u32 blockOffset);
extern Item  __swi(FILEFOLIOSWI+5) OpenDiskFileInDir(Item dirItem, char *path);
extern Item  __swi(FILEFOLIOSWI+6) MountMacFileSystem(char *path);
extern Item  __swi(FILEFOLIOSWI+7) ChangeDirectory(char *path);
extern Item  __swi(FILEFOLIOSWI+8) GetDirectory(char *pathBuf, int pathBufLen);
extern Item  __swi(FILEFOLIOSWI+9) CreateFile(char *path);
extern Err   __swi(FILEFOLIOSWI+10) DeleteFile(char *path);
extern Item  __swi(FILEFOLIOSWI+11) CreateAlias(char *aliasPath,
extern Err   __swi(FILEFOLIOSWI+13) DismountFileSystem(char *name);
#endif

/*****************************************************************************/

#ifdef __REDFN_TYPES /* types.h */
#define KERNELSWI 0x10000
#endif

/*****************************************************************************/

#ifdef __REDFN_GRAPHICS
#define GRAFSWI 0x2000
Err __swi(GRAFSWI+39) DrawCels (Item bitmapItem, CCB *ccb);
Err __swi(GRAFSWI+23) DrawScreenCels (Item screenItem, CCB *ccb);
#endif

/*****************************************************************************/

#ifdef __REDFN_HARDWARE /* hardware.h */
u32 __swi(KERNELSWI+17) ReadHardwareRandomNumber(void);
#endif

/*****************************************************************************/

#ifdef __REDFN_IO /* io.h */
Err __swi(KERNELSWI+24)	SendIO(Item ior, const IOInfo *ioiP);       /* async  */
Err __swi(KERNELSWI+37) DoIO(Item ior, const IOInfo *ioiP);   /* sync   */
Err __swi(KERNELSWI+25) AbortIO(Item ior);      /* abort an io          */
Err __swi(KERNELSWI+41) WaitIO(Item ior);              /* wait for io completion */
#endif

/*****************************************************************************/

#ifdef __REDFN_ITEM /* item.h */
Item  __swi(KERNELSWI+0)  CreateSizedItem(s32 ctype,TagArg *p,s32 size);
Err   __swi(KERNELSWI+3)  DeleteItem(Item i);
Item  __swi(KERNELSWI+4)  FindItem(s32 ctype,TagArg *tp);
Err   __swi(KERNELSWI+8)  CloseItem(Item i);
Item  __swi(KERNELSWI+5)  OpenItem(Item foundItem, void *args);
s32 __swi(KERNELSWI+10) SetItemPri(Item i,u8 newpri);
Err   __swi(KERNELSWI+28) SetItemOwner(Item i,Item newOwner);
s32 __swi(KERNELSWI+7)  LockItem(Item s,u32 flags);
Err   __swi(KERNELSWI+6)  UnlockItem(Item s);
Item  __swi(KERNELSWI+36) FindAndOpenItem(s32 ctype,TagArg *tp);
#endif

/*****************************************************************************/

#ifdef __REDFN_MEM /* mem.h */
void  __swi(KERNELSWI+13) *AllocMemBlocks(s32 size, u32 typebits);
Err   __swi(KERNELSWI+20)  ControlMem(void *p, s32 size, s32 cmd, Item task);
s32 __swi(KERNELSWI+33)  SystemScavengeMem(void);
#endif

/*****************************************************************************/

#ifdef __REDFN_MSGPORT /* msgport.h */
extern Err __swi(KERNELSWI+16)  SendMsg(Item mp,Item msg, const void *dataptr, s32 datasize);
extern Err __swi(KERNELSWI+16)  SendSmallMsg(Item mp,Item msg, u32 val1, u32 val2);
extern Item __swi(KERNELSWI+19)	GetMsg(Item mp);
extern Item __swi(KERNELSWI+15)	GetThisMsg(Item msg);
extern Item __swi(KERNELSWI+40) WaitPort(Item mp,Item msg);
extern Err __swi(KERNELSWI+18)  ReplyMsg(Item msg, s32 result, const void *dataptr, s32 datasize);
extern Err __swi(KERNELSWI+18)  ReplySmallMsg(Item msg, s32 result,
#endif

/*****************************************************************************/

#ifdef __REDFN_OPERAMATH /* operamath.h */
#define MATHSWI 0x50000
#define MULMANYVEC3MAT33DIVZ_F16 (MATHSWI+18)
#define MULVEC3MAT33DIVZ_F16 (MATHSWI+17)
#define ABSVEC4_F16 (MATHSWI+16)
#define ABSVEC3_F16 (MATHSWI+15)
#define CROSS3_F16 (MATHSWI+14)
#define DOT4_F16 (MATHSWI+13)
#define DOT3_F16 (MATHSWI+12)
#define MULOBJECTMAT44_F16 (MATHSWI+11)
#define MULOBJECTVEC4MAT44_F16 (MATHSWI+10)
#define MULMANYVEC4MAT44_F16 (MATHSWI+9)
#define MULMAT44MAT44_F16 (MATHSWI+8)
#define MULVEC4MAT44_F16 (MATHSWI+7)
#define MULSCALARF16 (MATHSWI+6)
#define MULMANYF16 (MATHSWI+5)
#define MULOBJECTMAT33_F16 (MATHSWI+4)
#define MULOBJECTVEC3MAT33_F16 (MATHSWI+3)
#define MULMANYVEC3MAT33_F16 (MATHSWI+2)
#define MULMAT33MAT33_F16 (MATHSWI+1)
#define MULVEC3MAT33_F16 (MATHSWI+0)

void __swi(MULMANYVEC3MAT33DIVZ_F16) MulManyVec3Mat33DivZ_F16 (mmv3m33d *s);
void __swi(MULVEC3MAT33DIVZ_F16) MulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n);
void __swi(MULVEC3MAT33_F16) MulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat);
void __swi(MULVEC4MAT44_F16) MulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat);
void __swi(MULMAT33MAT33_F16) MulMat33Mat33_F16 (mat33f16 dest, mat33f16 src1, mat33f16 src2);
void __swi(MULMAT44MAT44_F16) MulMat44Mat44_F16 (mat44f16 dest, mat44f16 src1, mat44f16 src2);
frac16 __swi(DOT3_F16) Dot3_F16 (vec3f16 v1, vec3f16 v2);
frac16 __swi(DOT4_F16) Dot4_F16 (vec4f16 v1, vec4f16 v2);
void __swi(CROSS3_F16) Cross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2);
void __swi(MULMANYVEC3MAT33_F16) MulManyVec3Mat33_F16(vec3f16 *dest, vec3f16 *src, mat33f16 mat, s32 count);
void __swi(MULMANYVEC4MAT44_F16) MulManyVec4Mat44_F16(vec4f16 *dest, vec4f16 *src, mat44f16 mat, s32 count);
void __swi(MULOBJECTVEC3MAT33_F16) MulObjectVec3Mat33_F16(void *objectlist[], ObjOffset1 *offsetstruct, s32 count);
void __swi(MULOBJECTVEC4MAT44_F16) MulObjectVec4Mat44_F16(void *objectlist[], ObjOffset1 *offsetstruct, s32 count);
void __swi(MULOBJECTMAT33_F16) MulObjectMat33_F16(void *objectlist[], ObjOffset2 *offsetstruct, mat33f16 mat, s32 count);
void __swi(MULOBJECTMAT44_F16) MulObjectMat44_F16(void *objectlist[], ObjOffset2 *offsetstruct, mat44f16 mat, s32 count);
void __swi(MULMANYF16) MulManyF16 (frac16 *dest, frac16 *src1, frac16 *src2, s32 count);
void __swi(MULSCALARF16) MulScalarF16 (frac16 *dest, frac16 *src, frac16 scalar, s32 count);
frac16 __swi(ABSVEC3_F16) AbsVec3_F16 (vec3f16 vec);
frac16 __swi(ABSVEC4_F16) AbsVec4_F16 (vec4f16 vec);
#endif

/*****************************************************************************/

#ifdef __REDFN_SEMAPHORE /* semaphore.h */
s32 __swi(KERNELSWI+7) LockSemaphore(Item s,u32 flags);
Err __swi(KERNELSWI+6)   UnlockSemaphore(Item s);
#endif

/*****************************************************************************/

#ifdef __REDFN_TASK /* task.h */
extern s32 __swi(KERNELSWI+1) WaitSignal(u32 sigMask);
extern Err __swi(KERNELSWI+2)   SendSignal(Item task,u32 sigMask);
extern void __swi(KERNELSWI+9)   Yield(void);
extern s32 __swi(KERNELSWI+21)	AllocSignal(u32 sigMask);
extern Err __swi(KERNELSWI+22)	FreeSignal(u32 sigMask);
extern Err __swi(KERNELSWI+39)	SetExitStatus(s32 status);
#endif

/*****************************************************************************/

#ifdef __REDFN_TIME /* time.h */
u32 __swi(KERNELSWI+38) SampleSystemTime(void);
#endif

/*****************************************************************************/

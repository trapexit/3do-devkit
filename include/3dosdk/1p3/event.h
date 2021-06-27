/*****

$Id: event.h,v 1.18 1994/03/24 01:52:17 dplatt Exp $

$Log: event.h,v $
 * Revision 1.18  1994/03/24  01:52:17  dplatt
 * Define the device-online/offline, and filesystem-mount/dismount
 * event structures.  Rename DiscInserted to DeviceOnline.
 *
 * Revision 1.17  1994/03/19  01:17:44  dplatt
 * Formally declare ID 0 to be illegal, since it tends to indicate the
 * presence of a fried controller or a short circuit on the bus.
 *
 * Revision 1.16  1994/03/19  01:12:23  dplatt
 * Add pod flags to tell the Event Broker whether the pod will work
 * correctly with short Control Port transfers, more than one such transfer
 * per VBL, and/or transfer filtering.
 *
 * Add an event structure for the SendEvent message.
 *
 * Revision 1.15  1994/03/01  01:18:54  shawn
 * Fixed CR 150
 *
 * Revision 1.14  1994/03/01  00:49:52  dplatt
 * Add some #defines: pod IDs for pods that have actually been built
 * (including several in the extended range) and some useful
 * administrivia.  Deborah, this #define's for you!
 *
 * Revision 1.13  1994/02/04  21:18:02  dplatt
 * Update the analog-joystick events.
 *
 * Revision 1.12  1994/01/26  22:25:19  dplatt
 * Add IR-controller event structures and codes.
 *
 * Revision 1.11  1994/01/20  05:19:39  phil
 * fixed C++ mods
 *
 * Revision 1.10  1993/12/21  19:54:21  dplatt
 * Update the analog-joystick event codes
 *
 * Revision 1.9  1993/12/16  00:01:49  dplatt
 * Add joystick support.  Add C++ wrapper.
 *
 * Revision 1.8  1993/11/13  00:03:35  dplatt
 * Add lightgun definitions
 *
 * Revision 1.7  1993/08/31  22:09:58  dplatt
 * Add lightgun support
 *
 *****/

/*
  Copyright The 3DO Company Inc., 1992.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#pragma include_only_once

#ifndef __H_EVENT
#define __H_EVENT

#ifndef __GRAPHICS_H
# include "graphics.h"
#endif

#ifndef __H_DISCDATA
# include "discdata.h"
#endif

#define EventPortName "eventbroker"

#define EVENT_QUEUE_MAX_PERMITTED 20
#define EVENT_QUEUE_DEFAULT       3


enum EventBrokerFlavor {
  EB_NoOp               = 0,
  EB_Configure          = 1,
  EB_ConfigureReply     = 2,
  EB_EventRecord        = 3,
  EB_EventReply         = 4,
  EB_SendEvent          = 5,
  EB_SendEventReply     = 6,
  EB_Command            = 7,
  EB_CommandReply       = 8,
  EB_RegisterEvent      = 9,
  EB_RegisterEventReply = 10,
  EB_GetListeners       = 11,
  EB_GetListenersReply  = 12,
  EB_SetFocus           = 13,
  EB_SetFocusReply      = 14,
  EB_GetFocus           = 15,
  EB_GetFocusReply      = 16,
  EB_ReadPodData        = 17,
  EB_ReadPodDataReply   = 18,
  EB_WritePodData       = 19,
  EB_WritePodDataReply  = 20,
  EB_LockPod            = 21,
  EB_LockPodReply       = 22,
  EB_UnlockPod          = 23,
  EB_UnlockPodReply     = 24,
  EB_IssuePodCmd        = 25,
  EB_IssuePodCmdReply   = 26,
  EB_DescribePods       = 27,
  EB_DescribePodsReply  = 28
};

#define	LC_ISNOTFOCUSED		0
#define	LC_ISFOCUSED		1
enum ListenerCategory {
  LC_NoSeeUm         = 0,
  LC_FocusListener   = 1,
  LC_Observer        = 2,
  LC_FocusUI         = 3
};

/*****
 The Event Broker includes the following header in every message it
 sends.  The header, followed by some amount of data (in many cases)
 can be accessed via the data pointer and size fields in the message
 header.

 If a listener sends the Event Broker a small (standard size, pass by
 reference) message, the Event Broker will transmit back a minimal reply...
 it will simply set the reply value in the message structure to {zero,
 or an Item number, or an error code} and send the message back with
 a null data pointer and size.  No header or data will be transmitted.

 If a listener sends the Event Broker a larger (pass by reference) message,
 the Broker mau send back a block of data in the message which consists
 of an EventBrokerHeader followed by [optional] information.  This reply
 data is not always present - the listener should check the data pointer
 and length to see if it was sent.

 Hence - if you want to send a message to the Event Broker and get back
 _any_ information other than success/failure/Item-number kinds of stuff,
 be sure to use CreateSizedItem to create the message, and make the
 message big enough to hold the amount of data you expect to receive!
*****/

typedef struct EventBrokerHeader {
  enum EventBrokerFlavor ebh_Flavor;
} EventBrokerHeader;

/*****   EB_Configure message

 Data transmitted from listener->Broker to [re]configure an event-listener
 port.

*****/

typedef struct ConfigurationRequest {
  EventBrokerHeader     cr_Header; /* { EB_Configure } */
  enum ListenerCategory cr_Category; /* focus, monitor, or hybrid */
  uint32                cr_TriggerMask[8]; /* events to trigger on */
  uint32                cr_CaptureMask[8]; /* events to capture */
  int32                 cr_QueueMax;  /* max # events in transit */
  uint32                rfu[8];            /* must be zero for now */
} ConfigurationRequest;

/*****   EB_EventRecord message

 Data transmitted from Broker->listener to report one or more events.
 Data consists of an EventBrokerHeader followed by one or more
 EventFrame structures (with ef_ByteCount > 0), followed by a
 degenerate EventFrame (ef_ByteCount == 0, remainder of frame not
 present or accounted for in the message data length).

 The same format is used in an EB_SendEvent message transmitted from
 a listener to the Event Broker.

*****/

typedef struct EventFrame {
  uint32         ef_ByteCount;         /* total size of EventFrame */
  uint32         ef_SystemID;          /* Opera machine ID, or zero=local */
  uint32         ef_SystemTimeStamp;   /* event-count timestamp */
  int32          ef_Submitter;         /* Item of event sender, or 0 */
  uint8          ef_EventNumber;       /* event code, [0,255] */
  uint8          ef_PodNumber;         /* CP pod number, or zero */
  uint8          ef_PodPosition;       /* CP position on daisychain, or zero */
  uint8          ef_GenericPosition;   /* Nth generic device of type, or 0 */
  uint8          ef_Trigger;           /* 1 for trigger, 0 for capture */
  uint8          rfu1[3];
  uint32         rfu2;
  uint32         ef_EventData[1];      /* first word of event data */
} EventFrame;


typedef struct EventFrameHeader {
  uint32         ef_ByteCount;         /* total size of EventFrame */
  uint32         ef_SystemID;          /* Opera machine ID, or zero=local */
  uint32         ef_SystemTimeStamp;   /* event-count timestamp */
  int32          ef_Submitter;         /* Item of event sender, or 0 */
  uint8          ef_EventNumber;       /* event code, [0,255] */
  uint8          ef_PodNumber;         /* CP pod number, or zero */
  uint8          ef_PodPosition;       /* CP position on daisychain, or zero */
  uint8          ef_GenericPosition;   /* Nth generic device of type, or 0 */
  uint8          ef_Trigger;           /* 1 for trigger, 0 for capture */
  uint8          rfu1[3];
  uint32         rfu2;
} EventFrameHeader;

typedef struct SendEvent {
  EventBrokerHeader  se_Header;
  EventFrame         se_FirstFrame;
} SendEvent;


/*
   Specific event formats follow
*/

typedef struct ControlPadEventData {
  uint32         cped_ButtonBits;  /* left justified, zero fill */
} ControlPadEventData;

/*****
 By no coincidence whatsoever, the ordering of the bits in the generic
 Control Pad event data is identical to the ordering of these bits in
 the Control Port data sent by a standard 3DO Control Pad pod.  Only
 the shift offset has been changed to protect the guilty.
*****/

#define ControlDown          0x80000000
#define ControlUp            0x40000000
#define ControlRight         0x20000000
#define ControlLeft          0x10000000
#define ControlA             0x08000000
#define ControlB             0x04000000
#define ControlC             0x02000000
#define ControlStart         0x01000000
#define ControlX             0x00800000
#define ControlRightShift    0x00400000
#define ControlLeftShift     0x00200000

typedef struct MouseEventData {
  uint32         med_ButtonBits;   /* left justified, zero fill */
  int32          med_HorizPosition;
  int32          med_VertPosition;
} MouseEventData;

#define MouseLeft            0x80000000
#define MouseMiddle          0x40000000
#define MouseRight           0x20000000
#define MouseShift           0x10000000

/*
   Since people have asked, here's the explanation:

   -  The "Gun" generic class and event structures are defined for the
      use of any Control Port (or other) device/pod which has enough
      electronic smarts in it to actually report an X and Y position on
      the screen at which the gun was pointed.  The position values are
      assumed to be in screen coordinates.

   -  The "Lightgun" generic class and event structures are for the use
      of pods/devices which can't report X and Y positions, but only a
      time (counter value) between the beginning of field, and the time
      that the scan-beam passed into the field-of-view of the lightgun
      sensor.  Some lightguns can also send in a "signal quality" pulse
      counter, this being the number of successive horizontal scan lines
      during which the beam was seen at roughly the same horizontal
      posotion.  A line pulse count of 0 means "no hit".
*/

typedef struct GunEventData {
  uint32         ged_ButtonBits;   /* left justified, zero fill */
  int32          ged_HorizPosition;
  int32          ged_VertPosition;
} GunEventData;

typedef struct LightGunEventData {
  uint32         lged_ButtonBits;       /* left justified, zero fill */
  uint32         lged_Counter;          /* counter at top-center of hit */
  uint32         lged_LinePulseCount;   /* # of scan lines which were hit */
} LightGunEventData;

#define LightGunTrigger      0x80000000

typedef struct CharacterEventData {
  uint32         ced_CharacterTyped;  /* right-justified */
  uint32         ced_ModifierKeys;    /* left justified ? */
} CharacterEventData;

typedef struct KeyboardEventData {
  uint32         ked_KeyMatrix[8];
} KeyboardEventData;

typedef struct StickEventData {
  uint32         stk_ButtonBits;        /* left justified, zero fill */
  int32          stk_HorizPosition; 
  int32          stk_VertPosition;
  int32          stk_DepthPosition;
} StickEventData;

#define StickCapability      0x000C0000
#define Stick4Way            0x00080000
#define StickTurbulence      0x00040000
#define StickButtons         0xFFF00000
#define StickFire            0x80000000
#define StickA               0x40000000
#define StickB               0x20000000
#define StickC               0x10000000
#define StickUp              0x08000000
#define StickDown            0x04000000
#define StickRight           0x02000000
#define StickLeft            0x01000000
#define StickPlay            0x00800000
#define StickStop            0x00400000
#define StickLeftShift       0x00200000
#define StickRightShift      0x00100000


typedef struct IRControllerEventData {
  uint32         ir_KeyMatrix[8];      /* 256 key codes, 1 = down, 0 = up */
  int32          ir_KeyCode;           /* key code for up/down events */
  int32          ir_GenericCode;       /* generic code for up/down events */
  int32          ir_Model;             /* type of IR controller */
} IRControllerEventData;

#define IRModelGeneric3DO    0x33444F20
#define IRModelForage        0x34414745


/*
   Generic IR-controller codes are not yet defined.
*/

typedef struct DeviceStateEventData {
  Item            dsed_DeviceItem;
  uint32          dsed_DeviceUnit;
} DeviceStateEventData;

typedef struct FilesystemEventData {
  Item            fsed_FilesystemItem;
  char            fsed_Name[FILESYSTEM_MAX_NAME_LEN];
} FilesystemEventData;

/*
   Event-number definitions.  Event numbers count from 1 to 256, and must
   correspond 1:1 with the event bits defined a bit further down.
*/

#define EVENTNUM_ControlButtonPressed                1
#define EVENTNUM_ControlButtonReleased               2
#define EVENTNUM_ControlButtonUpdate                 3
#define EVENTNUM_ControlButtonArrived                4
#define EVENTNUM_MouseButtonPressed                  5
#define EVENTNUM_MouseButtonReleased                 6
#define EVENTNUM_MouseUpdate                         7
#define EVENTNUM_MouseMoved                          8
#define EVENTNUM_MouseDataArrived                    9
#define EVENTNUM_GunButtonPressed                   10
#define EVENTNUM_GunButtonReleased                  11
#define EVENTNUM_GunUpdate                          12
#define EVENTNUM_GunDataArrived                     13
#define EVENTNUM_KeyboardKeyPressed                 14
#define EVENTNUM_KeyboardKeyReleased                15
#define EVENTNUM_KeyboardUpdate                     16
#define EVENTNUM_KeyboardDataArrived                17
#define EVENTNUM_CharacterEntered                   18
#define EVENTNUM_GivingFocus                        19
#define EVENTNUM_LosingFocus                        20
#define EVENTNUM_LightGunButtonPressed              21
#define EVENTNUM_LightGunButtonReleased             22
#define EVENTNUM_LightGunUpdate                     23
#define EVENTNUM_LightGunFireTracking               24
#define EVENTNUM_LightGunDataArrived                25
#define EVENTNUM_StickButtonPressed                 26
#define EVENTNUM_StickButtonReleased                27
#define EVENTNUM_StickUpdate                        28
#define EVENTNUM_StickMoved                         29
#define EVENTNUM_StickDataArrived                   30
#define EVENTNUM_IRKeyPressed                       31
#define EVENTNUM_IRKeyReleased                      32

#define EVENTNUM_DeviceOnline                       64
#define EVENTNUM_DeviceOffline                      65
#define EVENTNUM_FilesystemMounted                  66
#define EVENTNUM_FilesystemOffline                  67
#define EVENTNUM_FilesystemDismounted               68
#define EVENTNUM_ControlPortChange                  69
#define EVENTNUM_PleaseSaveAndExit                  70
#define EVENTNUM_PleaseExitImmediately              71
#define EVENTNUM_EventQueueOverflow                 72

#define EVENTBIT0_ControlButtonPressed               0x80000000
#define EVENTBIT0_ControlButtonReleased              0x40000000
#define EVENTBIT0_ControlButtonUpdate                0x20000000
#define EVENTBIT0_ControlButtonArrived               0x10000000
#define EVENTBIT0_MouseButtonPressed                 0x08000000
#define EVENTBIT0_MouseButtonReleased                0x04000000
#define EVENTBIT0_MouseUpdate                        0x02000000
#define EVENTBIT0_MouseMoved                         0x01000000
#define EVENTBIT0_MouseDataArrived                   0x00800000
#define EVENTBIT0_GunButtonPressed                   0x00400000
#define EVENTBIT0_GunButtonReleased                  0x00200000
#define EVENTBIT0_GunUpdate                          0x00100000
#define EVENTBIT0_GunDataArrived                     0x00080000
#define EVENTBIT0_KeyboardKeyPressed                 0x00040000
#define EVENTBIT0_KeyboardKeyReleased                0x00020000
#define EVENTBIT0_KeyboardUpdate                     0x00010000
#define EVENTBIT0_KeyboardDataArrived                0x00008000
#define EVENTBIT0_CharacterEntered                   0x00004000
#define EVENTBIT0_GivingFocus                        0x00002000
#define EVENTBIT0_LosingFocus                        0x00001000
#define EVENTBIT0_LightGunButtonPressed              0x00000800
#define EVENTBIT0_LightGunButtonReleased             0x00000400
#define EVENTBIT0_LightGunUpdate                     0x00000200
#define EVENTBIT0_LightGunFireTracking               0x00000100
#define EVENTBIT0_LightGunDataArrived                0x00000080
#define EVENTBIT0_StickButtonPressed                 0x00000040
#define EVENTBIT0_StickButtonReleased                0x00000020
#define EVENTBIT0_StickUpdate                        0x00000010
#define EVENTBIT0_StickMoved                         0x00000008
#define EVENTBIT0_StickDataArrived                   0x00000004
#define EVENTBIT0_IRKeyPressed                       0x00000002
#define EVENTBIT0_IRKeyReleased                      0x00000001

#define EVENTBIT2_DeviceOnline                       0x80000000
#define EVENTBIT2_DeviceOffline                      0x40000000
#define EVENTBIT2_FilesystemMounted                  0x20000000
#define EVENTBIT2_FilesystemOffline                  0x10000000
#define EVENTBIT2_FilesystemDismounted               0x08000000
#define EVENTBIT2_ControlPortChange                  0x04000000
#define EVENTBIT2_PleaseSaveAndExit                  0x02000000
#define EVENTBIT2_PleaseExitImmediately              0x01000000
#define EVENTBIT2_EventQueueOverflow                 0x00800000

/*****   EB_Command message

 For internal use, not specified here.

*****/

/*****   EB_RegisterEvent message

 Used to register an event name with the Event Broker.  The reply message
 will be a bufferless response, with the reply value being either an
 error code or the number of an event which has been allocated to
 this registered name.

*****/

typedef struct RegisterEvent {
  EventBrokerHeader     re_Header; /* { EB_RegisterEvent } */
  int32                 re_IsUserInterfaceEvent;
  char                  re_EventName[256];
} RegisterEvent;

/*****     EB_GetListeners message,  EB_GetListenersReply response

 Message consists of a simple request to be given a list of all
 Event Broker listeners (no data other than the header need be present).
 Response contains a list of all known listeners.

*****/

typedef struct ListenerList {
  EventBrokerHeader     ll_Header; /* { EB_GetListenersReply } */
  int32                 ll_Count;
  struct {
    Item                  li_PortItem;
    enum ListenerCategory li_Category;
  } ll_Listener[1];
} ListenerList;

/*****   EB_SetFocus message,  EB_SetFocusReply response
         EB_GetFocus message,  EB_GetFocusReply response

 Messages used to set and read the focus.  The reply responses carry the
 current focus-listener's Item number as the reply value, and do not carry
 a data buffer.  The act of setting focus may at some point be limited
 a privileged system task.  Anybody can find out who has focus.

*****/

typedef struct SetFocus {
  EventBrokerHeader     sf_Header; /* { EB_SetFocus } */
  Item                  sf_DesiredFocusListener;
} SetFocus;

/*****   EB_ReadPodData message,  EB_ReadPodDataReply response
         EB_WritePodData message, EB_WritePodDataReply response
         EB_IssuePodCmd message , EB_IssuePodCmdReply response
         EB_LockPod message,      EB_LockPodReply response
         EB_UnlockPod message,    EB_UnlockPodReply response

 Messages used to read the currently-available data from a pod, or to
 write raw data to a pod, or to issue a command to the pod driver,
 or to lock or unlock the pod.

 All of these messages use the same data structure.  The pd_WaitFlag has
 the following meaning:

 Message                flag == 0                   flag == 1

 ReadPodData            Read current data           Wait until next CP field
                        and reply immediately       then read data and reply

 WritePodData           Update data-to-be-sent      Actually send data during
                        and reply immediately       next CP field, then reply

 IssuePodCmd            Start command and           Execute command, reply
                        reply immediately           when command is complete

 LockPod                Lock pod if possible        Wait until pod can be
                        and reply, or return        locked, then lock and
                        a cannot-lock reply         reply.

 UnlockPod              Unlock pod and reply        <--- same

*****/

typedef struct PodData {
  EventBrokerHeader    pd_Header;
  int32                pd_PodNumber;
  int32                pd_WaitFlag;
  int32                pd_DataByteCount;
  uint8                pd_Data[4];
} PodData;

typedef struct PodDataHeader {
  EventBrokerHeader    pd_Header;
  int32                pd_PodNumber;
  int32                pd_WaitFlag;
  int32                pd_DataByteCount;
} PodDataHeader;

/*****   EB_DescribePods message,  EB_DescribePodsReply response

 Messages used to ask the Event Broker to describe the set of pods
 known to be attached to the Control Port.  The DescribePods message
 carries no data [but should be sent in a fairly large message, to
 provide space for the response].  The DescribePodsReply response
 contains a pod count, and array of pod descriptions.

*****/

typedef struct PodDescription {
  uint8          pod_Number;
  uint8          pod_Position;
  uint8          rfu[2];
  uint32         pod_Type;
  uint32         pod_BitsIn;
  uint32         pod_BitsOut;
  uint32         pod_Flags;
  uint8          pod_GenericNumber[16];
  Item           pod_LockHolder;
} PodDescription;

typedef struct PodDescriptionList {
  EventBrokerHeader    pdl_Header;
  int32                pdl_PodCount;
  PodDescription       pdl_Pod[1];
} PodDescriptionList;


/*****
 Important note - the leftmost 16 bits in the pod flags word are
 specially reserved - if the Nth-leftmost bit is set, then the
 device is a generic of type N and the Nth entry in the pod_GenericNumber
 table gives the device's ordinal position for devices of that
 generic type.  E.g. if the 0th bit is set, the device is a generic
 control pad, and pod_GenericNumber[0] contains 1 if it's the first
 generic control pad, 2 if it's the second, etc.

 Any flag bits not having to do with generic identity should go in the
 righthand 16 bits.
*****/

#define POD_IsControlPad        0x80000000
#define POD_IsMouse             0x40000000
#define POD_IsGun               0x20000000
#define POD_IsGlassesCtlr       0x10000000
#define POD_IsAudioCtlr         0x08000000
#define POD_IsKeyboard          0x04000000
#define POD_IsLightGun          0x02000000
#define POD_IsStick             0x01000000
#define POD_IsIRController      0x00800000

/*****
 The following special flags are used for communication between the pod
 driverlet and the Event Broker.

 POD_OutputActive is set if the driverlet is sending data to the pod.

 POD_ShortFramesOK is set if the pod will operate properly if the Control
   Port is sending shorter data frames than usual (and thus the clock
   pulses are run for a smaller fraction of an interVBL than usual).  Most
   pods can support this mode of operation.  Pod driverlets should NOT set
   this flag if their pods have the capability in hardware to change the
   width of their input or output registers "on the fly", as this would
   lead to a potential delivery of garbage data to the driverlet and/or to
   other forms of disruption of the chain.

 POD_MultipleFramesOK is set if the pod will operate properly if more than
   one Control Port transfer cycle takes place per VBL.  Most pods can use
   this mode.  Lightguns and the stereo glasses cannot.

 POD_FilteringOK is set if the pod will work properly if the Control Port
   and/or Event Broker filter out duplicate data frames without calling the
   driverlet.  Pods which set this bit tend to be both "stateless" (they do
   not reset any internal counters at VBL-pulse time) and "absolute" (they
   don't send in any "relative motion since the last VBL" sort of
   information).  The Control Pad allows filtering;  the mouse does not
   because it's not absolute _or_ stateless;  the keyboard does not because
   it is not stateless and has a handshaking protocol to implement.
*****/

#define POD_OutputActive        0x00000001
#define POD_ShortFramesOK       0x00000002
#define POD_MultipleFramesOK    0x00000004
#define POD_FilteringOK         0x00000008

enum GenericPodType {
  GENERIC_ControlPad   =  0,
  GENERIC_Mouse        =  1,
  GENERIC_Gun          =  2,
  GENERIC_GlassesCtlr  =  3,
  GENERIC_AudioCtlr    =  4,
  GENERIC_Keyboard     =  5,
  GENERIC_LightGun     =  6,
  GENERIC_Stick        =  7,
  GENERIC_IRController =  8
};

typedef struct PodDescriptions {
  EventBrokerHeader   pd_Header;
  int32               pd_Pods;
  PodDescription      pd_Pod[1];
} PodDescriptions;

/*****   EB_IssuePodCmd special information

 The EB_IssuePodCmd message is used to issue a "generic" sort of command
 to a pod driver.  For example:  any pod which is registered as a
 "generic audio controller" can be sent a "generic audio" command;
 its driver will interpret in the appropriate fashion.

 The EB_IssuePodCmd message uses a specialized form of the PodData
 structure.  The pd_Data field carries within it two bytes of
 well-defined information:

 - The generic class of this command (e.g. "this is a generic audio
   control command") is in pd_Data[0].
 - A command subcode, specific to the generic class (e.g. "this is
   a set-volume subcommand of the generic audio variety") is in pd_Data[1].

 plus as many bytes of additional command data or context as is
 required by the specific generic.

 Applications are STRONGLY ENCOURAGED to control pods via the IssuePodCmd
 mechanism, rather than by sending raw data bits to the pod via the
 WritePodData mechanism!  Applications which use the IssuePodCmd mechanism
 will continue to work with new revisions of standard and extended
 devices (as long as those devices have conformant drivers, that is),
 while applications which WritePodData directly to the pod will very
 probably need to be modified to support new revisions of devices.

*****/

enum POD_GenericAudioCommands {
  GENERIC_AUDIO_SetChannels    = 0
};

/* 
  The GENERIC_AUDIO_SetChannels command takes one byte of data
  as follows.
*/

#define AUDIO_Channels_Mute        0x00
#define AUDIO_Channels_RightToBoth 0x01
#define AUDIO_Channels_LeftToBoth  0x02
#define AUDIO_Channels_Stereo      0x03

enum POD_GenericGlassesCommands {
  GENERIC_GLASSES_SetView    = 0
};

/* 
  The GENERIC_AUDIO_SetChannels command takes two bytes of information,
  the first for the left eye and the second for the right eye.  The
  OnOddField value means that the lens should be clear during the odd
  video field and opaque during the even field;  OnEvenField vice versa.
*/

#define GLASSES_AlwaysOn           0x00
#define GLASSES_OnOddField         0x01
#define GLASSES_OnEvenField        0x02
#define GLASSES_Opaque             0x03

enum POD_KeyboardCommands {
  GENERIC_KEYBOARD_SetLEDs     = 0
};

/*
  The GENERIC_KEYBOARD_SetLEDs command takes two bytes of information,
  which set the LEDs on a keyboard.  The bit definitions below are for the
  second (least-significant) byte of the LEDs.
*/

#define KEYBOARD_LED_SCROLLLOCK    0x01
#define KEYBOARD_LED_NUMLOCK       0x02
#define KEYBOARD_LED_CAPSLOCK      0x04

enum POD_LightGunCommands {
  GENERIC_LIGHTGUN_SetLEDs      = 0
};

/*
  The GENERIC_LIGHTGUN_SetLEDs command takes one byte of data, which
  sets the LEDs on the lightgun.  LED bit settings are left-justified.
  Up to 8 LEDs are supported by this command (although the stock
  lightgun has only two LEDs), and only the leftmost two bits from this
  command are copied to the lightgun's output register).
*/

#define LIGHTGUN_LED1              0x80
#define LIGHTGUN_LED2              0x40

/*****
       Pod ID definitions.  This list is both a subset and superset of the
       IDs listed in the latest hardware spec - it reflects the devices
       which have actually been built (at least one each).  IDs in the
       spec, but not listed here, are subject to possible reassignment.

       Contact Dave Platt or Kim Pickering if you need an ID!
 *****/

#define PODID_3DO_3D_GLASSES        0x41
#define PODID_3DO_MOUSE             0x49
#define PODID_3DO_LIGHTGUN          0x4D
#define PODID_3DO_SPLITTER          0x56
#define PODID_3DO_SPLITTER_2        0x57

#define PODID_3DO_BASIC_CONTROL_PAD 0x80
#define PODID_3DO_EXTD_CONTROL_PAD  0xA0
#define PODID_3DO_SILLY_CONTROL_PAD 0xC0

/*
  Warning - the "silly" super-extended Control Pad id will not work with
  versions of the Event Broker prior to 2/24/94.  dplatt
*/

#define PODID_SPLITTER_END_CHAIN_1  0xFE
#define PODID_END_CHAIN		    0xFF

/* Extended IDs with embedded input/output length codes follow */

#define PODID_SHORTCIRCUIT_DONT_USE 0x00
#define PODID_3DO_ANALOG_JOYSTICK   0x01
#define PODID_3DO_KEYBOARD          0x02
#define PODID_3DO_IR_RECEIVER       0x03

/* Masks for detecting ROMfulness, checking for control pad, etc. */

#define POD_CONTROL_PAD_MASK        0xE0
#define POD_ROMFUL_MASK_BIT         0x80
#define POD_JJJ_MASK                0x70
#define POD_KKK_MASK                0x0F


/***** Convenience interfaces

 The following set of functions provide a simple, convenient interface
 for applications which wish to monitor the state of a small number of
 generic Control Port devices (control pads and mouse-like devices).

 To use:  call InitEventUtility and specify the maximum number of
 control pads and mice you wish to be able to monitor.  Also specify
 whether you want to be an observer or a focus listener.

 Then: at will, call GetControlPad or GetMouse, and pass in the
 number of the control pad or mouse whose current position or state
 you wish to interrogate (the first pad is 1, not 0!).  You may
 specify whether you wish an immediate response, or whether you wish
 to wait for the next change in the control pad or mouse status
 before the value is returned.  These functions return 1 if an event
 has occured, 0 if no event occurred, and a negative error code if
 something went amiss.

 When you are done, call KillEventUtility to close down.

 If you init as a focus listener, you will not be told of any control pad
 or mouse changes when you do not have focus... and so you might
 "believe" that a mouse or control pad button is still down, long after
 it has been released.  So... if you are signing on as a focus listener
 through this interface, you'd probably want to call GetMouse and
 GetControlPad with wait=1, so that you would in effect "go to sleep"
 whenever you don't have the focus.

*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
Err InitEventUtility (int32 numControlPads, int32 numMice,
		      int32 focusListener);

Err GetControlPad(int32 padNumber, int32 wait, ControlPadEventData *data);
Err GetMouse(int32 mouseNumber, int32 wait, MouseEventData *data);

Err KillEventUtility(void);

#define	ER_EVBR	((Make6Bit('E')<<6)|Make6Bit('B'))
#define MAKEEB(svr,class,err) MakeErr(ER_TASK,ER_EVBR,svr,ER_E_SSTM,class,err)

#define ER_TEV_NoSuch            1

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

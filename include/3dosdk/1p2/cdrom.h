/*****

$Id: cdrom.h,v 1.14 1993/07/22 07:01:55 dale Exp $

$Log: cdrom.h,v $
 * Revision 1.14  1993/07/22  07:01:55  dale
 * CDROM_READ_DEVICE_TYPE changed to CDROM_READ_DEVICE_DRIVER
 *
 * Revision 1.13  1993/07/15  03:36:58  dplatt
 * Nah... just turn logging off completely in the #include file.  Can be
 * overridden by Makefile if we ever need to.
 *
 * Revision 1.12  1993/07/15  03:30:09  dplatt
 * In PRODUCTION mode, do not include the 3k-byte CD-ROM activity log
 * array.
 *
 *****/

/*
  Copyright 3DO Company, 1992.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#pragma include_only_once

#ifndef CDROM_H
#define CDROM_H

/* #define CDLOG 256 */

#define CDROMCMD_PASSTHROUGH     3
#define CDROMCMD_DISCDATA        4
#define CDROMCMD_SETDEFAULTS     5
#define CDROMCMD_READ_SUBQ       6
#define CDROMCMD_READ_DISC_CODE  7
#define CDROMCMD_OPEN_DRAWER     8
#define CDROMCMD_CLOSE_DRAWER    9

enum MEI_CDROM_Commands {
  CDROM_SEEK                     = 0x01,
  CDROM_SPIN_UP                  = 0x02,
  CDROM_SPIN_DOWN                = 0x03,
  CDROM_DIAGNOSTICS              = 0x04,
  CDROM_DRAWER_OPEN              = 0x06,
  CDROM_DRAWER_CLOSE             = 0x07,
  CDROM_ABORT                    = 0x08,
  CDROM_MODE_SET                 = 0x09,
  CDROM_RESET                    = 0x0A,
  CDROM_FLUSH                    = 0x0B,
  CDROM_READ_DATA                = 0x10,
  CDROM_DATA_PATH_CHECK          = 0x80,
  CDROM_READ_ERROR               = 0x82,
  CDROM_READ_IDENTIFICATION      = 0x83,
  CDROM_MODE_SENSE               = 0x84,
  CDROM_READ_CAPACITY            = 0x85,
  CDROM_READ_HEADER              = 0x86,
  CDROM_READ_SUBQ                = 0x87,
  CDROM_READ_UPC                 = 0x88,
  CDROM_READ_ISRC                = 0x89,
  CDROM_READ_DISC_CODE           = 0x8A,
  CDROM_READ_DISC_INFORMATION    = 0x8B,
  CDROM_READ_TOC                 = 0x8C,
  CDROM_READ_SESSION_INFORMATION = 0x8D,
  CDROM_READ_DEVICE_DRIVER       = 0x8E
};

/********
 In all of the following enum classes, a value of 0 means "not
 specified by caller, use the driver default".
********/

#define CDROM_Option_Unspecified 0

enum CDROM_DensityCodes {
  CDROM_DEFAULT_DENSITY          = 1,
  CDROM_DATA                     = 2,
  CDROM_MODE2_XA                 = 3,
  CDROM_DIGITAL_AUDIO            = 4
};

enum MEI_CDROM_DensityCodes {
  MEI_CDROM_DEFAULT_DENSITY          = 0x00,
  MEI_CDROM_DATA                     = 0x01,
  MEI_CDROM_MODE2_XA                 = 0x81,
  MEI_CDROM_DIGITAL_AUDIO            = 0x82
};

enum CDROM_Error_Recovery {
  CDROM_DEFAULT_RECOVERY         = 1,
  CDROM_CIRC_RETRIES_ONLY        = 2,
  CDROM_BEST_ATTEMPT_RECOVERY    = 3
};

enum MEI_CDROM_Error_Recovery {
  MEI_CDROM_DEFAULT_RECOVERY         = 0x00,
  MEI_CDROM_CIRC_RETRIES_ONLY        = 0x01,
  MEI_CDROM_BEST_ATTEMPT_RECOVERY    = 0x20
};

enum CDROM_Speed {
  CDROM_SINGLE_SPEED             = 1,
  CDROM_DOUBLE_SPEED             = 2
};

enum MEI_CDROM_SPEED {
  MEI_CDROM_SINGLE_SPEED             = 0x00,
  MEI_CDROM_DOUBLE_SPEED             = 0x80
};

enum CDROM_Readahead {
  CDROM_READAHEAD_ENABLED             = 1,
  CDROM_READAHEAD_DISABLED            = 2
};

enum CDROM_Pitch {
  CDROM_PITCH_SLOW                    = 1,
  CDROM_PITCH_NORMAL                  = 2,
  CDROM_PITCH_FAST                    = 3
};

/*
  The block-size definitions need to be expanded and re-thought.
*/

enum CDROM_Block_Sizes {
  CDROM_M1_D                  = 2048,
  CDROM_DA                    = 2352,
  CDROM_DA_PLUS_ERR           = 2353,
  CDROM_DA_PLUS_SUBCODE       = 2448,
  CDROM_DA_PLUS_BOTH          = 2449
};

enum MEI_DISCID {
  MEI_DISC_DA_OR_CDROM           = 0x00,
  MEI_DISC_CDI                   = 0x10,
  MEI_DISC_CDROM_XA              = 0x20
};

struct CDROM_BlockParameters {
  uint8              densityCode;
  uint8              lengthMSB;
  uint8              lengthLSB;
  uint8              flags;
  uint32             userBlockSize;
};

struct CDROM_ErrorRecoveryParameters {
  uint8              type;
  uint8              retryCount;
};

struct CDROM_StopTimeParameters {
  uint8              time;
};

struct CDROM_DriveSpeedParameters {
  uint8              speed; /* 0x00 or 0x80 */
  uint8              vpitchMSB;
  uint8              vpitchLSB;
};

struct CDROM_ChunkSizeParameters {
  uint8              size; /* 1 to 8 */
};

typedef struct CDROM_Parameters {
  struct CDROM_BlockParameters block;
  struct CDROM_ErrorRecoveryParameters errorRecovery;
  struct CDROM_StopTimeParameters stopTime;
  struct CDROM_DriveSpeedParameters driveSpeed;
  struct CDROM_ChunkSizeParameters chunkSize;
} CDROM_Parameters;

struct CDROM_MSF {
  uint8              rfu;
  uint8              minutes;
  uint8              seconds;
  uint8              frames;
};

enum CDROMAddressFormat {
  CDROM_Address_Blocks         = 0,
  CDROM_Address_Abs_MSF        = 1,
  CDROM_Address_Track_MSF      = 2
};

/*
  N.B.  The "retryShift" field is meaningful if and only if the errorRecovery
  field contains a meaningful value.  Specifying 0 for the retryShift field
  does _not_ mean "use the default value"!  The default value for retries
  is used iff the errorRecovery field contains the default.
*/

union CDROMCommandOptions {
  uint32 asLongword;
  struct {
    unsigned int     densityCode   : 3;
    unsigned int     errorRecovery : 2;
    unsigned int     addressFormat : 2;
    unsigned int     readAhead     : 2;
    unsigned int     timeoutShift  : 4; /* 2^N millisecond timeout */
    unsigned int     retryShift    : 3; /* (2^N)-1 retries */
    unsigned int     speed         : 2;
    unsigned int     pitch         : 2;
    unsigned int     blockLength   : 12;
  } asFields;
};

enum CDROM_Level0_FSM {
  CDROM_Level0_Idle                = 0,
  CDROM_Level0_Command             = 1,
  CDROM_Level0_Data                = 2,
  CDROM_Level0_CommandData         = 3,
  CDROM_Level0_KillData            = 4,
  CDROM_Level0_EndAction           = 5,
  CDROM_Level0_Holdoff             = 6,
  CDROM_Level0_EndActionReadahead  = 7,
  CDROM_Level0_Readahead           = 8
};

enum CDROM_Level1_FSM {
  CDROM_Level1_Idle                = 0,
  CDROM_Level1_Running             = 1,
  CDROM_Level1_Initialize          = 2
};

enum CDROM_Level2_FSM {
  CDROM_Level2_Idle                = 0,
  CDROM_Level2_Wake_Up             = 1,
  CDROM_Level2_Check_Data_Path     = 2,
  CDROM_Level2_Spin_Up             = 3,
  CDROM_Level2_Read_Disc_ID        = 4,
  CDROM_Level2_Read_TOC_Entry      = 5,
  CDROM_Level2_Read_Session        = 6,
  CDROM_Level2_Read_Capacity       = 7,
  CDROM_Level2_Await_Request       = 8,
  CDROM_Level2_Prepare_Request     = 9,
  CDROM_Level2_Run_Request         = 10,
  CDROM_Level2_Finish_Request      = 11,
  CDROM_Level2_Abort_Readahead     = 12,
  CDROM_Level2_Flush_FIFO          = 13,
  CDROM_Level2_Spin_Down           = 14,
  CDROM_Level2_Open_Drawer         = 15,
  CDROM_Level2_Kill_Pending_IOs    = 16,
  CDROM_Level2_Sluggish            = 17,
  CDROM_Level2_Poke_Drive          = 18
};

enum CDROM_Disc_Available {
  CDROM_Disc_Not_Available         = 0,
  CDROM_Disc_Available             = 1,
  CDROM_Disc_Availability_Unknown  = 2
};

enum MEI_CDROM_Error_Codes {
  MEI_CDROM_no_error = 0x00,
  MEI_CDROM_recv_retry = 0x01,
  MEI_CDROM_recv_ecc = 0x02,
  MEI_CDROM_not_ready = 0x03,
  MEI_CDROM_toc_error = 0x04,
  MEI_CDROM_unrecv_error = 0x05,
  MEI_CDROM_seek_error = 0x06,
  MEI_CDROM_track_error = 0x07,
  MEI_CDROM_ram_error = 0x08,
  MEI_CDROM_diag_error = 0x09,
  MEI_CDROM_focus_error = 0x0A,
  MEI_CDROM_clv_error = 0x0B,
  MEI_CDROM_data_error = 0x0C,
  MEI_CDROM_address_error = 0x0D,
  MEI_CDROM_cdb_error = 0x0E,
  MEI_CDROM_end_address = 0x0F,
  MEI_CDROM_mode_error = 0x10,
  MEI_CDROM_media_changed = 0x11,
  MEI_CDROM_hard_reset = 0x12,
  MEI_CDROM_rom_error = 0x13,
  MEI_CDROM_cmd_error = 0x14,
  MEI_CDROM_disc_out = 0x15,
  MEI_CDROM_hardware_error = 0x16,
  MEI_CDROM_illegal_request = 0x17
};


struct CDROM_Disc_Information {
  uint8              discID;
  uint8              firstTrackNumber;
  uint8              lastTrackNumber;
  uint8              minutes;
  uint8              seconds;
  uint8              frames;
};

struct CDROM_TOC_Entry {
  uint8              reserved0;
  uint8              addressAndControl;
  uint8              trackNumber;
  uint8              reserved3;
  uint8              minutes;
  uint8              seconds;
  uint8              frames;
  uint8              reserved7;
};

struct CDROM_Session_Information {
  uint8              valid;
  uint8              minutes;
  uint8              seconds;
  uint8              frames;
  uint8              rfu[2];
};

struct CDROM_Disc_Data {
  struct CDROM_Disc_Information     info;
  struct CDROM_TOC_Entry            TOC[100];
  struct CDROM_Session_Information  session;
};

#define CD_CTL_PREEMPHASIS    0x01
#define CD_CTL_COPY_PERMITTED 0x02
#define CD_CTL_DATA_TRACK     0x04
#define CD_CTL_FOUR_CHANNEL   0x08
#define CD_CTL_QMASK          0xF0
#define CD_CTL_Q_NONE         0x00
#define CD_CTL_Q_POSITION     0x10
#define CD_CTL_Q_MEDIACATALOG 0x20
#define CD_CTL_Q_ISRC         0x30

typedef struct CDROM {
  Device             cdrom;
  uchar              cdrom_PushParameters;
  List               cdrom_RequestsToDo;
  IOReq             *cdrom_RequestRunning;
  IOReq             *cdrom_CommandStatusReq;
  IOReq             *cdrom_DataReq;
  uint32             cdrom_LastErrorCode;
  uint32             cdrom_VendorType;
  uint32             cdrom_DeviceMediaChangeCntr;
  int32              cdrom_Holdoff_MS;
  int32              cdrom_Cooldown_MS;
  uint32             cdrom_MediumBlockCount;
  int32              cdrom_NextBlockOffset;
  int32              cdrom_BlocksPerDataXfer;
  uint32             cdrom_TrackToProbe;
  uint32             cdrom_ReadRemaining;
  uint8              cdrom_CommandBuffer[8];
  uint8              cdrom_StatusBuffer[32];
  uint8              cdrom_ErrorBuffer[12];
  uint8              cdrom_RetrysPermitted;
  uint8              cdrom_Level0;
  uint8              cdrom_Level1;
  uint8              cdrom_Level2;
  uint8              cdrom_MustReadError;
  uint8              cdrom_DoingReadError;
  uint8              cdrom_DiscAvailable;
  uint8              cdrom_DiscInfoAvailable;
  uint8              cdrom_Initialized;
  uint8              cdrom_DoReadahead;
  uint8              cdrom_ReadaheadActive;
  uint8              cdrom_MustFlush;
  uint8              cdrom_StatusByte;
  uint8              cdrom_StatusBytesRead;
  uint8              cdrom_TimeoutClock;
  uint8              cdrom_TimedOut;
  uint8              cdrom_XBusUnit;
  uint8              cdrom_ErrorCodeRead;
  CDROM_Parameters   cdrom_CurrentParameters;
  CDROM_Parameters   cdrom_DesiredParameters;
  struct CDROM_Disc_Information
                     cdrom_DiscInformation;  
  struct CDROM_TOC_Entry
                     cdrom_TOC_Entry[100];
  struct CDROM_Session_Information
                     cdrom_SessionInformation;
#ifdef CDLOG
  uint32             cdrom_LogIndex;
  struct {
    uint32           index;
    uint8            actor;
    uint8            level0;
    uint8            level1;
    uint8            level2;
    uint8            doRA;
    uint8            raActive;
  } cdrom_Log[CDLOG];
#endif
} CDROM;

#define CDROM_TIMED_OUT_ONCE  0x40000000
#define CDROM_TIMED_OUT_TWICE 0x20000000

#define XBUS_DEVICE_CDROM        1
#define XBUS_MFG_MEI             16

#define CDROM_VENDOR_CODE_MEI1    0x4d454931

#define CDROM_STATUS_DOOR         0x80
#define CDROM_STATUS_DISC_IN      0x40
#define CDROM_STATUS_SPIN_UP      0x20
#define CDROM_STATUS_ERROR        0x10
#define CDROM_STATUS_DOUBLE_SPEED 0x02
#define CDROM_STATUS_READY        0x01

typedef struct CDROMDriveQueueEntry {
  Node        cdrdqe;
  CDROM      *cdrdqe_Device;
} CDROMDriveQueueEntry;

#endif

#ifndef __CDROM_H
#define __CDROM_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: cdrom.h,v 1.20 1994/11/22 18:38:30 shawn Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif


/* #define CDLOG 256 */
#define CDROM_TICKLE_DELAY       2

#define CDROMCMD_PASSTHROUGH     3
#define CDROMCMD_DISCDATA        4
#define CDROMCMD_SETDEFAULTS     5
#define CDROMCMD_READ_SUBQ       6
#define CDROMCMD_READ_DISC_CODE  7
#define CDROMCMD_OPEN_DRAWER     8
#define CDROMCMD_CLOSE_DRAWER    9


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


enum CDROM_Error_Recovery {
  CDROM_DEFAULT_RECOVERY         = 1,
  CDROM_CIRC_RETRIES_ONLY        = 2,
  CDROM_BEST_ATTEMPT_RECOVERY    = 3
};


enum CDROM_Speed {
  CDROM_SINGLE_SPEED             = 1,
  CDROM_DOUBLE_SPEED             = 2
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
} CDROM;

#define CDROM_STATUS_DOOR         0x80
#define CDROM_STATUS_DISC_IN      0x40
#define CDROM_STATUS_SPIN_UP      0x20
#define CDROM_STATUS_ERROR        0x10
#define CDROM_STATUS_DOUBLE_SPEED 0x02
#define CDROM_STATUS_READY        0x01



/*****************************************************************************/


#endif /* __CDROM_H */

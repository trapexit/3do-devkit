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
  podrom.h - definitions for the header of a downloadable pod driverlet
*/

#ifndef AIF_H
# include "aif.h"
#endif

typedef struct PodROM {
  struct {
    uint32                  prh_HeaderChecksum;
    uint32	            prh_TotalByteCount;
    uint32	            prh_FamilyCode;
    uint32	            prh_FamilyVersion;
    uint32                  prh_ImageAreaChecksum;
    uint32                  prh_RFU_MBZ[3];
  } pr_Header;
  struct AIFHeader        pr_aif;
} PodROM;

#define PODROM_CompleteReset 0x23
#define PODROM_NoUpload      0x94
#define PODROM_UploadIncr    0x89
#define PODROM_UploadDecr    0x52

#define PODROM_CHECKSUM_MAGIC_BITS   0xDEADBEEF

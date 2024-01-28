/*****

$Id: discdata.h,v 1.10 1994/01/21 00:26:16 dplatt Exp $

$Log: discdata.h,v $
 * Revision 1.10  1994/01/21  00:26:16  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.11  1994/01/19  22:09:14  dplatt
 * Move or eliminate C++ wrappers wrongly placed
 *
 * Revision 1.10  1994/01/19  20:29:26  dplatt
 * Add C++ wrappers
 *
 * Revision 1.9  1993/08/19  01:25:32  dplatt
 * Set maximum disc blocks to 330,000 (73 minutes 40 seconds) rather than
 * 300,000 (66 minutes 40 seconds).  This is pushing the envelope,
 * folks.
 *
 * Revision 1.8  1993/07/30  02:44:48  stan
 * Fixed bad comment by Bryce
 *
 * Revision 1.7  1993/07/29  05:28:37  bryce
 * Add new ADVISORY flag indicating this CD is a data disc capable CD.
 * Dipir will perform an extra check to be sure.
 *
 * Revision 1.6  1993/07/03  00:31:06  dplatt
 * Change label mechanisms to keep the lawyers happy
 *
 * Revision 1.5  1993/06/15  03:58:58  dplatt
 * Fix typo
 *
 * Revision 1.4  1993/05/08  01:05:08  dplatt
 * Add NVRAM support and recover RCS
 *
 * Revision 1.3  1992/12/08  05:58:20  dplatt
 * Magenta changes
 *
 * Revision 1.2  1992/10/01  23:35:00  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:44:14  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#ifndef __H_DISCDATA
#define __H_DISCDATA

#pragma include_only_once

/*
  Define the position of the primary label on each Opera disc, the
  block offset between avatars, and the index of the last avatar
  (i.e. the avatar count minus one).  The latter figure *must* match
  the ROOT_HIGHEST_AVATAR figure from "filesystem.h", as the same
  File structure is use to read the label at boot time, and to provide
  access to the root directory.
*/

#define DISC_BLOCK_SIZE           2048
#define DISC_LABEL_OFFSET         225
#define DISC_LABEL_AVATAR_DELTA   32786
#define DISC_LABEL_HIGHEST_AVATAR 7
#define DISC_TOTAL_BLOCKS         330000

#define ROOT_HIGHEST_AVATAR       7
#define FILESYSTEM_MAX_NAME_LEN   32

#define VOLUME_STRUCTURE_OPERA_READONLY    1
#define VOLUME_STRUCTURE_LINKED_MEM        2

#define VOLUME_SYNC_BYTE          0x5A

/*
// This disc won't necessarily cause a reboot when inserted.  This flag is
// advisory ONLY. Only by checking with cdromdipir can you be really sure.
// Place in dl_VolumeFlags.  Note: the first volume gets this flag also.
*/
#define	VOLUME_FLAGS_DATADISC	0x01

/*
  Data structures written on CD disc (Compact Disc disc?)
*/
typedef struct DiscLabel {
  uchar    dl_RecordType;                   /* Should contain 1 */
  uint8    dl_VolumeSyncBytes[5];           /* Synchronization byte */
  uchar    dl_VolumeStructureVersion;       /* Should contain 1 */
  uchar    dl_VolumeFlags;                  /* Should contain 0 */
  uchar    dl_VolumeCommentary[32];         /* Random comments about volume */
  uchar    dl_VolumeIdentifier[32];         /* Should contain disc name */
  uint32   dl_VolumeUniqueIdentifier;       /* Roll a billion-sided die */
  uint32   dl_VolumeBlockSize;              /* Usually contains 2048 */
  uint32   dl_VolumeBlockCount;             /* # of blocks on disc */
  uint32   dl_RootUniqueIdentifier;         /* Roll a billion-sided die */
  uint32   dl_RootDirectoryBlockCount;      /* # of blocks in root */
  uint32   dl_RootDirectoryBlockSize;       /* usually same as vol blk size */
  uint32   dl_RootDirectoryLastAvatarIndex; /* should contain 7 */
  uint32   dl_RootDirectoryAvatarList[ROOT_HIGHEST_AVATAR+1];
} DiscLabel;

typedef struct DirectoryHeader {
  int32      dh_NextBlock;
  int32      dh_PrevBlock;
  uint32     dh_Flags;
  uint32     dh_FirstFreeByte;
  uint32     dh_FirstEntryOffset;
} DirectoryHeader;

#define DIRECTORYRECORD(AVATARCOUNT) \
  uint32     dir_Flags; \
  uint32     dir_UniqueIdentifier; \
  uint32     dir_Type; \
  uint32     dir_BlockSize; \
  uint32     dir_ByteCount; \
  uint32     dir_BlockCount; \
  uint32     dir_Burst; \
  uint32     dir_Gap; \
  char       dir_FileName[FILESYSTEM_MAX_NAME_LEN]; \
  uint32     dir_LastAvatarIndex; \
  uint32     dir_AvatarList[AVATARCOUNT];

typedef struct DirectoryRecord {
  DIRECTORYRECORD(1)
} DirectoryRecord;

#define DIRECTORY_LAST_IN_DIR        0x80000000
#define DIRECTORY_LAST_IN_BLOCK      0x40000000

#endif

#ifndef __H_DISCDATA
#define __H_DISCDATA

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: discdata.h,v 1.16 1994/10/25 15:57:33 vertex Exp $
**
******************************************************************************/


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


#define DIRECTORY_LAST_IN_DIR        0x80000000
#define DIRECTORY_LAST_IN_BLOCK      0x40000000


/*****************************************************************************/


#endif /* __H_DISCDATA */

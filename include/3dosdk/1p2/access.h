#pragma force_top_level
#pragma include_only_once

/* *************************************************************************
 *
 * Access Public Include File
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with any tab stops from 1 to 8
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 980717 -Niki Woo        Added Shadow Pen and Event / Graphics Flags
 * 930709 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


#ifndef __ACCESS_H
#define __ACCESS_H

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "item.h"
#include "list.h"
#include "operror.h"

#include "filesystem.h"
#include "filesystemdefs.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "graphics.h"
#include "hardware.h"

/* ===  =========  ====================================================== */
/* ===             ====================================================== */
/* ===  Constants  ====================================================== */
/* ===             ====================================================== */
/* ===  =========  ====================================================== */

#define ACCTAG_REQUEST          1
#define ACCTAG_SCREEN           2
#define ACCTAG_BUFFER           3
#define ACCTAG_BUFFERSIZE       4
#define ACCTAG_TITLE            5
#define ACCTAG_TEXT             6
#define ACCTAG_BUTTON_ONE       7
#define ACCTAG_BUTTON_TWO       8
#define ACCTAG_SAVE_BACK        9
#define ACCTAG_STRINGBUF       10
#define ACCTAG_STRINGBUF_SIZE  11
#define ACCTAG_FG_PEN          12
#define ACCTAG_BG_PEN          13
#define ACCTAG_HILITE_PEN      14
#define ACCTAG_SHADOW_PEN      15	// еее Shadow Pen Tag @NICK

#define ACCREQ_LOAD       1
#define ACCREQ_SAVE       2
#define ACCREQ_DELETE     3
#define ACCREQ_OK         4
#define ACCREQ_OKCANCEL   5
#define ACCREQ_ONEBUTTON  6
#define ACCREQ_TWOBUTTON  7



/* ===  ======  ========================================================= */
/* ===          ========================================================= */
/* ===  Macros  ========================================================= */
/* ===          ========================================================= */
/* ===  ======  ========================================================= */



/* ===  ===============  ================================================ */
/* ===                   ================================================ */
/* ===  Data Structures  ================================================ */
/* ===                   ================================================ */
/* ===  ===============  ================================================ */



/* ===  =================  ============================================== */
/* ===                     ============================================== */
/* ===  Error Definitions  ============================================== */
/* ===                     ============================================== */
/* ===  =================  ============================================== */

#define ACCERR_BAD_REQUEST  -5001 // Unknown request argument
#define ACCERR_NO_SCREEN    -5002 // No screen specified
#define ACCERR_BAD_ARGS     -5003 // Something abotu the args was bad



/* ===  ===============================  ================================ */
/* ===                                   ================================ */
/* ===  Externs and Function Prototypes  ================================ */
/* ===                                   ================================ */
/* ===  ===============================  ================================ */



#endif  /* of #define __ACCESS_H */

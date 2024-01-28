/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
**
**	Module: l3Error.h
**	Application: l3 library/folio
**	Date: 4/93
**	Author: ccw
**
**
**
**
*/

#ifndef __L3ERROR
#define __L3ERROR

typedef enum
{
  ERR_PRINTIT,
  ERR_ALLOCMEM,
  ERR_NOGRAPHICS,
  ERR_LOCKMEM,
  ERR_FILEOPEN,
  ERR_FILEEXIST,
  ERR_FILECREATE
} L3ERROR;

extern int l3Error (L3ERROR e, char *szFile, int nLineNo, void *p);

#define L3ERROR(e, p) l3Error (e, __FILE__, __LINE__, p)

#endif

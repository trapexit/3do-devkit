/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1
 *with
 **							squareroot-delta-exactencoding and
 *write an AIFC file.
 **							Throw out all chunks but SSND FORM
 *COMM MARK INST FVER
 **
 **	This File:		iff_tools.h
 **
 ** Contains:		Include file for simple IFF reader
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and
 *proprietary *	information of the 3DO Company and shall not be used by any
 *Person or for any *	purpose except as expressly authorized in writing by
 *the 3DO Company.
 **
 **
 **  History:
 **  -------
 **	 6/17/93	RDG		version 1.0
 **  6/17/93	RDG		added iff_getpos
 **	 6/15/93	RDG		version .12
 **	 6/15/93	RDG		modify to use MacFileIO routines
 **	 2/16/93	RDG		ported to back to MPW 3.2.3 and release
 *first version .10
 **  2/15/93	RDG		added iff_begin_snd and iff_end_snd to more
 *easily handle SSND
 **						 chunk size... also modified the
 *iff_control struct by adding
 **						 member iffc_ssnd_pos to hold file
 *position of the ssnd data *	 2/7/93		RDG		added
 *uint,uint32,int32 typedefs
 **	 2/7/93		RDG		added iff_get_16 and put_8
 **	 2/4/93		RDG		ported to think C
 **
 **************************************************************************************/

#include "iff_tools.h"
#include "SquashSnd.h"
#include <stdio.h>
#include <types.h>

#define TRUE 1
#define FALSE 0

#define PRT(x) printf x
#define ERR(x) PRT (x)
#define DBUG(x) /*PRT(x) */

#define IFFREAD(buf, len)                                                     \
  Result = mfread ((char *)buf, 1, len, iffc->iffc_fileid);                   \
  iffc->iffc_length -= len

#define IFFWRITE(buf, len)                                                    \
  Result = mfwrite ((char *)buf, 1, len, iffc->iffc_fileid);                  \
  iffc->iffc_length += len

#define EVENUP(n)                                                             \
  if (n & 1)                                                                  \
  n++

/******************************************************/
long
iff_open_file (iff_control *iffc, char *filename)
{
  mfopen (filename, &iffc->iffc_fileid);
  return (iffc->iffc_fileid == NULL);
}

/******************************************************/
long
iff_create_file (iff_control *iffc, char *filename, long creator,
                 long fileType)
{
  mfcreate (filename, creator, fileType, &iffc->iffc_fileid);
  return (iffc->iffc_fileid == NULL);
}

/******************************************************/
long
iff_rewind (iff_control *iffc)
{
  long Result = 0;
  fpos_t OldPos, NewPos;

  if (iffc->iffc_fileid != 0)
    {
      Result = mfgetpos (iffc->iffc_fileid, &OldPos);
      Result = mfseek (iffc->iffc_fileid, 12, MSEEK_START);
      Result = mfgetpos (iffc->iffc_fileid, &NewPos);
      iffc->iffc_length = iffc->iffc_length + ((int32)OldPos - (int32)NewPos);
      return (FALSE);
    }
  else
    {
      return (TRUE);
    }
}

/******************************************************/
long
iff_close_file (iff_control *iffc)
{
  long Result = 0;

  if (iffc->iffc_fileid != 0)
    {
      Result = mfclose (iffc->iffc_fileid);
      iffc->iffc_fileid = 0;
      return (Result);
    }
  else
    {
      return (TRUE);
    }
}

static int32 Str[2] = { 0, 0 };

char *
TypeToString (uint32 type)
{

  Str[0] = type;
  return ((char *)&Str[0]);
}

/******************************************************/
long
iff_getpos (iff_control *iffc)
{
  long Result = 0;
  fpos_t CurPos;

  Result = mfgetpos (iffc->iffc_fileid, &CurPos);

  if (!Result)
    return CurPos;
  else
    return Result;
}

/******************************************************/
long
iff_begin_form (iff_control *iffc, uint32 type)
{
  uint32 pad[3];
  long Result = 0;

  /* load pad with data to be written */
  pad[0] = ID_FORM;
  pad[1] = 0x3FFFFFFF; /* Default set to very long for TAPE: */
  pad[2] = type;
  iffc->iffc_length = 4;

  Result = mfwrite ((char *)&pad, 1, 12, iffc->iffc_fileid);

  return (Result != 12);
}

/******************************************************/
long
iff_end_form (iff_control *iffc)
{
  uint32 pad;
  long Result = 0;

  /* load pad with data to be written */
  pad = iffc->iffc_length;

  /* try to seek to FORM size field */
  Result = mfseek (iffc->iffc_fileid, 4, MSEEK_START);
  if (Result)
    return (2);

  Result = mfwrite ((char *)&pad, 1, 4, iffc->iffc_fileid);

  return (Result != 4);
}

/******************************************************/
long
iff_write_chunk (iff_control *iffc, uint32 type, void *data, int32 numbytes)
{
  uint32 pad[2];
  long Result = 0;

  DBUG (("iff_write_chunk - %d\n", numbytes));

  /* load pad with data to be written */
  pad[0] = type;
  pad[1] = numbytes;

  IFFWRITE ((char *)&pad, 8);
  if (Result != 8)
    return (TRUE);

  EVENUP (numbytes);
  if (numbytes > 0)
    IFFWRITE (data, numbytes);
  return (Result != numbytes);
}

/******************************************************
**
**    Read the type and size of a FORM
**
******************************************************/
long
iff_read_form (iff_control *iffc, uint32 *type)
{
  long Result = 0;
  uint32 pad[3];

  Result = mfread ((char *)pad, 1, 12, iffc->iffc_fileid);

  DBUG (("iff_read_form - %d, %x, %d, %s\n", Result, pad[0], pad[1],
         TypeToString (pad[2])));

  *type = pad[2];
  iffc->iffc_length = pad[1] - 4; /* account for FORM type */

  if (Result != 12)
    return (3);

  if (pad[0] != ID_FORM)
    return (2);

  return (FALSE);
}

/******************************************************
**
**    Read the type and size of a CHUNK
**
******************************************************/
long
iff_read_chunk_header (iff_control *iffc, uint32 *type, uint32 *size)
{
  uint32 pad[2];
  long Result = 0;

  IFFREAD (pad, 8);

  *type = pad[0];
  *size = pad[1];

  return (Result != 8);
}

/******************************************************
**
**    Read the data from a CHUNK
**
******************************************************/
long
iff_read_chunk_data (iff_control *iffc, void *data, uint32 numbytes)
{
  long Result = 0;

  EVENUP (numbytes);
  IFFREAD (data, numbytes);

  return (Result != numbytes);
}

/******************************************************
**
**    Skip the data in a CHUNK
**
******************************************************/
long
iff_skip_chunk_data (iff_control *iffc, uint32 numbytes)
{
  long Result = 0;

  EVENUP (numbytes);
  Result = mfseek (iffc->iffc_fileid, numbytes, MSEEK_CUR);

  iffc->iffc_length -= numbytes;
  return (Result = 0);
}

/******************************************************/
long
iff_begin_ssnd (iff_control *iffc)
{
  long Result = 0;
  uint32 pad[2]
      = { 0, 0 }; /*  these represent offset and blockSize which are unused  */

  Result = iff_write_chunk (iffc, ID_SSND, &pad, 8);

  Result = mfgetpos (iffc->iffc_fileid, &iffc->iffc_ssnd_pos);

  return (Result);
}

/******************************************************/
long
iff_end_ssnd (iff_control *iffc)
{
  long Result = 0, pad_byte = 0, ssnd_size, cur_pos;

  /* calculate size of compressed SSND chunk */
  Result = mfgetpos (iffc->iffc_fileid, &cur_pos);
  ssnd_size
      = (8
         + (cur_pos
            - iffc->iffc_ssnd_pos)); /*offset and blockSize + raw snd data */

  if (ssnd_size & 1) /* Evenup size of SSND chunk */
    {
      IFFWRITE (&pad_byte, 1);
      ++cur_pos;
    }

  Result = mfseek (iffc->iffc_fileid, (iffc->iffc_ssnd_pos - 12), MSEEK_START);
  if (Result)
    return (TRUE);
  Result = mfwrite ((char *)&ssnd_size, 1, 4, iffc->iffc_fileid);
  if (Result != 4)
    return (TRUE);
  Result = mfseek (iffc->iffc_fileid, cur_pos, MSEEK_START);
  if (Result)
    return (TRUE);
}

/******************************************************
**
**    Read a block of sample data from a SSND CHUNK
**
******************************************************/
long
iff_get_block (iff_control *iffc, char *sampleData, long size)
{
  long Result = 0;

  IFFREAD (sampleData, size);

  return (Result);
}

/******************************************************
**
**    Write a block of sample data from a SSND CHUNK
**
******************************************************/
long
iff_put_block (iff_control *iffc, char *sampleData, long size)
{
  long Result = 0;

  IFFWRITE (sampleData, size);

  return (Result);
}

/******************************************************
**
**    Read a 16bit sample from a SSND CHUNK
**
******************************************************/
long
iff_get_16 (iff_control *iffc, short *sample16)
{
  long Result = 0;

  IFFREAD (sample16, 2);

  return (Result != 2);
}

/******************************************************
**
**    write a 8bit sample to a SSND CHUNK
**
******************************************************/
long
iff_put_8 (iff_control *iffc, char sample8)
{
  long Result = 0;

  IFFWRITE (&sample8, 1);

  return (Result != 1);
}

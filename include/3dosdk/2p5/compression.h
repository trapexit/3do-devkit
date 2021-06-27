#ifndef __COMPRESSION_H
#define __COMPRESSION_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: compression.h,v 1.6 1995/02/14 00:53:19 vertex Exp $
**
**  Compression folio interface definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __OPERROR_H
#include "operror.h"
#endif


/****************************************************************************/


/* kernel interface definitions */
#define COMP_FOLIONAME  "compression"


/*****************************************************************************/


typedef void (* CompFunc)(void *userData, uint32 word);

#ifndef __COMPRESSION_PRIVATE
typedef void Compressor;
typedef void Decompressor;
#endif


/*****************************************************************************/


/* Error codes */

#define MakeCompErr(svr,class,err) MakeErr(ER_FOLI,ER_COMP,svr,ER_E_SSTM,class,err)

/* Bad Compressor/Decompressor parameter */
#define COMP_ERR_BADPTR       MakeCompErr(ER_SEVERE,ER_C_STND,ER_BadPtr)

/* Unknown tag supplied */
#define COMP_ERR_BADTAG       MakeCompErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* No memory */
#define COMP_ERR_NOMEM        MakeCompErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* More data than needed */
#define COMP_ERR_DATAREMAINS  MakeCompErr(ER_SEVERE,ER_C_NSTND,1)

/* Not enough data */
#define COMP_ERR_DATAMISSING  MakeCompErr(ER_SEVERE,ER_C_NSTND,2)

/* Too much data for target buffer */
#define COMP_ERR_OVERFLOW     MakeCompErr(ER_SEVERE,ER_C_NSTND,3)


/*****************************************************************************/


/* for use with CreateCompressor() and CreateDecompressor() */
typedef enum CompressionTags
{
    COMP_TAG_WORKBUFFER = TAG_ITEM_LAST,
    COMP_TAG_USERDATA
} CompressionTags;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenCompressionFolio(void);
Err CloseCompressionFolio(void);

/* compressor */
Err CreateCompressor(Compressor **comp, CompFunc cf, const TagArg *tags);
Err DeleteCompressor(Compressor *comp);
Err FeedCompressor(Compressor *comp, void *data, uint32 numDataWords);
int32 GetCompressorWorkBufferSize(const TagArg *tags);

/* decompressor */
Err CreateDecompressor(Decompressor **decomp, CompFunc cf, const TagArg *tags);
Err DeleteDecompressor(Decompressor *decomp);
Err FeedDecompressor(Decompressor *decomp, void *data, uint32 numDataWords);
int32 GetDecompressorWorkBufferSize(const TagArg *tags);

/* varargs variants of some of the above */
Err CreateCompressorVA(Compressor **comp, CompFunc cf, uint32 tags, ...);
int32 GetCompressorWorkBufferSizeVA(uint32 tags, ...);
Err CreateDecompressorVA(Decompressor **decomp, CompFunc cf, uint32 tags, ...);
int32 GetDecompressorWorkBufferSizeVA(uint32 tags, ...);

/* convenience routines */
Err SimpleCompress(void *source, uint32 sourceWords, void *result, uint32 resultWords);
Err SimpleDecompress(void *source, uint32 sourceWords, void *result, uint32 resultWords);


#ifdef __cplusplus
}
#endif


/****************************************************************************/


/* user function offsets */
#define CREATECOMPRESSOR              -1
#define DELETECOMPRESSOR              -2
#define FEEDCOMPRESSOR                -3
#define GETCOMPRESSORWORKBUFFERSIZE   -4
#define CREATEDECOMPRESSOR            -5
#define DELETEDECOMPRESSOR            -6
#define FEEDDECOMPRESSOR              -7
#define GETDECOMPRESSORWORKBUFFERSIZE -8


/*****************************************************************************/


#endif /* __COMPRESSION_H */

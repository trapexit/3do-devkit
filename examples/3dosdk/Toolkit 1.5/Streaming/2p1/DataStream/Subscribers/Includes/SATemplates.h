/*********************************************************************************
 *	File:			SATemplates.h
 *
 *	Contains:		Definitions for instrument template tags used by
 *the subscriber to preload instrument templates before playback.
 *
 *	Written by:		Darren Gibbs, adapted from the prototype
 *subscriber by Joe Buczek.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/11/94		rdg		Add support for ADPCM and uncompressed
 *22K DSP instruments 1/20/94		rdg		make C++ compatible
 *	10/20/93	jb		Change conditionals to use __CC_NORCROFT
 *for 3DO builds 10/14/93	rdg		Version 2.8 -- New Today Split
 *source into separate files.
 */

#ifndef __SATEMPLATES_H__
#define __SATEMPLATES_H__

#include "types.h"

#ifdef __CC_NORCROFT
#include "SAStreamChunks.h"
#endif

/**********************************************/
/* Flags for pre-loading instrument templates */
/**********************************************/

#define SA_COMPRESSION_SHIFT 24
#define SA_CHANNELS_SHIFT 16
#define SA_RATE_SHIFT 8
#define SA_SIZE_SHIFT 0

/* Sample rate values */
#define SA_44KHz 1
#define SA_22KHz 2

/* Sample size values */
#define SA_16Bit 1
#define SA_8Bit 2

/* Sample channel values */
#define SA_STEREO 2
#define SA_MONO 1

/* Sample compression type values */
#define SA_COMP_NONE 1
#define SA_COMP_SDX2 2
#define SA_COMP_ADPCM4 4

/* Macro to create instrument tags from human usable parameters.
 * The tag values created by this macro are used by the
 * GetTemplateItem() routine.
 */
#define MAKE_SA_TAG(rate, bits, chans, compression)                           \
  ((long)((rate << SA_RATE_SHIFT) | (bits << SA_SIZE_SHIFT)                   \
          | (chans << SA_CHANNELS_SHIFT)                                      \
          | (compression << SA_COMPRESSION_SHIFT)))

/* Uncompressed instrument tags */
#define SA_44K_16B_S                                                          \
  (MAKE_SA_TAG (SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE))
#define SA_44K_16B_M (MAKE_SA_TAG (SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_NONE))
#define SA_44K_8B_S (MAKE_SA_TAG (SA_44KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE))
#define SA_44K_8B_M (MAKE_SA_TAG (SA_44KHz, SA_8Bit, SA_MONO, SA_COMP_NONE))
#define SA_22K_16B_S                                                          \
  (MAKE_SA_TAG (SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE))
#define SA_22K_16B_M (MAKE_SA_TAG (SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_NONE))
#define SA_22K_8B_S (MAKE_SA_TAG (SA_22KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE))
#define SA_22K_8B_M (MAKE_SA_TAG (SA_22KHz, SA_8Bit, SA_MONO, SA_COMP_NONE))

/* Compressed instrument tags */
#define SA_44K_16B_S_SDX2                                                     \
  (MAKE_SA_TAG (SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2))
#define SA_44K_16B_M_SDX2                                                     \
  (MAKE_SA_TAG (SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2))
#define SA_22K_16B_S_SDX2                                                     \
  (MAKE_SA_TAG (SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2))
#define SA_22K_16B_M_SDX2                                                     \
  (MAKE_SA_TAG (SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2))

#define SA_44K_16B_M_ADPCM4                                                   \
  (MAKE_SA_TAG (SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4))
#define SA_22K_16B_M_ADPCM4                                                   \
  (MAKE_SA_TAG (SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4))

#ifdef __CC_NORCROFT
/****************************************************************************
 * Structure to describe instrument template items that may be dynamically
 * loaded. An array of these describes all the possible templates we know
 * about. We actually load an instrument template when explicitly asked to
 * do so with a control message or when we encounter a header in a stream
 * that requires an instrument type for which we do not	have a loaded
 * template.
 * Each instantiation of an audio subscriber gets a clean copy of an
 * initial array of these structures which it uses to cache templates.
 ****************************************************************************/
typedef struct TemplateRec
{
  long templateTag;     /* used to match caller input value */
  Item templateItem;    /* item for the template or zero */
  char *instrumentName; /* ptr to string of filename */
} TemplateRec, *TemplateRecPtr;

/* Forward reference for context */
struct SAudioContext;

/* Routine prototypes */
#ifdef __cplusplus
extern "C"
{
#endif

  long GetTemplateTag (SAudioSampleDescriptorPtr descPtr);
  Item GetTemplateItem (struct SAudioContext *ctx, long templateTag,
                        long templateCount);
  long LoadTemplates (struct SAudioContext *ctx, long *tagPtr,
                      long templateCount);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SATEMPLATES_H__ */

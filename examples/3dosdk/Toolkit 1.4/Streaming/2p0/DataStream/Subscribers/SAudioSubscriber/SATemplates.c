/*******************************************************************************************
 *	File:			SAudioTemplates.c
 *
 *	Contains:		Implementation of routines to manage instrument
 *templates.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/8/94		fyp		Version 2.8
 *						Yanked trace compile switches to the make
 *file level. 10/14/93	rdg		Version 2.8 -- New Today Split source
 *into separate files.
 */

#include "SATemplates.h"
#include "SAErrors.h"
#include "SAStreamChunks.h"
#include "audio.h"
#include "types.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if TRACE_TEMPLATES

#include "SAudioTraceCodes.h"
#include "SubscriberTraceUtils.h"

/* Locate the buffer.  It's define in SAMain.c. */
extern TraceBufferPtr SATraceBufPtr;

#define ADD_TRACE_L1(bufPtr, event, chan, value, ptr)                         \
  AddTrace (bufPtr, event, chan, value, ptr)

#else

/* Trace is off */
#define ADD_TRACE_L1(bufPtr, event, chan, value, ptr)

#endif

/* The following table describes all the AudioFolio instrument templates that
 * we know about. These will be dynamically or pre-use loaded. The user is able
 * to cause them to be preloaded by sending us a control message that tells us
 * which of these is desired. Each is specified by its tag value. The Item
 * associated with the entity is cached so that if we are asked for the same
 * thing again later, we will already have loaded it from disk. A copy of this
 * table is made for each instantiation of an audio subscriber. This is done
 * because each subscriber thread must own its own items (which are cached in
 * the table). The following table is COPIED to allocated memory when a
 * subscriber is instantiated. The contents of the following should never be
 * modified.
 */
TemplateRec gInitialTemplates[] = {

  { SA_22K_8B_M, 0, "halfmono8.dsp" },
  { SA_22K_8B_S, 0, "halfstereo8.dsp" },
  { SA_44K_8B_S, 0, "fixedstereo8.dsp" },
  { SA_44K_8B_M, 0, "fixedmono8.dsp" },
  { SA_44K_16B_M, 0, "fixedmonosample.dsp" },
  { SA_44K_16B_S, 0, "fixedstereosample.dsp" },
  { SA_44K_16B_M_SDX2, 0, "dcsqxdmono.dsp" },
  { SA_44K_16B_S_SDX2, 0, "dcsqxdstereo.dsp" },
  { SA_22K_16B_M_SDX2, 0, "dcsqxdhalfmono.dsp" },
  { SA_22K_16B_S_SDX2, 0, "dcsqxdhalfstereo.dsp" }

};

long kMaxTemplateCount = (sizeof (gInitialTemplates) / sizeof (TemplateRec));

/*******************************************************************************************
 * Routine to determine template cache tag based on the sample descriptor
 *structure. If we don't have a template for the data type return an error.
 *******************************************************************************************/
long
GetTemplateTag (SAudioSampleDescriptorPtr descPtr)
{
  if (descPtr->numChannels == 1)
    {
      if (descPtr->compressionType == ID_SDX2)
        {
          if (descPtr->sampleRate == 22050)
            return SA_22K_16B_M_SDX2;
          else
            return SA_44K_16B_M_SDX2;
        }

      if (descPtr->sampleSize == 8)
        {
          if (descPtr->sampleRate == 22050)
            return SA_22K_8B_M;
          else
            return SA_44K_8B_M;
        }

      if (descPtr->sampleRate == 44100)
        return SA_44K_16B_M;
    }

  if (descPtr->numChannels == 2)
    {
      if (descPtr->compressionType == ID_SDX2)
        {
          if (descPtr->sampleRate == 22050)
            return SA_22K_16B_S_SDX2;
          else
            return SA_44K_16B_S_SDX2;
        }

      if (descPtr->sampleSize == 8)
        {
          if (descPtr->sampleRate == 22050)
            return SA_22K_8B_S;
          else
            return SA_44K_8B_S;
        }

      if (descPtr->sampleRate == 44100)
        return SA_44K_16B_S;
    }

  /* Couldn't find a template match audio data type */
  return -1;
}

/*******************************************************************************************
 * Routine to walk through the array of template records looking for the
 *template specified by 'templateTag'. If the template for the matching record
 *has not been loaded, then load it immediately and cache the template item for
 *later use.
 *******************************************************************************************/
Item
GetTemplateItem (TemplateRecPtr tp, long templateTag, long templateCount)
{
  long k;

  /* Walk through the array of template records (cache) looking for the
   * template specified by 'templateTag'. If the template for the matching
   * record has not been loaded, then load it immediately and cache the
   * template item for later use.
   */
  for (k = 0; k < templateCount; k++, tp++)
    {
      /* Check for an exact match of bits and that the template
       * has not already been loaded.
       */
      if (tp->templateTag == templateTag)
        {
          /* Check to see if the entity needs to be loaded and
           * load it if so.
           */
          if (tp->templateItem == 0)
            tp->templateItem = LoadInsTemplate (tp->instrumentName, 0);

          return tp->templateItem;
        }
    }

  return SAudioErrTemplateNotFound;
}

/*******************************************************************************************
 * Routine search for and load a number of instrument templates given a pointer
 *to a NULL terminated list of tags that describe the instruments. The
 *templates are cached (if one searched for is already loaded, it is ignored).
 *******************************************************************************************/
long
LoadTemplates (TemplateRecPtr tp, long *tagPtr, long templateCount)
{
  long status;

  /* Walk through the array of template tags and load any
   * that are requested that have not already been loaded.
   */
  while (*tagPtr != 0)
    {
      status = GetTemplateItem (tp, *tagPtr, templateCount);
      if (status < 0)
        return status;

      /* Advance to the next tag specified */
      tagPtr++;
    }

  ADD_TRACE_L1 (SATraceBufPtr, kLoadedTemplates, -1, 0, 0);

  return kDSNoErr;
}

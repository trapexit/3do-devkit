#pragma include_only_once

#include "types.h"

#include "sastreamchunks.h"

#define	SA_COMPRESSION_SHIFT 24
#define	SA_CHANNELS_SHIFT    16
#define	SA_RATE_SHIFT	     8
#define	SA_SIZE_SHIFT	     0

#define	SA_44KHz 1
#define	SA_22KHz 2

#define	SA_16Bit 1
#define	SA_8Bit	 2

#define	SA_STEREO 2
#define	SA_MONO	  1

#define	SA_COMP_NONE   1
#define	SA_COMP_SDX2   2
#define	SA_COMP_ADPCM4 4

#define	MAKE_SA_TAG( rate, bits, chans, compression )	\
  ((long) ( (rate << SA_RATE_SHIFT )                    \
            | (bits << SA_SIZE_SHIFT)                   \
            | (chans << SA_CHANNELS_SHIFT)              \
            | (compression << SA_COMPRESSION_SHIFT) ) )

#define	SA_44K_16B_S (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_44K_16B_M (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_NONE ))
#define	SA_44K_8B_S  (MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_44K_8B_M  (MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_MONO, SA_COMP_NONE ))
#define	SA_22K_16B_S (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_22K_16B_M (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_NONE ))
#define	SA_22K_8B_S  (MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_22K_8B_M  (MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_MONO, SA_COMP_NONE ))

#define	SA_44K_16B_S_SDX2 (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 ))
#define	SA_44K_16B_M_SDX2 (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 ))
#define	SA_22K_16B_S_SDX2 (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 ))
#define	SA_22K_16B_M_SDX2 (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 ))

#define	SA_44K_16B_M_ADPCM4 (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4 ))
#define	SA_22K_16B_M_ADPCM4 (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4 ))

typedef struct TemplateRec {
  long		templateTag;		/* used to match caller input value */
  Item		templateItem;		/* item for the template or zero */
  char*		instrumentName;		/* ptr to string of filename */
} TemplateRec, *TemplateRecPtr;

struct SAudioContext;

EXTERN_C_BEGIN

long GetTemplateTag(SAudioSampleDescriptorPtr descPtr);
Item GetTemplateItem(struct SAudioContext* ctx, long templateTag, long templateCount);
long LoadTemplates(struct SAudioContext* ctx, long* tagPtr, long templateCount);

EXTERN_C_END

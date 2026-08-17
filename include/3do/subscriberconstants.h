#pragma once
#include "subscriberutils.h"

/* Shared constants for the Cinepak (CPak) and EZSqueeze (EZQ) movie
 * subscribers.  Both cpaksubscriber.h and ezqsubscriber.h include this
 * file.  FILM_CHUNK_TYPE differs per codec ('FILM' vs 'EZQF') and is
 * defined in each subscriber's own header.
 */

#define	FHDR_CHUNK_TYPE	CHAR4LITERAL('F','H','D','R') /* chunk data type shared by both movie formats */
#define	FRME_CHUNK_TYPE	CHAR4LITERAL('F','R','M','E') /* chunk data type shared by both movie formats */

#define	CPAK_MAX_SUBSCRIPTIONS 2 /* max # of streams that can use the subscriber */
#define	CPAK_MAX_CHANNELS      8 /* max # of logical channels per subscription */
#define	CPAK_MAX_CHUNKS	       128 /* max # of chunks pending per subscriber */

#define	BYTES_PER_PIXEL	  2	// this is all 16 bit uncoded data
#define	SCANLINES_PER_ROW 2	// 2 interlaced scanlines per frame buffer line

#define	NO_FRAME_ITEM -1	/* used to indicate no current frame msg item */

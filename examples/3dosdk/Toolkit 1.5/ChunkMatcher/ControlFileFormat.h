/*******************************************************************************************
 *	File:			ControlFileFormat.h
 *
 *	Contains:		
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __CONTROLFILEFORMAT__
#define __CONTROLFILEFORMAT__



/***********/
/*  TYPES  */
/***********/

typedef struct {
	SUBS_CHUNK_COMMON;
	union {
		struct {				// sub-type 'GOTO'
			uint32		value;
			} marker;

		struct {				//sub-type 'SYNC'
			uint32		value;
			} sync;

		struct {				// sub-type 'ALRM'
			uint32		options;
			uint32		newTime;
			} alarm;

		struct {				// sub-type 'PAUS'
			uint32		options;
			uint32		amount;
			} pause;

		struct {				// sub-type 'STOP'
			uint32		options;
			} stop;
		} u;

} ControlChunk;


#endif

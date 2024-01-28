/*
	File:		codec.h

	Contains:	xxx put contents here xxx

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Portions Copyright © 1993 by SuperMac Technology, Inc.

	Change History (most recent first):

		 <2>	 4/17/93	dsm		Update for Initial Developer Release
		 <1>	 3/31/93	dsm		first checked in

	To Do:
*/


#ifndef __codec_h
#define __codec_h

/*
	codec.h
	Codec Includes
	Peter Barrett
	Copyright (C) 1992, SuperMac Technology.
*/

typedef char* codec;
typedef char* codecHandler;


#ifdef __cplusplus 
extern "C" {
#endif

codecHandler CreateCodecHandler(void);
void	DisposeCodecHandler(codecHandler ch);
		
codec	CreateCodec(codecHandler ch, long codecType);
void	DisposeCodec(codec c);
int		PreDecompress(codec c, void *compressedFrame);
int		Decompress(codec c, void *compressedFrame, char *baseAddr, long rowBytes);

#ifdef __cplusplus
}
#endif



#endif

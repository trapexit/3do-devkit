
#ifndef __codec_h
#define __codec_h

/*
	codec.h
	Codec Includes
	Peter Barrett
	Copyright (C) 1992, SuperMac Technology.

 *	1/20/94		rdg		make C++ compatible

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

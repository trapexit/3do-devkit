/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1 with 
 **							squareroot-delta-exactencoding and write an AIFC file.  
 **							Throw out all chunks but SSND FORM COMM MARK INST FVER
 **
 **	This File:		MacFileIO.h
 **
 ** Contains:		header file to define public routines for MacFileIO.c
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and proprietary 
 **	information of the 3DO Company and shall not be used by any Person or for any 
 **	purpose except as expressly authorized in writing by the 3DO Company.
 **	                        
 **                                                                  
 **  History:                                                        
 **  -------                                                         
 **	 6/17/93		RDG		version 1.0
 **	 6/15/93		RDG		version .12
 **	 6/15/93		RDG		New Today                                  
 **                                                                  
 **************************************************************************************/

#ifndef __MACFILEIO_H__
#define __MACFILEIO_H__


/* File Position Modes for mfseek() */
#define MSEEK_START		1
#define MSEEK_END		2
#define MSEEK_CUR		3

/* Public Function Prototypes */
int 	mfcreate( char *filename, long creator, long fileType, long *fileID );
int		mfopen( char *filename, long *fileID );
int		mfclose( long fileID );
int		mfgetc( long fileID );
int		mfputc( int c, long fileID );
size_t	mfread( void *ptr, size_t size, size_t nmemb, long fileID);
size_t	mfwrite( const void *ptr, size_t size, size_t nmemb, long fileID );
int		mfgetw( short inRefNum );
int		mfputw( int w, long fileID );
int 	mfseek( long fileID, long offset, short posMode );
int 	mfgetpos( long fileID, long *offset );

#endif	/* __MACFILEIO_H__ */

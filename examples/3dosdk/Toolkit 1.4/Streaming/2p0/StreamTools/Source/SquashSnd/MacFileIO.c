/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1 with 
 **							squareroot-delta-exactencoding and write an AIFC file.  
 **							Throw out all chunks but SSND FORM COMM MARK INST FVER
 **
 **	This File:		MacFileIO.c
 **
 ** Contains:		Routines to do fileio with native mac calls
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
 **	 6/17/93	RDG		version 1.0
 **	 6/15/93	RDG		version .12
 **	 6/15/93	RDG		New Today (based on code from Jay Moreland)                                  
 **                                                                  
 **************************************************************************************/

#include <StdLib.h>
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <Strings.h>
#include <Values.h>
#include <Types.h>
#include <Resources.h>
#include <Files.h>
#include <OSUtils.h>
#include <OSEvents.h>
#include <DiskInit.h>
#include <Packages.h>
#include <Traps.h>
#include <Serial.h>
#include <Devices.h>
#include <Errors.h>
#include <StandardFile.h>
#include <PLStringFuncs.h>

#define myEOF -1
#define stderr		(&_iob[2])

extern FILE _iob[];

int mfcreate( char *filename, long creator, long fileType, long *fileID )
	{
	OSErr 	err;
	Str255	pFileName;
	short	fRefNum;
	short	vRefNum = 0;
	
	strcpy((char *) &pFileName, filename);
	c2pstr((char *) &pFileName);

	/* remove any old files with this name */
	err = FSDelete(pFileName, vRefNum);
	
	if ( ( err = Create(pFileName, vRefNum, (OSType) creator, (OSType) fileType) ) != noErr )
		return err;

	if ( ( err = FSOpen(pFileName, vRefNum, &fRefNum) ) != noErr )
		return err;

	*fileID = (long) fRefNum;
	return 0;
	}

int mfopen( char *filename, long *fileID )
	{
	OSErr 	err;
	Str255	pFileName;
	short	fRefNum;
	short	vRefNum = 0;
	
	strcpy((char *) &pFileName, filename);
	c2pstr((char *) &pFileName);

	if ( ( err = FSOpen(pFileName, vRefNum, &fRefNum) ) != noErr )
		return err;
		
	*fileID = (long) fRefNum;
	return 0;
	}

int mfclose( long fileID )
	{
	OSErr 	err;
	
	err = FSClose((short) fileID);
	return err;
	}

int mfgetc( long fileID )
	{
	char c;
	OSErr err;
	long numberOfBytes = 1;
	
	if ( ( err = FSRead( (short) fileID, &numberOfBytes, &c ) == noErr ) )
		return (unsigned int)c;
	return myEOF;
	}
	
int mfputc( int c, long fileID )
	{
	OSErr err;
	long numberOfBytes = 1;
	
	c = c<<24;
	if ( ( err = FSWrite( (short) fileID, &numberOfBytes, &c ) ) == noErr )
		return 0;
	return myEOF;
	}
	
size_t mfread( void *ptr, size_t size, size_t nmemb, long fileID )
	{
	long numberOfBytes = size*nmemb;
	OSErr err;
	
	if ( ( err = FSRead( (short) fileID, &numberOfBytes, ptr ) ) == noErr ) 
		return numberOfBytes/size;
	return err;
	}
	
size_t mfwrite( const void *ptr, size_t size, size_t nmemb, long fileID )
	{
	long numberOfBytes = size*nmemb;
	OSErr err;
	
	if ( ( err = FSWrite( (short) fileID, &numberOfBytes, ptr ) ) == noErr )
		return numberOfBytes/size;
	return  err ;
	}
	
int mfgetw( long fileID )
	{
	long numberOfBytes = 4;
	int x;
	OSErr err;
	
	if ( (err = FSRead( (short) fileID, &numberOfBytes, &x ) ) == noErr )
		return x;
	return myEOF;
	}
	
int mfputw( int w, long fileID )
	{
	long numberOfBytes = 4;
	OSErr err;
	
	if ( ( err = FSWrite( (short) fileID, &numberOfBytes, &w ) ) == noErr )
		return 0;
	return err;
	}

int	mfseek( long fileID, long offset, short posMode )
	{
	OSErr err;
	
	if ( ( err = SetFPos((short) fileID, posMode, offset ) ) == noErr )
		return 0;
	return err;
	}

int mfgetpos( long fileID, long *offset )
	{
	OSErr err;
	
	if ( ( err = GetFPos((short) fileID, offset ) ) == noErr )
		return 0;
	return err;
	}

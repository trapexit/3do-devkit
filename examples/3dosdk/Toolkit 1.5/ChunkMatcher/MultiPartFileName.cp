/*******************************************************************************************
 *	File:			MultiPartFileName.cp
 *
 *	Contains:		Represents multi-part name for output files.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/3/94		crm		Removed conditionally-compiled use of
 *stream manipulators. 8/27/94		crm		New today
 *
 *******************************************************************************************/

#include "MultiPartFileName.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strstream.h>
#include <types.h>

static const int MAX_NAME = 256;
static const char *DEFAULT_DELIMITER = ".";
static const int DEFAULT_PART_WIDTH = 4;

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/**********************
 * TMultiPartFileName
 **********************
 * Null constructor
 */

TMultiPartFileName::TMultiPartFileName ()

{
  this->name = new char[MAX_NAME];
  this->directory = NULL;
  this->partName = NULL;
  this->subPartName = NULL;
  this->suffix = NULL;
  this->partNumber = 0;
  this->subPartNumber = 0;
  this->delimiter = DEFAULT_DELIMITER;
  this->partWidth = DEFAULT_PART_WIDTH;
}

/**********************
 * TMultiPartFileName
 **********************
 * Constructor
 */

TMultiPartFileName::TMultiPartFileName (const char *directoryName)

{
  this->name = new char[MAX_NAME];
  this->directory = NULL;
  this->partName = NULL;
  this->subPartName = NULL;
  this->suffix = NULL;
  this->partNumber = 0;
  this->subPartNumber = 0;
  this->delimiter = DEFAULT_DELIMITER;
  this->partWidth = DEFAULT_PART_WIDTH;
  ReplaceString (this->directory, directoryName);
}

/***********************
 * ~TMultiPartFileName
 ***********************
 * Destructor
 */

TMultiPartFileName::~TMultiPartFileName ()

{
  delete[] this->name;
  delete[] this->directory;
  delete[] this->partName;
  delete[] this->subPartName;
  delete[] this->suffix;
}

/**********************
 * TMultiPartFileName
 **********************
 * SetDirectory
 **********************
 * Set the directory of this file name.
 */

void
TMultiPartFileName::SetDirectory (const char *newDirectory)

{
  this->ReplaceString (this->directory, newDirectory);
}

/**********************
 * TMultiPartFileName
 **********************
 * SetSuffix
 **********************
 * Set the suffix of this file name.
 */

void
TMultiPartFileName::SetSuffix (const char *newSuffix)

{
  this->ReplaceString (this->suffix, newSuffix);
}

/**********************
 * TMultiPartFileName
 **********************
 * SetPartName
 **********************
 * Set the part name of this file name.
 */

void
TMultiPartFileName::SetPartName (const char *newPartName)

{
  this->ReplaceString (this->partName, newPartName);
}

/**********************
 * TMultiPartFileName
 **********************
 * SetSubPartName
 **********************
 * Set the sub-part name of this file name.
 */

void
TMultiPartFileName::SetSubPartName (const char *newSubPartName)

{
  this->ReplaceString (this->subPartName, newSubPartName);
}

/**********************
 * TMultiPartFileName
 **********************
 * GetPartNumber
 **********************
 * Result is the current part number in
 * this file name.
 */

unsigned long
TMultiPartFileName::GetPartNumber () const

{
  return this->partNumber;
}

/**********************
 * TMultiPartFileName
 **********************
 * GetSubPartNumber
 **********************
 * Result is the current sub-part number in
 * this file name.
 */

unsigned long
TMultiPartFileName::GetSubPartNumber () const

{
  return this->subPartNumber;
}

/**********************
 * TMultiPartFileName
 **********************
 * SetPartNumber
 **********************
 * Set the part number in this file name.
 */

void
TMultiPartFileName::SetPartNumber (unsigned long newPartNumber)

{
  this->partNumber = newPartNumber;
}

/**********************
 * TMultiPartFileName
 **********************
 * SetSubPartNumber
 **********************
 * Set the sub-part number in this file name.
 */

void
TMultiPartFileName::SetSubPartNumber (unsigned long newSubPartNumber)

{
  this->subPartNumber = newSubPartNumber;
}

/************************
 * TMultiPartFileName
 ************************
 * operator const char*
 ************************
 * Cast this file name object to an immutable
 * pointer to the file name string.
 */

TMultiPartFileName::operator const char * ()

{
  this->AssembleFileName ();

  // Return a constant pointer to the name buffer

  return (const char *)this->name;
}

/***************
 * operator <<
 ***************
 * Print the specified file name to the
 * specified character stream.
 */

ostream &
operator<< (ostream &stream, TMultiPartFileName &fileName)

{
  fileName.AssembleFileName ();

  stream << fileName.name;

  return stream;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/**********************
 * TMultiPartFileName
 **********************
 * ReplaceString
 **********************
 * Replace the storage of the specified string.
 */

void
TMultiPartFileName::ReplaceString (char *&string, const char *newString)

{
  delete[] string;
  if (newString == NULL || *newString == '\0')
    string = NULL;
  else
    {
      string = new char[strlen (newString) + 1];
      if (string != NULL)
        strcpy (string, newString);
    }
}

/**********************
 * TMultiPartFileName
 **********************
 * AssembleFileName
 **********************
 * Assemble the current file name in the internal
 * buffer.
 */

void
TMultiPartFileName::AssembleFileName ()

{
  const char *DELIMITER = "."; // delimiter used in file name
  ostrstream nameStream (this->name,
                         MAX_NAME); // write to name buffer as a stream

  *this->name = '\0';

  if (this->directory != NULL)
    nameStream << this->directory;

  nameStream.fill ('0');

  // First part of file name is "partNNNN"

  if (this->partName != NULL)
    {
      nameStream << this->partName;
      nameStream.width (this->partWidth);
      nameStream << this->partNumber;
    }

  // Second part of file name is ".subpartNNNN"

  if (this->subPartName != NULL)
    {
      nameStream << this->delimiter << this->subPartName;
      nameStream.width (this->partWidth);
      nameStream << this->subPartNumber;
    }

  // Tack on suffix ".suffix"

  if (this->suffix != NULL)
    {
      nameStream << this->delimiter << this->suffix;
    }

  // Remember to terminate the string in the buffer

  nameStream << '\0';
}

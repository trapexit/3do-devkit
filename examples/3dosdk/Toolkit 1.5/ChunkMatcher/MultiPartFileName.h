/*******************************************************************************************
 *	File:			MultiPartFileName.h
 *
 *	Contains:		Represents multi-part name for output files.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	8/27/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __MULTIPARTFILENAME__
#define __MULTIPARTFILENAME__

/*************/
/*  CLASSES  */
/*************/

class TMultiPartFileName
{
public:
  TMultiPartFileName ();
  TMultiPartFileName (const char *directory);

  ~TMultiPartFileName ();

  void SetDirectory (const char *newDirectory);
  // Set the directory in the path name

  void SetSuffix (const char *newSuffix);
  // Set the suffix in the file name

  void SetPartName (const char *newPartName);
  // Set the part name in the file name

  void SetSubPartName (const char *newSubPartName);
  // Set the sub-part name in the file name

  unsigned long GetPartNumber () const;
  // Get the part number in the file name

  unsigned long GetSubPartNumber () const;
  // Get the sub-part number in the file name

  void SetPartNumber (unsigned long newPartNumber);
  // Set the part number in the file name

  void SetSubPartNumber (unsigned long newSubPartNumber);
  // Set the sub-part number in the file name

  operator const char * ();
  // Return an immutable pointer to this file name

  friend ostream &operator<< (ostream &, TMultiPartFileName &);
  // Print this file name to the specified character stream

private:
  TMultiPartFileName (const TMultiPartFileName &) {}
  void
  operator= (const TMultiPartFileName &)
  {
  }
  // Copying and assignment are disallowed

  void ReplaceString (char *&string, const char *newString);
  // Replace the storage of the specified string

  void AssembleFileName ();
  // Assemble the current file name in the internal buffer

private:
  char *name;                  // name of file
  char *directory;             // file directory name string
  char *partName;              // file part name string
  char *subPartName;           // file sub-part name string
  char *suffix;                // file suffix name string
  unsigned long partNumber;    // part number used in output file name
  unsigned long subPartNumber; // sub-part number used in output file name
  const char *delimiter;       // delimiter used in file name
  int partWidth;               // width of part number field in file name
};

#endif

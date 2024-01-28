/*******************************************************************************************
 *	File:			Options.h
 *
 *	Contains:		Options object represents program options
 *specified on the shell command line.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	9/26/94		crm		Reduced stream I/O expression causing
 *Apple C++ compiler to fail 7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __OPTIONS__
#define __OPTIONS__

/*************/
/*  CLASSES  */
/*************/

class TOptions
{
public:
  enum Operation
  {
    NO_OPERATION,
    EXTRACT,
    EXTRACT_GROUPS,
    STREAM_INFO
  };
  // enumerated type for operations this tool can perform

  TOptions ();
  virtual ~TOptions ();

  Boolean Parse (int argc, char **argv);
  // Parse the specified command line

  const char *GetProgramName () const;
  // Return the name of this program

  const char *GetProgramVersion () const;
  // Return the version string for this program

  Operation GetOperation () const;
  // Return the operation being performed

  Boolean GetVerbose () const;
  // Return the state of the verbose flag

  const char *GetInputFileName () const;
  // Return the input file name string

  const char *GetOutputDirectoryName () const;
  // Return the output directory name string

  void PrintUsage (ostream &stream) const;
  // Print usage information to the specified stream

private:
  static const char *PROGRAM_VERSION; // release of this program

  static const char *VERBOSE_FLAG;   // print diagnostic output
  static const char *INPUT_FLAG;     // define input file
  static const char *OUTPUT_FLAG;    // define output file
  static const char *OUTPUTDIR_FLAG; // define output directory
  static const char *OPERATION_FLAG; // operation to perform
  static const char
      *SEPARATE_FLAG; // separate cels that normally would be written together
  static const char *EXTRACT_COMMAND; // extraction operation
  static const char
      *EXTRACT_GROUPS_COMMAND; // extraction operation (group items together)
  static const char *STREAM_INFO_COMMAND; // stream analysis operation

  TOptions (const TOptions &) {}
  void
  operator= (const TOptions &)
  {
  }
  // Copying and assignment are disallowed

  Operation MapOperationName (const char *opName) const;
  // Map operation name string to enumerated type

  Boolean OutputDirectoryRequired () const;
  // Result indicates whether output directory argument
  // is required for the current operation

  char *programName;    // name of this program
  char *inputFileName;  // path name of input file
  char *outputFileName; // path name of output file
  char *outputDirName;  // path name of output file
  Boolean verbose;      // print progress information to standard output
  Operation operation;  // operation to perform
};

#endif

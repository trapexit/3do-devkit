/*******************************************************************************************
 *	File:			Options.cp
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
 *	6/16/94		crm		New today
 *
 *******************************************************************************************/

#include "Options.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*************************/
/*  STATIC DATA MEMBERS  */
/*************************/

const char *TOptions::PROGRAM_VERSION = "1.0d0"; // release of this program

const char *TOptions::VERBOSE_FLAG = "-v";    // print diagnostic output
const char *TOptions::INPUT_FLAG = "-if";     // define input file
const char *TOptions::OUTPUT_FLAG = "-of";    // define output file
const char *TOptions::OUTPUTDIR_FLAG = "-od"; // define output directory
const char *TOptions::OPERATION_FLAG = "-op"; // define operation to perform

const char *TOptions::EXTRACT_COMMAND
    = "extract"; // extract items from the input chunk file
const char *TOptions::EXTRACT_GROUPS_COMMAND
    = "extract_groups"; // extract items from the input chunk file
const char *TOptions::STREAM_INFO_COMMAND
    = "stream_info"; // analyze contents of the input stream chunk file

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/************
 * TOptions
 ************
 * Null constructor
 */

TOptions::TOptions ()

{
  this->programName = NULL;
  this->inputFileName = NULL;
  this->outputFileName = NULL;
  this->outputDirName = NULL;
  this->verbose = false;
  this->operation = NO_OPERATION;
}

/*************
 * ~TOptions
 *************
 * Destructor
 */

TOptions::~TOptions () {}

/************
 * TOptions
 ************
 * Parse
 ************
 * Parse the specified command line and
 * save the result of the parse as the
 * state of this object.
 * Result is true if the command line
 * contained all required information, and
 * false if some required information was
 * missing.
 */

Boolean
TOptions::Parse (int argc, char **argv)

{
  char *flag;     // next flag string
  int count;      // count of command-line arguments processed so far
  Boolean result; // false if command line could not be parsed completely

  // Set the program name from the command-line string

  this->programName = argv[0];

  result = true;

  // Loop over the command-line arguments

  for (count = 1; count < argc; ++count)
    {
      flag = argv[count];

      /***************************************************/
      /*  Verbose:
       */
      /*  Enable progress messages					   */
      /***************************************************/
      if (strcmp (flag, VERBOSE_FLAG) == 0)
        this->verbose = true;

      /***************************************************/
      /*  Operation: */
      /*  Declare the operation to be performed		   */
      /***************************************************/
      else if (strcmp (flag, OPERATION_FLAG) == 0)
        {
          // Expecting operation name string as next argument

          if (++count >= argc)
            // Ran out of arguments
            break;

          // Convert the operation name string to an enumerated type

          this->operation = this->MapOperationName (argv[count]);

          // Validate the operation name string

          if (this->operation == NO_OPERATION)
            cerr << "*** Unknown operation specified '" << argv[count]
                 << "'\n\n";
        }

      /***************************************************/
      /*  Input File Name:	    					   */
      /*  Declare the path name of the input chunk file  */
      /***************************************************/
      else if (strcmp (flag, INPUT_FLAG) == 0)
        {
          // Expecting file name string as next argument

          if (++count < argc)
            this->inputFileName = argv[count];
          else
            // Ran out of arguments
            break;
        }

      /***************************************************/
      /*  Output File Name:	    				       */
      /*  Declare the path name of the output file       */
      /***************************************************/
      else if (strcmp (flag, OUTPUT_FLAG) == 0)
        {
          // Expecting file name string as next argument

          if (++count < argc)
            this->outputFileName = argv[count];
          else
            // Ran out of arguments
            break;
        }

      /***************************************************/
      /*  Output Directory Name:		    		       */
      /*  Declare the name of the output directory       */
      /***************************************************/
      else if (strcmp (flag, OUTPUTDIR_FLAG) == 0)
        {
          // Expecting directory name string as next argument

          if (++count < argc)
            this->outputDirName = argv[count];
          else
            // Ran out of arguments
            break;
        }
    }

  // Check that an operation was specified

  if (this->operation == NO_OPERATION)
    {
      cerr << "*** Please specify an operation to perform\n\n";
      result = false;
    }

  // Check that an input file was specified

  if (this->inputFileName == NULL)
    {
      cerr << "*** Please specify an input file name\n\n";
      result = false;
    }

  // Check that an output directory was specified
  // An output directory is only required when extraction
  // is being performed.

  if (this->outputDirName == NULL && this->OutputDirectoryRequired ())
    {
      cerr << "*** Please specify an output directory name\n\n";
      result = false;
    }

  return result;
}

/****************
 * TOptions
 ****************
 * GetOperation
 ****************
 * Result is an enumerated type for the
 * operation specified on the command
 * line.
 */

TOptions::Operation
TOptions::GetOperation () const

{
  return this->operation;
}

/**************
 * TOptions
 **************
 * GetVerbose
 **************
 * Result is the setting of the verbose option.
 */

Boolean
TOptions::GetVerbose () const

{
  return this->verbose;
}

/******************
 * TOptions
 ******************
 * GetProgramName
 ******************
 * Result is the name of the program, as
 * specified on the command line.
 */

const char *
TOptions::GetProgramName () const

{
  return this->programName;
}

/*********************
 * TOptions
 *********************
 * GetProgramVersion
 *********************
 * Result is the version of the program.
 */

const char *
TOptions::GetProgramVersion () const

{
  return PROGRAM_VERSION;
}

/**************************
 * TOptions
 **************************
 * GetOutputDirectoryName
 **************************
 * Result is the output directory name string.
 */

const char *
TOptions::GetOutputDirectoryName () const

{
  return this->outputDirName;
}

/********************
 * TOptions
 ********************
 * GetInputFileName
 ********************
 * Result is the input file name string.
 */

const char *
TOptions::GetInputFileName () const

{
  return this->inputFileName;
}

/**************
 * TOptions
 **************
 * PrintUsage
 **************
 * Write program usage information to the
 * specified stream.
 */

void
TOptions::PrintUsage (ostream &stream) const

{
  stream << this->programName << " version " << PROGRAM_VERSION << "\n";
  stream << "Usage: " << this->programName << "\n";
  stream << "\t\t" << OPERATION_FLAG << " 'operation'\toperation to perform\n";
  stream << "\t\t" << INPUT_FLAG << " 'name'\tname of input chunk file\n";
  stream << "\t\t" << OUTPUT_FLAG << " 'name'\tname of output file\n";
  stream << "\t\t" << OUTPUTDIR_FLAG << " 'name'\tname of output directory\n";
  stream << "\t\t" << VERBOSE_FLAG << "\t\tprint progress information\n\n";
  stream << "Operations:\n";
  stream << "\t\tstream_info\tanalyze stream chunk file and print summary\n";
  stream << "\t\textract\t\textract items\n";
  stream
      << "\t\textract_groups\textract items, keeping grouped items together\n";
  stream << "\t\t\t\t(e.g., keep foreground cel and anti-alias edge\n";
  stream << "\t\t\t\tcel together in same output file)\n";
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

/********************
 * TOptions
 ********************
 * MapOperationName
 ********************
 * Map the specified operation name string
 * to the enumerated type for valid operations.
 */

TOptions::Operation
TOptions::MapOperationName (const char *opName) const

{
  if (strcmp (opName, EXTRACT_COMMAND) == 0)
    return EXTRACT;
  if (strcmp (opName, EXTRACT_GROUPS_COMMAND) == 0)
    return EXTRACT_GROUPS;
  else if (strcmp (opName, STREAM_INFO_COMMAND) == 0)
    return STREAM_INFO;
  else
    return NO_OPERATION;
}

/***************************
 * TOptions
 ***************************
 * OutputDirectoryRequired
 ***************************
 * Result is true if the current operation
 * requires the output directory argument.
 */

Boolean
TOptions::OutputDirectoryRequired () const

{
  // The extraction operation requires an output directory

  return (this->operation == EXTRACT || this->operation == EXTRACT_GROUPS)
             ? true
             : false;
}

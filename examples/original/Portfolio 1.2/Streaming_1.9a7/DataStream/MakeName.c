/*******************************************************************************************
 *	File:			MakeName.c
 *
 *	Contains:		routine to generate a unique name for use with
 *kernel services
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/

#include "types.h"

#include "MakeName.h"

#define MAX_NAME_LENGTH                                                       \
  32                     /* max length of name we'll create from user prefix */
#define POSTFIX_LENGTH 8 /* 8 hex-ascii characters form the name postfix */

/************************************************************/
/* Array used to convert a supplied unique 32-bit value to 	*/
/* a unique name postfix.
 */
/************************************************************/
static char _hexChar[] = "0123456789ABCDEF";

/*******************************************************************************************
 * Routine to create a unique name given a name preamble string, and some
 * unique 32-bit value. Typically, the uniqueID value is an address of
 *something that is guaranteed to not be ureused until the object associated
 *with the name is deleted, for example, a pointer that stores an Item named
 *with such a name. Returns a pointer to the output buffer, name, or NULL if it
 *can't fit the postfix into the buffer because it it too small.
 *******************************************************************************************/
char *
MakeName (char *name, int32 maxNameLen, char *baseNameString, int32 uniqueID)
{
  int32 index;
  char *destPtr;

  /* Initialize output to empty */
  name[0] = '\0';

  /* Make sure at least the postfix can fit into the
   * output buffer. If not, return NULL as an error.
   */
  if (maxNameLen < (POSTFIX_LENGTH + 1))
    return NULL;

  /* Copy as much of the semaphore name prefix and space permits,
   * leaving enough space to make a hex ASCII postfix name out of
   * the supplied uniqeID value.
   */
  destPtr = name;
  for (index = 0; index < maxNameLen - POSTFIX_LENGTH; index++)
    if ((*destPtr++ = baseNameString[index]) == '\0')
      break;

  /* Back up to write over the NULL byte */
  destPtr--;

  /* Terminate the hex ASCII postfix string with a NULL */
  destPtr[POSTFIX_LENGTH] = '\0';

  /* Convert the uniqueID value to hexadecimal ASCII */
  for (index = POSTFIX_LENGTH - 1; index >= 0; index--)
    {
      /* Get least significant 4 bits and convert them to an ASCII char */
      destPtr[index] = _hexChar[uniqueID & 0x0000000F];

      /* Shift the uniqueID value down by 4 bits */
      uniqueID = uniqueID >> 4;
    }

  /* Return a pointer to the output buffer as our result */
  return name;
}

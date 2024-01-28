/*
        File:		CString.cp

        Contains:	Simple CString class.

        Written by:	Paul A. Bauersfeld
                                        Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <2>	  1/26/94	CDE		Cleanup for 3DO.
        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CString.h"
#include "ctype.h"
#include "stdio.h"
#include "string.h"

#include "CPlusSwiHack.h"

CString::CString (void)
{
  fLength = 0;
  fText = (char *)NULL;
}

CString::CString (CString &str)
{
  fLength = str.fLength;
  fText = (char *)NULL;

  if (str.fText != NULL)
    {
      fText = this->CloneText (str.fText, fLength);
      if (fText == NULL)
        fLength = 0;
    }
}

CString::CString (char *pCStr)
{
  fLength = 0;
  fText = (char *)NULL;

  if ((pCStr != NULL) && (pCStr[0] != '\0'))
    {
      fLength = strlen (pCStr);
      fText = this->CloneText ((char *)pCStr, fLength);
      if (fText == NULL)
        fLength = 0;
    }
}

CString::~CString (void)
{
  if (fText != NULL)
    delete fText;
}

CString
CString::operator= (CString &str)
{
  fLength = str.fLength;

  if (fText != NULL)
    {
      delete fText;
      fText = (char *)NULL;
    }

  if (fLength != 0)
    {
      fText = this->CloneText (str.fText, fLength);
      if (fText == NULL)
        fLength = 0;
    }

  return *this;
}

CString
operator+ (CString &str1, CString &str2)
{
  CString tmpStr;
  long totalLen = str1.fLength + str2.fLength;

  if (totalLen == 0)
    return tmpStr;

  tmpStr.fLength = 0;
  tmpStr.fText = new char[totalLen + 1];

  if (tmpStr.fText != NULL)
    {
      tmpStr.fText[0] = '\0';

      if (str1.fText != NULL)
        {
          memcpy (tmpStr.fText, str1.fText, (size_t)str1.fLength);
          tmpStr.fLength = str1.fLength;
        }

      if (str2.fText != NULL)
        memcpy (&tmpStr.fText[tmpStr.fLength], str2.fText,
                (size_t)str2.fLength);

      tmpStr.fLength = totalLen;
    }

  return tmpStr;
}

CString
operator+ (CString &str, char chr)
{
  CString tmpStr;
  char tmpChr = chr;

  if (str.fText == NULL)
    {
      tmpStr.fLength = 1;
      tmpStr.fText = str.CloneText (&tmpChr, 1);

      if (tmpStr.fText != NULL)
        {
          tmpStr.fText[0] = tmpChr;
          tmpStr.fText[1] = '\0';
        }
      else
        tmpStr.fLength = 0;
    }
  else
    {
      tmpStr.fLength = str.fLength + 1;
      tmpStr.fText = str.CloneText (str.fText, tmpStr.fLength);

      if (tmpStr.fText != NULL)
        {
          tmpStr.fText[str.fLength] = tmpChr;
          tmpStr.fText[tmpStr.fLength] = '\0';
        }
      else
        tmpStr.fLength = 0;
    }

  return tmpStr;
}

StrCompVal
CString::Compare (CString &str, int caseSense)
{
  char chr1, chr2;
  long i, nChars;

  if (fText == NULL)
    {
      if (str.fText == NULL)
        return kEqual;
      else
        return kLessThan;
    }
  else if (str.fText == NULL)
    return kGreaterThan;

  if (str.fLength < fLength)
    nChars = str.fLength;
  else
    nChars = fLength;

  if (caseSense == 0)
    {
      for (i = 0; i < nChars; ++i)
        {
          chr1 = (char)tolower (fText[i]);
          chr2 = (char)tolower (str.fText[i]);

          if (chr1 != chr2)
            {
              if (chr1 < chr2)
                return kLessThan;
              else
                return kGreaterThan;
            }
        }
    }
  else
    {
      for (i = 0; i < nChars; ++i)
        {
          chr1 = fText[i];
          chr2 = str.fText[i];

          if (chr1 != chr2)
            {
              if (chr1 < chr2)
                return kLessThan;
              else
                return kGreaterThan;
            }
        }
    }

  if (fLength == str.fLength)
    return kEqual;
  else
    {
      if (fLength < str.fLength)
        return kLessThan;
      else
        return kGreaterThan;
    }
}

void
CString::Delete (long strPos, long nChars)
{
  long newLen, i, j = 0;

  if ((fText == NULL) || ((strPos + nChars - 1) > fLength))
    return;

  newLen = fLength - nChars;

  if ((fLength - newLen) > fLength)
    {
      char *pTefText = new char[newLen + 1];

      if (pTefText != NULL)
        {
          for (i = 0; i <= fLength; ++i)
            {
              if (i == strPos)
                i += nChars;

              pTefText[j++] = fText[i];
            }

          delete fText;

          fText = pTefText;
        }
      else
        newLen = fLength;
    }
  else
    {
      char *pTefText = fText;

      for (i = 0; i <= fLength; ++i)
        {
          if (i == strPos)
            i += nChars;

          pTefText[j++] = fText[i];
        }
    }

  fLength = newLen;
}

void
CString::Insert (long strPos, char chr)
{
  if (strPos > fLength)
    return;

  if (fText == NULL)
    {
      char tmpChr = chr;

      fLength = 1;
      fText = this->CloneText (&tmpChr, fLength);

      if (fText != NULL)
        {
          fText[1] = '\0';
          return;
        }
      else
        fLength = 0;
    }
  else
    {
      long newLen = fLength + 1;
      long i, j = 0;
      char *tmpStr = new char[newLen];

      if (tmpStr != NULL)
        {
          for (i = 0; i <= fLength; ++i)
            {
              if (i == strPos)
                tmpStr[j++] = chr;

              tmpStr[j++] = fText[i];
            }

          delete fText;

          fText = tmpStr;
          fLength = newLen;
        }
    }
}

void
CString::Insert (long strPos, CString &str)
{
  if (str.fText == NULL || strPos > fLength)
    return;

  if (fText == NULL)
    {
      *this = str;
    }
  else
    {
      long totalLen = str.fLength + fLength;
      long i, j, k = 0;

      if (totalLen > fLength)
        {
          char *tmpStr = new char[totalLen + 1];

          for (i = 0; i <= fLength; ++i)
            {
              if (i == strPos)
                {
                  for (j = 0; j < str.fLength; ++j)
                    tmpStr[k++] = str.fText[j];
                }

              tmpStr[k++] = fText[i];
            }

          delete fText;

          fText = tmpStr;
        }
      else
        {
          for (i = fLength + str.fLength; i > strPos + str.fLength; --i)
            fText[i] = fText[i - str.fLength];

          for (i = 0; i < str.fLength; ++i)
            fText[strPos + i] = str.fText[i];
        }

      fLength = totalLen;
    }
}

char *
CString::CloneText (char *pStr, long nChars)
{
  char *pTmp = new char[nChars + 1];

  if (pTmp)
    memcpy (pTmp, pStr, (size_t)(nChars + 1));

  return pTmp;
}
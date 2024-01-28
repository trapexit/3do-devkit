/*
        File:		CString.h

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
#ifndef _CSTRING_H_
#define _CSTRING_H_

enum StrCompVal
{
  kLessThan,
  kEqual,
  kGreaterThan
};

class CString
{
public:
  CString (void);
  CString (CString &str);
  CString (char *pCStr);
  virtual ~CString (void);

  long Length (void);

  // return a c-string from CString
  operator char * ();

  CString operator= (CString &str);

  friend CString operator+ (CString &str1, CString &str2);
  friend CString operator+ (CString &str1, char chr);

  void operator+= (CString &str);
  void operator+= (char chr);

  int operator<(CString &str);
  int operator> (CString &str);
  int operator<= (CString &str);
  int operator>= (CString &str);
  int operator== (CString &str);
  int operator!= (CString &str);

  StrCompVal Compare (CString &str, int caseSense = 0);

  void Delete (long strPos, long nChars);

  void Insert (long strPos, char chr);
  void Insert (long strPos, CString &str);

  char operator[] (long strPos);

private:
  char *CloneText (char *pStr, long nChars);

  long fLength;
  char *fText;
};

inline long
CString::Length (void)
{
  return fLength;
}

inline void
CString::operator+= (CString &str)
{
  *this = *this + str;
}

inline void
CString::operator+= (char chr)
{
  *this = *this + chr;
}

inline CString::operator char * (void) { return fText; }

inline int
CString::operator<(CString &str)
{
  return (this->Compare (str) == kLessThan);
}

inline int
CString::operator> (CString &str)
{
  return (this->Compare (str) == kGreaterThan);
}

inline int
CString::operator<= (CString &str)
{
  return (this->Compare (str) != kGreaterThan);
}

inline int
CString::operator>= (CString &str)
{
  return (this->Compare (str) != kLessThan);
}

inline int
CString::operator== (CString &str)
{
  return (this->Compare (str) == kEqual);
}

inline int
CString::operator!= (CString &str)
{
  return (this->Compare (str) != kEqual);
}

inline char
CString::operator[] (long strPos)
{
  return (strPos >= fLength) ? '\0' : fText[strPos];
}

#endif

/*
        File:		CList.cp

        Contains:	List management classes

        Written by:	Paul A. Bauersfeld
                                        Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CList.h"

#include "CPlusSwiHack.h"

CListLink::CListLink (ListEntry e, CListLink *r)
{
  entry = e;
  next = r;
}

CListLink::~CListLink (void)
{
  // nothing to do
}

CList::CList (void) { last = 0L; }

CList::CList (ListEntry e)
{
  last = new CListLink (e, 0);

  if (last)
    last->next = last;
}

CList::~CList (void) {}

long
CList::Append (ListEntry listEntry)
{
  if (last)
    {
      last->next = new CListLink (listEntry, last->next);
      if (last)
        last = last->next;
    }
  else
    {
      last = new CListLink (listEntry, 0);
      if (last)
        last->next = last;
    }

  return 0L;
}

long
CList::Count (void)
{
  register long count = 0L;
  CLLIterator iterator (this);

  while (iterator.Operator ())
    count++;
  return count;
}

ListEntry
CList::GetNth (long n)
{
  register long count = 0L;
  register ListEntry listEntry;
  CLLIterator iterator (this);

  do
    {
      listEntry = iterator.Operator ();
    }
  while (listEntry && (count++ != n));

  return listEntry;
}

long
CList::Insert (ListEntry listEntry)
{
  if (last)
    {
      last->next = new CListLink (listEntry, last->next);
    }
  else
    {
      last = new CListLink (listEntry, 0);

      if (last)
        last->next = last;
    }

  return 0L;
}

void
CList::Remove (long n)
{
  long cnt = Count ();

  if (n > cnt)
    return;

  if (cnt != 0L)
    {
      CListLink *prevLink = last->next;
      CListLink *thisLink;

      if (n > 0)
        {
          for (long ni = 0; ni < (n - 1); ni++)
            prevLink = prevLink->next;

          thisLink = prevLink->next;
          if ((cnt - 1) == n)
            {
              prevLink->next = last->next;
              last = prevLink;
            }
          else
            {
              prevLink->next = (prevLink->next)->next;
            }
        }
      else
        {
          thisLink = prevLink;
          last->next = prevLink->next;
        }

      // the client is the only one that knows about this. don't delete!
      // delete thisLink->entry;

      delete thisLink;
    }
}

CLLIterator::CLLIterator (CList *s)
{
  fSaveList = s;
  this->Reset ();
}

CLLIterator::~CLLIterator (void)
{
  // nothing to do
}

void
CLLIterator::Reset (void)
{
  fList = fSaveList;
  fLink = fList->last;
}

ListEntry
CLLIterator::Operator (void)
{
  ListEntry listEntry;

  if (fLink != 0L)
    {
      fLink = fLink->next;
      listEntry = fLink->entry;
    }
  else
    listEntry = 0L;

  if (fLink == fList->last)
    fLink = 0;

  return listEntry;
}

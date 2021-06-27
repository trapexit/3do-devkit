/*
	File:		CList.h

	Contains:	List management classes

	Written by:	Paul A. Bauersfeld
					Jon A. Weiner

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

	<1>	 10/28/93	pab		New today.

	To Do:
*/
#ifndef _CLIST_H_ 
#define _CLIST_H_

typedef void *ListEntry;

class CListLink
{
	friend class CListLink;
	friend class CLLIterator;
	
	public:
		CListLink *next;
		ListEntry entry;
	
		CListLink(ListEntry e, CListLink *r);
		virtual ~CListLink(void);
};

class CList
{
	friend class CLLIterator;

	CListLink *last;
	
	public:
		CList(void);
		CList(ListEntry e);
		virtual ~CList(void);

		long Append(ListEntry e);
		long Count(void);
		ListEntry GetNth(long n);
		long Insert(ListEntry e);
		void Remove(long n);
};

class CLLIterator 
{
	public:
		CLLIterator(CList *s);
		virtual ~CLLIterator(void);
		
		ListEntry Operator(void);
		void Reset(void);

	private:
		CListLink *fLink;
		CList *fList;
		CList *fSaveList;
};
#endif

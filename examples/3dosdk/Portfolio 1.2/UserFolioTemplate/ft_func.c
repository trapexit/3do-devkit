/*
	File:		ft_func.c

	Contains:	This module contains the functions which are accessed
				at run time by applications calling the folio.

	Written by:	Craig Weisenfluh

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	  4/6/93	JAY		first checked in

	To Do:
*/

#include "ft.h"

	// this function takes no parameters, and returns an integer
int32 ftTestFolio (void )
{
	kprintf("\nftTestFolio - no parameters\n");
	return 1;
}

	// this function takes two parameters
void *ftTestFolioParms (int n, char *p )
{
	kprintf("\nftTestFolioParms '%d' '%s'\n", n, p);
	return (void *)1;
}

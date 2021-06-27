#include "mem.h"

void *__nw__FUi(unsigned int );
void __dl__FPv(void *);

void *__nw__FUi(unsigned int x) 
{
	if (x != 0L)
		return malloc(x);
	else
		return 0L;
}

void __dl__FPv(void *x) 
{
	if (x != 0L)
		free(x);
}

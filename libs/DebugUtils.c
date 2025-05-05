#ifndef DEBUGUTILS_C
#define DEBUGUTILS_C

#include "../includes/DebugUtils.h"
#include <stdio.h>

void print_buffer(const char* buffer, int buffer_size)
{
	putchar('\n');
	for ( int j = 0; j < buffer_size; j++ )
	{
		printf("%4d ", buffer[j]);
		if ( ((j+1) % 10) == 0 )
			putchar('\n');
	}
	putchar('\n');
	fflush(stdout);
}


#endif

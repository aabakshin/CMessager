#include <stdio.h>
#include "StringList.h"
#include <stdlib.h>

int main(int argc, char** argv)
{
	StringList* slist = NULL;

	int size = sl_get_size(slist);
	printf("list size: %d\n", size);

	const char* strings[] = 
	{
			"string1",
			"string2",
			"string3",
			"string4",
			"string5",
			NULL
	};

	sl_insert(&slist, strings[0]);
	sl_print(slist);
	sl_remove(&slist, strings[0]);
	sl_print(slist);
	sl_insert(&slist, strings[1]);
	sl_insert(&slist, strings[2]);
	sl_insert(&slist, strings[0]);
	sl_insert(&slist, strings[3]);

	sl_print(slist);
	
	sl_remove(&slist, strings[0]);

	sl_print(slist);

	sl_remove(&slist, strings[3]);

	sl_print(slist);

	sl_remove(&slist, strings[1]);

	sl_print(slist);

	sl_clear(&slist);

	return 0;
}

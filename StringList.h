#ifndef STRINGLIST_H_SENTRY
#define STRINGLIST_H_SENTRY

enum
{
	DATA_SIZE = 21
};

struct StringList 
{
	char data[DATA_SIZE];
	struct StringList* next;	
	struct StringList* prev;
};
typedef struct StringList StringList;


int sl_get_size(StringList* list);
void sl_insert(StringList** list_ptr, const char* data);
const char* sl_remove(StringList** list_ptr, const char* data);
int sl_clear(StringList** list_ptr);
void sl_print(StringList* list);

#endif

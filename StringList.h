/* 
 * Модуль StringList предоставляет контейнер для хранения строк,
 * а также интерфейс для работы с контейнером
 *
 */

#ifndef STRINGLIST_H_SENTRY
#define STRINGLIST_H_SENTRY

/* Системная константа */
enum
{
	DATA_SIZE = 21
};

/* Структура для хранения элемента контейнера */
struct StringList 
{
	char data[DATA_SIZE];
	struct StringList* next;	
	struct StringList* prev;
};
typedef struct StringList StringList;

/* Интерфейс для работы с контейнером */
int sl_get_size(StringList* list);
void sl_insert(StringList** list_ptr, const char* data);
const char* sl_remove(StringList** list_ptr, const char* data);
int sl_clear(StringList** list_ptr);
void sl_print(StringList* list);

#endif

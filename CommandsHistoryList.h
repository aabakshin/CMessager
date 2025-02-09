/*
 *	Модуль CommandsHistoryList представляет контейнер для хранения	
 *	истории команд, введённых пользователем. А также предоставляет
 *	интерфейс для взаимодействия с контейнером.
 *	Этот модуль вызывается только в модуле clientCore 
 */


#ifndef COMMANDS_HISTORY_LIST_H
#define COMMANDS_HISTORY_LIST_H

/* Системная константа */
enum
{
	COMMAND_SIZE = 100
};

/* Структура, представляющая элемент контейнера */
struct CommandsHistoryList
{
	int number; 
	char* command;
	int command_size;
	struct CommandsHistoryList* next;
	struct CommandsHistoryList* prev;
};
typedef struct CommandsHistoryList CommandsHistoryList;

/* Интерфейс для взаимодействия с контейнером */
int chl_is_empty(CommandsHistoryList* node);
int chl_insert(CommandsHistoryList** nodePtr, char* str, int str_size);
int chl_delete(CommandsHistoryList** nodePtr, int num);
int chl_clear(CommandsHistoryList** nodePtr);
int chl_get_size(CommandsHistoryList* node);
int chl_print(CommandsHistoryList* node);

#endif

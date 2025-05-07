#ifndef COMMAND_HISTORY_LIST_C
#define COMMAND_HISTORY_LIST_C

#include "../includes/CommandsHistoryList.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int chl_print(CommandsHistoryList* node)
{
	if ( node == NULL )
	{
		printf("%s", "\nList is empty\n");
		return 0;
	}

	int size = chl_get_size(node);
	printf("%s", "\nList is: ");
	int i;
	for ( i = 1; i <= size; i++ )
	{
		printf("%d (%s) --> ", node->number, node->command);
		node = node->next;
	}
	printf("%s\n\n", "NULL");

	return 1;
}

int chl_is_empty(CommandsHistoryList* node)
{
	return node == NULL;
}

int chl_insert(CommandsHistoryList** nodePtr, char* str, int str_size)
{
	if ( nodePtr == NULL )
		return 0;

	if ( *nodePtr == NULL )
	{
		CommandsHistoryList* newPtr = NULL;

		newPtr = malloc(sizeof(struct CommandsHistoryList));
		if ( newPtr == NULL )
			return 0;

		newPtr->number = 1;

		newPtr->command = malloc(sizeof(char) * str_size);
		if ( newPtr->command == NULL )
		{
			if ( newPtr )
				free(newPtr);

			return 0;
		}
		newPtr->command_size = str_size;

		int i;
		for ( i = 0; i < str_size-1; i++ )
			newPtr->command[i] = str[i];
		newPtr->command[i] = '\0';

		newPtr->next = NULL;
		newPtr->prev = NULL;
		*nodePtr = newPtr;
	}
	else
	{
		int cur_num = (*nodePtr)->number;
		CommandsHistoryList* curPtr = *nodePtr;

		/* Убрать дубликаты */
		/*int k = chl_get_size(*nodePtr);
		int i;
		for ( i = 1; i <= k; i++ )
		{
			if ( strcmp(str, curPtr->command) == 0 )
				return 0;

			curPtr = curPtr->next;
		}
		curPtr = *nodePtr;*/

		CommandsHistoryList* newPtr = NULL;
		newPtr = malloc(sizeof(struct CommandsHistoryList));
		if ( newPtr == NULL )
			return 0;

		newPtr->command = malloc(sizeof(char) * str_size);
		if ( newPtr->command == NULL )
		{
			if ( newPtr )
				free(newPtr);

			return 0;
		}

		newPtr->command_size = str_size;

		int i;
		for ( i = 0; i < str_size-1; i++ )
			newPtr->command[i] = str[i];
		newPtr->command[i] = '\0';

		newPtr->next = *nodePtr;
		(*nodePtr)->prev = newPtr;

		while ( (curPtr->next != NULL) && (curPtr->next->number != (*nodePtr)->number) )
		{
			cur_num++;
			curPtr = curPtr->next;
		}

		cur_num++;
		newPtr->prev = curPtr;
		newPtr->number = cur_num;
		curPtr->next = newPtr;
	}

	return 1;
}

int chl_delete(CommandsHistoryList** nodePtr, int num)
{
	if ( nodePtr == NULL )
		return 0;

	if ( *nodePtr == NULL )
		return 0;

	CommandsHistoryList* curPtr = *nodePtr;

	if ( curPtr->prev == NULL )
	{
		int deleteValue = curPtr->number;

		if ( curPtr->command != NULL )
			free(curPtr->command);
		free(curPtr);
		return deleteValue;
	}

	while ( curPtr->number != num )
	{
		if ( curPtr->next->number == (*nodePtr)->number )
			return 0;

		curPtr = curPtr->next;
	}

	int deleteValue = curPtr->number;

	if ( chl_get_size(*nodePtr) <= 2 )
	{
		if ( deleteValue == (*nodePtr)->number )
		{
			curPtr->next->prev = NULL;
			curPtr->next->next = NULL;
			*nodePtr = curPtr->next;
		}
		else
		{
			curPtr->prev->next = NULL;
			curPtr->prev->prev = NULL;
		}

		if ( curPtr->command != NULL )
			free(curPtr->command);
		free(curPtr);

		return deleteValue;
	}

	if ( deleteValue == (*nodePtr)->number )
		*nodePtr = curPtr->next;

	curPtr->next->prev = curPtr->prev;
	curPtr->prev->next = curPtr->next;
	curPtr->next = NULL;
	curPtr->prev = NULL;
	if ( curPtr->command != NULL )
		free(curPtr->command);
	free(curPtr);

	return deleteValue;
}

int chl_clear(CommandsHistoryList** nodePtr)
{
	if ( nodePtr == NULL )
		return 0;

	if ( *nodePtr == NULL )
		return 0;

	CommandsHistoryList* node = *nodePtr;
	int size = chl_get_size(node);

	int i;
	for ( i = 1; i <= size; i++ )
		chl_delete(nodePtr, (*nodePtr)->number);

	return 1;
}

int chl_get_size(CommandsHistoryList* node)
{
	if ( node == NULL )
		return 0;

	int first_num = node->number;
	int size = 1;
	while ( (node->next != NULL) && (node->next->number != first_num) )
	{
		node = node->next;
		size++;
	}

	return size;
}

#endif

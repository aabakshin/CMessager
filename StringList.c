/* Файл реализации модуля StringList */

#ifndef STRINGLIST_C_SENTRY
#define STRINGLIST_C_SENTRY

#include "StringList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sl_insert(StringList** list_ptr, const char* data)
{
	if ( (list_ptr == NULL) || (data == NULL) )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_insert\" error: \"list_ptr\" or \"data\" is NULL\n");
		return;
	}

	StringList* newPtr = NULL;
	newPtr = malloc(sizeof(struct StringList));
	if ( !newPtr )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_insert\" memory error\n");
		return;
	}

	int i;
	for (i = 0; data[i]; i++)
		newPtr->data[i] = data[i];
	newPtr->data[i] = '\0';
	newPtr->next = NULL;
	newPtr->prev = NULL;

	StringList* prevPtr = NULL;
	StringList* curPtr = *list_ptr;

	while (curPtr != NULL)
	{
		prevPtr = curPtr;
		curPtr = curPtr->next;
	}

	if (prevPtr == NULL)
		*list_ptr = newPtr;
	else
	{
		prevPtr->next = newPtr;
		newPtr->prev = prevPtr;
	}
}

const char* sl_remove(StringList** list_ptr, const char* data)
{
	if ( (list_ptr == NULL) || (data == NULL) || (strcmp(data, "") == 0) )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_remove\" error: \"list_ptr\" or \"data\" is NULL\n");
		return NULL;
	}

	StringList* tempPtr = NULL;
	StringList* prevPtr = NULL;
	StringList* curPtr  = *list_ptr;

	while ( (curPtr != NULL) && (strcmp(curPtr->data, data) != 0) )
	{
		prevPtr = curPtr;
		curPtr = curPtr->next;
	}

	if ( curPtr == NULL )
	{
		fprintf(stderr, "\n[StringList]: In function \"sl_remove\" error: \"%s\" value is not found in list!\n", data);
		return NULL;
	}

	tempPtr = curPtr;

	if ( prevPtr == NULL )
	{
		if ( curPtr->next == NULL )
		{
			*list_ptr = NULL;
		}
		else
		{
			(*list_ptr)->next->prev = NULL;
			*list_ptr = (*list_ptr)->next;

		}
		
		free(tempPtr);

		return data;
	}
	
	if ( curPtr->next == NULL )
	{
		prevPtr->next = NULL;
		free(tempPtr);
		
		return data;
	}
	
	prevPtr->next = curPtr->next;
	curPtr->next->prev = prevPtr;
	free(tempPtr);

	return data;
}

int sl_get_size(StringList* list)
{
	if ( list == NULL )
	{
		return 0;
	}

	int size = 0;
	while ( list != NULL )
	{
		size++;
		list = list->next;
	}
	
	return size;
}

int sl_clear(StringList** list_ptr)
{
	if ( list_ptr == NULL )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_clear\" error: \"list_ptr\" is NULL\n");
		return 0;
	}

	int size = sl_get_size(*list_ptr);
	int removed = 0;
	int i;
	for ( i = 1; i <= size; i++ )
	{
		StringList* list = *list_ptr;
		if ( sl_remove(list_ptr, list->data) != NULL )
			removed++;
	}
	
	if ( removed == size )
		return 1;
	
	fprintf(stderr, "%s", "\n[StringList]: In function \"sl_clear\" error: unable to clear list\n");
	return 0;
}

void sl_print(StringList* list)
{
	if ( list == NULL )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_print\" error: list is empty!\n");
		return;
	}
	
	printf("%s", "\nStringList is: ");
	while ( list != NULL )
	{
		printf("%s --> ", list->data);
		list = list->next;
	}
	printf("%s\n\n", "NULL");
}

#endif

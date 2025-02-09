#ifndef SESSION_LIST_C_SENTRY
#define SESSION_LIST_C_SENTRY

#include "SessionList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sess_insert(Session** list_ptr, const ClientData* data)
{
	if ( (list_ptr == NULL) || (data == NULL) )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_insert\" error: \"list_ptr\" or \"data\" is NULL\n");
		return;
	}

	Session* newPtr = NULL;
	newPtr = malloc(sizeof(struct Session));
	if ( !newPtr )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_insert\" memory error\n");
		return;
	}

	newPtr->data = (ClientData*) data;
	newPtr->next = NULL;
	newPtr->prev = NULL;

	Session* prevPtr = NULL;
	Session* curPtr = *list_ptr;

	while ( curPtr != NULL )
	{
		prevPtr = curPtr;
		curPtr = curPtr->next;
	}

	if ( prevPtr == NULL )
		*list_ptr = newPtr;
	else
	{
		prevPtr->next = newPtr;
		newPtr->prev = prevPtr;
	}
}

const ClientData* sess_remove(Session** list_ptr, const ClientData* data)
{
	if ( (list_ptr == NULL) || (data == NULL) )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_remove\" error: \"list_ptr\" or \"data\" is NULL\n");
		return NULL;
	}

	Session* tempPtr = NULL;
	Session* prevPtr = NULL;
	Session* curPtr  = *list_ptr;

	while ( (curPtr != NULL) && (curPtr->data != data) )
	{
		prevPtr = curPtr;
		curPtr = curPtr->next;
	}

	if ( curPtr == NULL )
	{
		fprintf(stderr, "\n[StringList]: In function \"sl_remove\" error: \"%p\" value is not found in list!\n", data);
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

int sess_get_size(Session* list)
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

int sess_clear(Session** list_ptr, int clear)
{
	if ( list_ptr == NULL )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_clear\" error: \"list_ptr\" is NULL\n");
		return 0;
	}

	int size = sess_get_size(*list_ptr);
	int removed = 0;
	int i;
	for ( i = 1; i <= size; i++ )
	{
		const ClientData* list = NULL;
		if ( (list = sess_remove(list_ptr, (*list_ptr)->data)) != NULL )
		{
			if ( clear )
				if ( list )
					free((void*)list);

			removed++;
		}
	}
	
	if ( removed == size )
		return 1;
	
	fprintf(stderr, "%s", "\n[StringList]: In function \"sl_clear\" error: unable to clear list\n");
	return 0;
}

void sess_print(Session* list)
{
	if ( list == NULL )
	{
		fprintf(stderr, "%s", "\n[StringList]: In function \"sl_print\" error: list is empty!\n");
		return;
	}
	
	int i = 0;

	printf("%s", "\nStringList is: ");
	while ( list != NULL )
	{
		i++;
		printf("[%d] (%s) --> ", list->data->fd, list->data->addr);
		if ( (i % 5) == 0 )
			putchar('\n');

		list = list->next;
	}
	printf("%s\n\n", "NULL");
}

#endif

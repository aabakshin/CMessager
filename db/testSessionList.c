#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SessionList.h"

static const char* addr1 = "192.168.50.128";
static const char* addr2 = "127.0.0.1";
static const char* addr3 = "ya.xochu.pitsu.sru";

int main(int argc, char** argv)
{
	ClientData* data1 = NULL;
	data1 = malloc(sizeof(ClientData));
	data1->fd = 0;
	
	int i;
	for ( i = 0; addr1[i]; i++ )
		data1->addr[i] = addr1[i];
	data1->addr[i] = '\0';


	ClientData* data2 = NULL;
	data2 = malloc(sizeof(ClientData));
	data2->fd = 1;
	
	for ( i = 0; addr2[i]; i++ )
		data2->addr[i] = addr2[i];
	data2->addr[i] = '\0';


	ClientData* data3 = NULL;
	data3 = malloc(sizeof(ClientData));
	data3->fd = 2;
	
	for ( i = 0; addr3[i]; i++ )
		data3->addr[i] = addr3[i];
	data3->addr[i] = '\0';

	
	Session* sess_list = NULL;

	int size = sess_get_size(sess_list);
	printf("size = %d\n", size);

	sess_insert(&sess_list, data1);

	size = sess_get_size(sess_list);
	printf("size = %d\n", size);

	sess_insert(&sess_list, data2);

	size = sess_get_size(sess_list);
	printf("size = %d\n", size);

	sess_insert(&sess_list, data3);

	size = sess_get_size(sess_list);
	printf("size = %d\n", size);

	sess_print(sess_list);

	/*
	ClientData* data4 = NULL;
	data4 = malloc(sizeof(ClientData));
	data4->fd = 0;

	for ( i = 0; addr1[i]; i++ )
		data4->addr[i] = addr1[i];
	data4->addr[i] = '\0';
	*/

	const ClientData* removed = sess_remove(&sess_list, data3);
	if ( removed )
		free((void*) removed);
	
	sess_print(sess_list);
	size = sess_get_size(sess_list);
	printf("size = %d\n", size);

	sess_clear(&sess_list, 1);

	size = sess_get_size(sess_list);
	printf("size = %d\n", size);



	return 0;
}


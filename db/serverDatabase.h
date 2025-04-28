#ifndef SERVERDATABASE_H_SENTRY
#define SERVERDATABASE_H_SENTRY

#include "SessionList.h"
#include <stdio.h>

enum
{
			STR_BUF_SIZE			=			100
};

typedef struct
{
	int ls;
	FILE* cfg_fd;
	FILE* user_table_fd;
	FILE* sess_table_fd;
	int db_records_num;
	char user_table_name[STR_BUF_SIZE];
	char sess_table_name[STR_BUF_SIZE];

} InitDbServData;

typedef struct
{
	InitDbServData* server_data; 
	Session* sess_list;
} Server;


int db_server_init(int port, InitDbServData* server_data);
int db_server_running(Server* serv_ptr);

#endif

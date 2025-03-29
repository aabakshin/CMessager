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
	int db_records_num;
	char userinfo_table_name[STR_BUF_SIZE];
	char usersessions_table_name[STR_BUF_SIZE];
	Session* sess_list;
} Server;


int db_server_init(int port);
int db_server_running(Server* serv_ptr);

#endif

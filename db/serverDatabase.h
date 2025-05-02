#ifndef SERVERDATABASE_H_SENTRY
#define SERVERDATABASE_H_SENTRY

#include "SessionList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

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

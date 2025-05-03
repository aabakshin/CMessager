#ifndef SERVERDATABASE_H_SENTRY
#define SERVERDATABASE_H_SENTRY

#include "SessionList.h"
#include "Config.h"
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


typedef struct
{
	int ls;
	FILE* cfg_fd;
	FILE* user_table_fd;
	FILE* sess_table_fd;
	ConfigFields* cfg;
} InitDbServData;

typedef struct
{
	InitDbServData* server_data;
	Session* sess_list;
} Server;


int db_server_init(int port, InitDbServData* server_data, ConfigFields* cfg_values);
int db_server_running(Server* serv_ptr);

#endif

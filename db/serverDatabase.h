/* Модуль предоставляет интерфейс для взаимодействия с внутренним сервером базы данных */

#ifndef SERVERDATABASE_H_SENTRY
#define SERVERDATABASE_H_SENTRY

#include "SessionList.h"

typedef struct
{
	int ls;
	int db_records_num;
	char userinfo_table_name[100];
	char usersessions_table_name[100];
	Session* sess_list;
} Server;

/* интерфейс инициализации и работы главного цикла сервера БД */
int db_server_init(int port);
int db_server_running(Server* serv_ptr);

#endif

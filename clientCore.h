/*
 * Модуль clientCore содержит функции для инициализации соединения, 
 * а также обработки серверных ответтов
 
 */

#ifndef CLIENTCORE_H_SENTRY
#define CLIENTCORE_H_SENTRY

#include "CommandsHistoryList.h"

enum 
{
	EXIT_CODE						=			 -2,
	MAX_RESPONSE_ARGS_NUM			=			100,
	SELECT_TIMER_SEC				=			  0,
	SELECT_TIMER_MSEC				=			300,
	HISTORY_COMMANDS_LIST_SIZE		=			  5
};

/* Следующие константы используются для настройки чувствительности антиспам модуля */
enum
{
	ANTISPAM_MODULE_TRIES_NUMBER			=			   2,
	ANTISPAM_MODULE_MSG_CNT					=			   5,
	ANTISPAM_MODULE_TOTAL_TIME_MS			=			5000,
	ANTISPAM_MODULE_MESSAGES_INTERVAL		=		     300
};

int get_str(char* buffer, int buffer_size);
int check_server_response(int peer_sock, char** response_tokens, int response_tokens_size, int* authorized);
int client_init(const char* address, const char* port);

#endif

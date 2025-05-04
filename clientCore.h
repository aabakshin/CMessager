#ifndef CLIENTCORE_H_SENTRY
#define CLIENTCORE_H_SENTRY

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

enum
{
	EXIT_CODE						=			 -1,
	MAX_RESPONSE_ARGS_NUM			=			100,
	SELECT_TIMER_SEC				=			  0,
	SELECT_TIMER_MSEC				=			300,
	HISTORY_COMMANDS_LIST_SIZE		=			  5,
	CAPTCHA_CODE_LENGTH				=			  4,
	MAX_MESSAGE_LENGTH				=			200
};

enum
{
	ANTISPAM_MODULE_TRIES_NUMBER			=			   2,
	ANTISPAM_MODULE_MSG_CNT					=			   5,
	ANTISPAM_MODULE_TOTAL_TIME_MS			=			5000,
	ANTISPAM_MODULE_MESSAGES_INTERVAL		=		     300
};


int check_server_response(int peer_sock, char** response_tokens, int response_tokens_size, int* authorized);
int client_init(const char* address, const char* port);
int restrict_message_length(char* read);
void delete_extra_spaces(char* read, int read_size);
char* get_code(void);

#endif

#ifndef SERVERAPI_H_SENTRY
#define SERVERAPI_H_SENTRY

#include "DatabaseStructures.h"
#include "StringList.h"

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
#include <signal.h>
#include <time.h>

#define SERVER_DATABASE_ADDR			"192.168.88.128"
#define SERVER_DATABASE_PORT			"7778"


/*A format of parsed table record from server database */
enum
{
				DB_LINE_EXIST,
				ID,
				USERNAME,
				PASS,
				RANK,
				REALNAME,
				AGE,
				QUOTE,
				MUTED,
				START_MUTE_TIME,
				MUTE_TIME,
				MUTE_TIME_LEFT,
				LAST_IP,
				LAST_DATE_IN,
				LAST_DATE_OUT,
				REGISTRATION_DATE
};

enum fsm_states
{
	fsm_start,
	fsm_client_has_account = fsm_start,
	fsm_login_process_wait_login,
	fsm_login_process_wait_pass,
	fsm_signup_wait_login,
	fsm_signup_wait_pass,
	fsm_wait_message,
	fsm_finish,
	fsm_error
};

enum
{
	SECRET_NUMBER				=			 900
};

enum status
{
	status_offline,
	status_online,
	status_busy,
	status_away,
	status_work,
	status_invisible = SECRET_NUMBER
};

typedef struct
{
	int ID;
	int sockfd;
	char buf[BUFFER_SIZE];
	int buf_used;
	enum fsm_states state;
	char login[LOGIN_SIZE];
	char pass[PASS_SIZE];
	char last_ip[LAST_IP_SIZE];
	char last_date_in[LAST_DATE_IN_SIZE];
	char registration_date[REG_DATE_SIZE];
	int rank;
	int authorized;
	enum status user_status;
	int muted;
	int start_mute_time;
	int mute_time;
	int mute_time_left;
} ClientSession;

enum
{
		CMD_CODE_INTERNAL_SERVER_ERROR	=			-3,
		CMD_CODE_OVERLIMIT_LENGTH		=			-2,
		CMD_CODE_UNKNOWN_COMMAND		=			-1,
		CMD_CODE_TEXT_MESSAGE			=			 0,
		CMD_CODE_COMMAND_HELP			=			 1,
		CMD_CODE_COMMAND_WHOIH			=			 2,
		CMD_CODE_COMMAND_CHGPASS		=			 3,
		CMD_CODE_COMMAND_OP				=			 4,
		CMD_CODE_COMMAND_DEOP			=			 5,
		CMD_CODE_COMMAND_PM				=			 6,
		CMD_CODE_COMMAND_STATUS			=			 7,
		CMD_CODE_COMMAND_RECORD			=			 8,
		CMD_CODE_COMMAND_MUTE			=			 9,
		CMD_CODE_COMMAND_UNMUTE			=			10,
		CMD_CODE_COMMAND_KICK			=			11,
		CMD_CODE_COMMAND_TABLE			=			12,
		CMD_CODE_COMMAND_BAN			=			13,
		CMD_CODE_COMMAND_UNBAN			=			14,
		QUEUE_SOCK_LEN					=			16
};

enum
{
		FRESHMAN_RANK_VALUE				=			 1,
		MEMBER_RANK_VALUE				=			 2,
		WISDOM_RANK_VALUE				=			 3,
		OLDMAN_RANK_VALUE				=			 4,
		ADMIN_RANK_VALUE				=		   617
};

typedef struct
{
	int ls;
	int db_sock;
	ClientSession** sess_array;
	long long sess_array_size;
	StringList* clients_online;
} Server;


/* Client interface */
void session_send_string(ClientSession *sess, const char *str);

/* low-level interface function to make request to database */
int request_to_db(Server* serv_ptr, char* response, int response_size, const char** query_strings);
/* high-level interface function to make request to database */
int get_field_from_db(Server* serv_ptr, char* field, const char* search_key, int field_code);

/* Server interface */
int is_valid_auth_str(const char *user_auth_str, int authentication);
int server_init(int port, Server* serv_ptr);
void server_close_session(int sock_num, Server* serv_ptr);
void server_force_stop(Server* serv_ptr);
int server_running(Server* serv_ptr);

#endif

#ifndef SERVERCORE_C_SENTRY
#define SERVERCORE_C_SENTRY

#include "DatabaseStructures.h"
#include "DateTime.h"
#include "StringList.h"
#include "Commons.h"
#include "serverCore.h"
#include "serverCommands.h"
#include "serverConfigs.h"
#include <signal.h>


static ClientSession* make_new_session(int sockfd, struct sockaddr_in *from);
static void session_handler_has_account(ClientSession* sess, const char* client_line);
static void session_handler_login_wait_login(ClientSession* sess, const char* client_line);
static void send_message_authorized(ClientSession* sess, const char* str);
static void success_new_authorized(ClientSession* sess);
static void session_handler_login_wait_pass(ClientSession* sess, const char* client_line);
static void session_handler_signup_wait_login(ClientSession* sess, const char* client_line);
static void session_handler_signup_wait_pass(ClientSession* sess, const char* client_line);
static void session_handler_wait_message(ClientSession* sess, const char* client_line);
static void session_fsm_step(ClientSession* sess, char* client_line);
static void session_check_lf(ClientSession* sess);
static int session_do_read(ClientSession* sess);
static int server_accept_client(void);

StringList* clients_online = NULL;
Server* serv = NULL;

static int exit_flag = 0;
static int sig_number = 0;

enum { TIMER_VALUE			=			10 };

void alrm_handler(int signo)
{
	char cur_time[CURRENT_TIME_SIZE];
	int save_errno = errno;
	signal(SIGALRM, alrm_handler);

	alarm(0);
	
	if ( serv )
	{	
		if ( serv->sess_array )
			free(serv->sess_array);
		free(serv);
	}

	fprintf(stderr, "[%s] %s Unable to connect to database server. Connection timeout!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
	exit(1);

	errno = save_errno;
}

void stop_handler(int signo)
{
	int save_errno = errno;
	signal(SIGINT, stop_handler);
	
	sig_number = signo;
	exit_flag = 1;

	errno = save_errno;
}

int is_valid_auth_str(const char* user_auth_str, int authentication)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( user_auth_str == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"is_valid_auth_str\" param \"used_auth_str\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	char* valid_symbols = ( !authentication ) ? "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_" : "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_!?$#" ;
	
	int auth_str_len = strlen(user_auth_str);
	
	/*printf("auth_str_len = %d\n", auth_str_len);
	printf("valid_symbols = %s\n", valid_symbols);*/

	if ( !authentication )
	{
		if ( (auth_str_len < MIN_LOGIN_LENGTH) || (auth_str_len > MAX_LOGIN_LENGTH) )
			return 0;
	}
	else
	{
		if ( (auth_str_len < MIN_PASS_LENGTH) || (auth_str_len > MAX_PASS_LENGTH) )
			return 0;
	}
	
	if ( strcmp(user_auth_str, "undefined") == 0 )
		return 0;

	int correct_symbols = 0;
	int i = 0;
	int j = 0;

	/*printf("user_auth_str = %s\n", user_auth_str);*/

	while ( valid_symbols[j] )
	{
		if ( user_auth_str[i] == valid_symbols[j] )
		{
			correct_symbols++;
			i++;
			j = 0;
			continue;
		}
		j++;
	}

	/*printf("correctSymbols = %d\nauth_str_len = %d\n", correctSymbols, auth_str_len);*/

	if ( correct_symbols == auth_str_len )
		return 1;

	return 0;
}

void session_send_string(ClientSession *sess, const char *str)
{
	char current_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (str == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_send_string\" params \"sess\" or \"str\" is NULL\n", get_time_str(current_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	int mes_size = strlen(str)+1;
	int bytes_sent = write(sess->sockfd, str, mes_size);
	

	if ( serv->sess_array[sess->sockfd] )
		printf("[%s] %s Sent %d bytes to %s\n", get_time_str(current_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, serv->sess_array[sess->sockfd]->last_ip);
	else
		printf("[%s] %s Sent %d bytes to %d socket\n", get_time_str(current_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, sess->sockfd);

	view_data(str, bytes_sent, 'c', 50);
	view_data(str, bytes_sent, 'd', 50);
}

static ClientSession* make_new_session(int sockfd, struct sockaddr_in* from)
{
	char current_time[CURRENT_TIME_SIZE];

	if ( (sockfd < 0) || (from == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"make_new_session\" incorrect input arguments: \"sockfd\"=%d \"from\"=%p\n", get_time_str(current_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sockfd, from);
		return NULL;
	}

	ClientSession* sess = malloc(sizeof(ClientSession));
	if ( !sess )
	{
		fprintf(stderr, "[%s] %s In function \"make_new_session\" memory error(\"sess\")!\n", get_time_str(current_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	memset(sess, 0, sizeof(ClientSession));

	long ip = ntohl(from->sin_addr.s_addr);
	int port = ntohs(from->sin_port);
	char* buf_ip = concat_addr_port(ip, port);
	if ( !buf_ip )
	{
		fprintf(stderr, "[%s] %s function \"concat_addr_port\" didn't make correct address!\n", get_time_str(current_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	memcpy(sess->last_ip, buf_ip, strlen(buf_ip)+1);
	if ( buf_ip )
		free(buf_ip);

	sess->sockfd				=				sockfd;
	sess->ID					=					-1;
	sess->state					=			 fsm_start;
	sess->user_status			=	    status_offline;

	session_send_string(sess, "*CLIENT_HAS_ACCOUNT\n");

	return sess;
}

static void session_handler_has_account(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_has_accout\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( check_client_answer(client_line) )
	{
		session_send_string(sess, "*LOGIN_WAIT_LOGIN\n");
		sess->state = fsm_login_process_wait_login;
	}
	else
	{
		session_send_string(sess, "*SIGNUP_WAIT_LOGIN\n");
		sess->state = fsm_signup_wait_login;
	}
}

static void session_handler_login_wait_login(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_login_wait_login\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	StringList* list = clients_online;
	while ( list )
	{
		if ( strcmp(client_line, list->data) == 0 )
		{
			session_send_string(sess, "*LOGIN_ALREADY_AUTHORIZED\n");
			return;
		}
		list = list->next;
	}

	/*
	char response_to_db[BUFFER_SIZE];
	memset(response_to_db, 0, sizeof(response_to_db));
	*/

	/* отправить команду READ client_line на сервер БД */
	/* client_line - это логин клиента */
	

	int index = get_record_index_by_name(client_line, DB_USERINFO_NAME);
	if ( (index > -1) && (strcmp(client_line, "undefined") != 0) )
	{
		memcpy(sess->login, client_line, strlen(client_line)+1);
		session_send_string(sess, "*LOGIN_WAIT_PASS\n");
	 	sess->state = fsm_login_process_wait_pass;
		return;
	}
	
	session_send_string(sess, "*LOGIN_NOT_EXIST\n");
}

static void send_message_authorized(ClientSession* sess, const char* str)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (str == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"send_message_authorized\" params \"sess\" or \"str\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char message[100];

	int len = strlen(str);
	int pos = len;
	memcpy(message, str, len+1);
	
	len = strlen(sess->login);
	pos += len;
	strncat(message, sess->login, len);
	message[pos] = '\n';
	pos++;
	message[pos] = '\0';
	
	int i;
	for ( i = 0; i < serv->sess_array_size; i++ )
	{
		if ( ( serv->sess_array[i] ) )
		{
			if ( (i == sess->sockfd) || (!serv->sess_array[i]->authorized) )
				continue;

			write(i, message, strlen(message)+1);
		}
	}
}

static void success_new_authorized(ClientSession* sess)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"success_new_authorized\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char str[BUFSIZE];
	const char* code = "*SUCCESSFULLY_AUTHORIZED|";
	
	int len = strlen(code);
	int pos = len;
	memcpy(str, code, len+1);
	
	len = strlen(sess->login);
	pos += len;
	strncat(str, sess->login, len);
	str[pos] = '\n';
	pos++;
	str[pos] = '\0';
	
	session_send_string(sess, str);
	
	send_message_authorized(sess, "*USER_AUTHORIZED|");
}

static void session_handler_login_wait_pass(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_login_wait_pass\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	StringList* list = clients_online;

	while ( list )
	{
		if ( strcmp(sess->login, list->data) == 0 )
		{
			session_send_string(sess, "*LOGIN_ALREADY_AUTHORIZED\n");
			sess->state = fsm_login_process_wait_login;
			return;
		}
		list = list->next;
	}
	
	FILE* dbusers = NULL;
	if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
	{
		fprintf(stderr, "[%s] %s Unable to open database file \"%s\". Is it exist?\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME);
		session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");		
		sess->state = fsm_error;
		return;
	}
	
	/* READ sess->login */
	int index = get_record_index_by_name(sess->login, DB_USERINFO_NAME);
	if ( index < 0 )
	{
		fprintf(stderr, "[%s] %s Unable to find in database file \"%s\" user with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME, sess->login);
		session_send_string(sess, "*LOGIN_NOT_EXIST\n");
		sess->state = fsm_error;
		return;
	}

	DBUsersInformation*	record = malloc(sizeof(DBUsersInformation));
	fseek(dbusers, index * sizeof(DBUsersInformation), SEEK_SET);
	fread(record, sizeof(DBUsersInformation), 1, dbusers);
	fclose(dbusers);
	
	if ( strcmp(client_line, record->pass) == 0 )
	{
		sess->ID = record->ID;
		
		memcpy(sess->pass, client_line, strlen(client_line)+1);

		FILE* dbxusers = NULL;
		if (!(dbxusers = fopen(DB_XUSERINFO_NAME, "rb")) )
		{
			fprintf(stderr, "[%s] %s Unable to open database file \"%s\". Is it exist?\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_XUSERINFO_NAME);
			session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
			sess->state = fsm_error;
			return;
		}

		DBXUsersInformation* xrecord = malloc(sizeof(DBXUsersInformation));
		fseek(dbxusers, sess->ID * sizeof(DBXUsersInformation), SEEK_SET);
		fread(xrecord, sizeof(DBXUsersInformation), 1, dbxusers);
		fclose(dbxusers);
		
		char last_in[CURRENT_DATE_BUF_SIZE];
		get_date_str(last_in, CURRENT_DATE_BUF_SIZE);
		
		memcpy(sess->last_date_in, last_in, strlen(last_in)+1);
		memcpy(sess->registration_date, xrecord->registration_date, strlen(xrecord->registration_date)+1);
		
		set_user_rank(sess);

		sess->start_mute_time = xrecord->start_mute_time;
		sess->mute_time = xrecord->mute_time;
		sess->muted = xrecord->muted;
		
		free(xrecord);

		if ( sess->muted )
			eval_mute_time_left(sess);
	
		/* WRITE sess */
		update_usersinfo_records(sess);
		update_ext_usersinfo_records(sess);
		
		sess->authorized = 1;
		sess->user_status = status_online;
		sl_insert(&clients_online, sess->login);
		sess->state = fsm_wait_message;
		success_new_authorized(sess);
	}
		
	if ( record )
		free(record);

	if ( !(sess->authorized) )
		session_send_string(sess, "*PASS_NOT_MATCH\n");
}

static void session_handler_signup_wait_login(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_signup_wait_login\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( is_valid_auth_str(client_line, 0) )
	{
		int index = get_record_index_by_name(client_line, DB_USERINFO_NAME);

		if ( index > -1 )
		{
			session_send_string(sess, "*LOGIN_ALREADY_USED\n");
			return;
		}
		
		memcpy(sess->login, client_line, strlen(client_line)+1);
		session_send_string(sess, "*SIGNUP_WAIT_PASS\n");
		sess->state = fsm_signup_wait_pass;
		return;
	}

	session_send_string(sess, "*LOGIN_INCORRECT\n");
}

static void session_handler_signup_wait_pass(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_pass\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( is_valid_auth_str(client_line, 1) )
	{
		int records_size = 0;
		evaluate_size_databases(&records_size);
		
		FILE* dbusers = NULL;
		if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
		{
			fprintf(stderr, "[%s] %s Cannot open database file \"%s\". Is it exist?\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME);
			session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
			sess->state = fsm_error;
			return;
		}

		DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
		if ( !record )
		{
			if ( dbusers )
				fclose(dbusers);

			fprintf(stderr, "[%s] %s In function \"session_handler_wait_pass\" memory error with \"record\"\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			return;
		}

		int i;
		for ( i = 0 ; i < records_size; i++ )
		{
			memset(record, 0, sizeof(DBUsersInformation));
			fseek(dbusers, i * sizeof(DBUsersInformation), SEEK_SET);
			fread(record, sizeof(DBUsersInformation), 1, dbusers);
			
			if ( strcmp(record->username, "undefined") == 0 )
				break;
		}
		
		if ( record )
			free(record);

		if ( dbusers )
			fclose(dbusers);

		int index = i;
		if ( index == records_size ) 
		{
			fprintf(stderr, "[%s] %s Database file is full\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
			sess->state = fsm_error;
			return;
		}
		
		sess->ID = index;
		
		memcpy(sess->pass, client_line, strlen(client_line)+1);
	
		char last_in[CURRENT_DATE_BUF_SIZE];
		get_date_str(last_in, CURRENT_DATE_BUF_SIZE);
		
		memcpy(sess->last_date_in, last_in, strlen(last_in)+1);
		memcpy(sess->registration_date, last_in, strlen(last_in)+1);

		set_user_rank(sess);

		sess->muted = 0;
		sess->start_mute_time = 0;
		sess->mute_time_left = 0;
		sess->mute_time = 0;

		update_usersinfo_records(sess);
		update_ext_usersinfo_records(sess);

		sess->authorized = 1;
		sess->user_status = status_online;

		sl_insert(&clients_online, sess->login);
		
		sess->state = fsm_wait_message;
		success_new_authorized(sess);

		return;
	}

	session_send_string(sess, "*NEW_PASS_INCORRECT\n");
}

static void session_handler_wait_message(ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_message\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char** cmd_args;
	int args_num = 0;
	int cmd_num = 0;

	cmd_args = is_received_message_command(client_line, &cmd_num, &args_num);

	switch ( cmd_num )
	{
		case CMD_CODE_OVERLIMIT_LENGTH:
			command_overlimit_length_handler(sess);
			break;
		case CMD_CODE_UNKNOWN_COMMAND:
			session_send_string(sess, "*UNKNOWN_COMMAND\n");
			break;
		case CMD_CODE_TEXT_MESSAGE:
			eval_mute_time_left(sess);
			if ( !sess->muted  )
			{
				text_message_handler(sess, client_line, 0, NULL);
			}
			else
			{
				const char* victim_message = "*MUTE_COMMAND_YOU_MUTED|";
				char response_victim[100];
				
				int len = strlen(victim_message);
				int pos = len;
				memcpy(response_victim, victim_message, len+1);

				char mtl[10];
				itoa(sess->mute_time_left, mtl, 9);
				len = strlen(mtl);
				pos += len;
				strncat(response_victim, mtl, len);
				response_victim[pos] = '\n';
				pos++;
				response_victim[pos] = '\0';

				session_send_string(sess, response_victim);
			}
			break;
		case CMD_CODE_COMMAND_HELP:
			help_command_handler(sess, cmd_args, args_num);
			break;
		case CMD_CODE_COMMAND_WHOIH:
			whoih_command_handler(sess, cmd_args, args_num);
			break;
		case CMD_CODE_COMMAND_CHGPASS:
			chgpass_command_handler(sess, cmd_args, args_num);
			break;
		case CMD_CODE_COMMAND_OP:
			if ( sess->rank == ADMIN_RANK_VALUE )
				op_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[4]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_DEOP:
			if ( sess->rank == ADMIN_RANK_VALUE )
				deop_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[5]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_PM:
			eval_mute_time_left(sess);
			if ( !sess->muted )
				pm_command_handler(sess, cmd_args, args_num);
			else
			{
				const char* victim_message = "*MUTE_COMMAND_YOU_MUTED|";
				char response_victim[100];
				
				int len = strlen(victim_message);
				int pos = len;
				memcpy(response_victim, victim_message, len+1);

				char mtl[10];
				itoa(sess->mute_time_left, mtl, 9);
				len = strlen(mtl);
				pos += len;
				strncat(response_victim, mtl, len);
				response_victim[pos] = '\n';
				pos++;
				response_victim[pos] = '\0';

				session_send_string(sess, response_victim);
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[6]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
			}
			break;
		case CMD_CODE_COMMAND_STATUS:
			status_command_handler(sess, cmd_args, args_num);
			break;
		case CMD_CODE_COMMAND_RECORD:
			record_command_handler(sess, cmd_args, args_num);
			break;
		case CMD_CODE_COMMAND_MUTE:
			if ( sess->rank == ADMIN_RANK_VALUE )
				mute_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[9]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_UNMUTE:
			if ( sess->rank == ADMIN_RANK_VALUE )
				unmute_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[10]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_KICK:
			if ( sess->rank == ADMIN_RANK_VALUE )
				kick_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[11]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_TABLE:
			if ( sess->rank == ADMIN_RANK_VALUE )
				table_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[12]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_BAN:
			if ( sess->rank == ADMIN_RANK_VALUE )
				ban_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[13]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "*UNKNOWN_COMMAND\n");
			}
			break;
		case CMD_CODE_COMMAND_UNBAN:
			if ( sess->rank == ADMIN_RANK_VALUE )
				unban_command_handler(sess, cmd_args, args_num);
			else
			{
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"session_handler_wait_message\"[14]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
				session_send_string(sess, "UNKNOWN_COMMAND\n");
			}
	}
}

static void session_fsm_step(ClientSession* sess, char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_fsm_step\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	switch ( sess->state )
	{
		case fsm_client_has_account:
			session_handler_has_account(sess, client_line);
			free(client_line);
			break;
		case fsm_login_process_wait_login:
			session_handler_login_wait_login(sess, client_line);
			free(client_line);
			break;
		case fsm_login_process_wait_pass:
			session_handler_login_wait_pass(sess, client_line);
			free(client_line);
			break;
		case fsm_signup_wait_login:
			session_handler_signup_wait_login(sess, client_line);
			free(client_line);
			break;
		case fsm_signup_wait_pass:
			session_handler_signup_wait_pass(sess, client_line);
			free(client_line);
			break;
		case fsm_wait_message:
			session_handler_wait_message(sess, client_line);
			free(client_line);
			break;
		case fsm_finish:
		case fsm_error:
			free(client_line);
	}
}

static void session_check_lf(ClientSession* sess)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_message\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	int pos = -1;

	int i;
	for ( i = 0; i < sess->buf_used; i++ )
	{
		if ( sess->buf[i] == '\n' )
		{
			pos = i;
			break;
		}
	}

	if ( pos < 1 )
		return;

	char* client_line = malloc(pos+1);

	memcpy(client_line, sess->buf, pos);
	client_line[pos] = '\0';
	
	sess->buf_used -= (pos+1);
	memmove(sess->buf, sess->buf+pos+1, sess->buf_used);
	if ( client_line[pos-1] == '\r' )
		client_line[pos-1] = '\0';

	printf("( %s )\n", client_line);
	session_fsm_step(sess, client_line);
}

static int session_do_read(ClientSession* sess)
{
	char cur_time[CURRENT_TIME_SIZE];
	
	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_message\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		return 0;
	}

	int rc = read(sess->sockfd, sess->buf, BUFSIZE);
	printf("[%s] %s Received %d bytes from %s => ", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, rc, sess->last_ip);
	
	if ( rc < 1 )
	{
		if ( rc == 0 )
		{
			sess->state = fsm_finish;
			printf("%s\n", "()");
		}
		else
		{
			sess->state = fsm_error;
			printf("(errno value = %d\n)", errno);
		}

		return 0;
	}

	sess->buf_used += rc;
	
	if ( sess->buf_used >= BUFSIZE )
	{
		fprintf(stderr, "[%s] %s Client's(%s) line is too long!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sess->login);
		sess->state = fsm_error;
		
		return 0;
	}

	session_check_lf(sess);

	if ( sess->state == fsm_finish )
		return 0;

	return 1;
}

int server_init(int port)
{
	signal(SIGALRM, alrm_handler);

	struct sockaddr_in addr;
	char cur_time[CURRENT_TIME_SIZE];

	printf("[%s] %s Creating socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( listen_sock < 0 )
	{
		fprintf(stderr, "[%s] %s socket() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}
	int opt = 1;																											/* Предотвращение "залипания" TCP порта */
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	
	printf("[%s] %s Binding socket to local address..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(listen_sock, (struct sockaddr*) &addr, sizeof(addr)) < 0 )
	{
		fprintf(stderr, "[%s] %s Failed to bind() a port. May be it has already used?\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}


	printf("[%s] %s Listening..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	if ( listen(listen_sock, QUEUE_SOCK_LEN) < 0 )
	{
		fprintf(stderr, "[%s] %s listen() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}
	

	printf("[%s] %s Trying to connect to local database server..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	struct sockaddr_in db_addr;
	db_addr.sin_family = AF_INET;

	int ok;
	ok = inet_aton(SERVER_DB_ADDR, &db_addr.sin_addr);
	if ( !ok )
	{
		fprintf(stderr, "[%s] %s Incorrect server database address!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}
	
	int db_port = atoi(SERVER_DB_PORT);
	if ( db_port < 1024 )
	{
		fprintf(stderr, "[%s] %s Incorrect port number!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}
	db_addr.sin_port = htons(db_port);
	
	int srv_db_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( srv_db_sock == -1 )
	{
		fprintf(stderr, "[%s] %s socket() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}
	opt = 1;																											/* Предотвращение "залипания" TCP порта */
	setsockopt(srv_db_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if ( connect(srv_db_sock, (struct sockaddr*) &db_addr, sizeof(db_addr)) == -1 )
	{
		fprintf(stderr, "[%s] %s connect() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}
	printf("[%s] %s Connected.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	ConfigFields cfg;
	memset(&cfg, 0, sizeof(struct ConfigFields));

	if ( !read_configuration_file(&cfg) )
	{
		fprintf(stderr, "[%s] %s Unable to read configuration file \"%s\"!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
		return 0;
	}

	serv = malloc( sizeof(Server) );
	if ( !serv )
	{
		fprintf(stderr, "[%s] %s memory error in \"serv\" struct!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	serv->db_sock = srv_db_sock;
	serv->ls = listen_sock;
	serv->sess_array = malloc(cfg.records_num * sizeof(ClientSession*));
	if ( !serv->sess_array )
	{
		fprintf(stderr, "[%s] %s memory error in \"serv\" struct records!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if (serv)
			free(serv);

		return 0;
	}	
	serv->sess_array_size = cfg.records_num;
	
	int i;
	for (i = 0; i < serv->sess_array_size; i++)
		serv->sess_array[i] = NULL;


	printf("[%s] %s Sending initialization data to database server...\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	char send_buf[BUFFER_SIZE];
	const char* init_db_msg = "INIT_TABLES|";
	int pos = strlen(init_db_msg);
	memcpy(send_buf, init_db_msg, pos);

	strcat(send_buf, cfg.userinfo_filename);
	pos += strlen(cfg.userinfo_filename);
	send_buf[pos] = '|';
	pos++;
	send_buf[pos] = '\0';

	strcat(send_buf, cfg.usersessions_filename);
	pos += strlen(cfg.usersessions_filename);
	send_buf[pos] = '\n';
	send_buf[pos+1] = '\0';
	
	int data_size = pos+1;
	int wc = write(serv->db_sock, send_buf, data_size);
	
	//////////////////////////////////////////////////////////////////////////////////
	alarm(TIMER_VALUE);
	
	char read_buf[BUFFER_SIZE];
	int rc = read(serv->db_sock, read_buf, sizeof(read_buf));
	
	alarm( 0 );
	//////////////////////////////////////////////////////////////////////////////////

	if ( rc < 1 )
	{
		fprintf(stderr, "[%s] %s Database server close connection.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		
		if ( serv )
		{
			if ( serv->sess_array )
				free(serv->sess_array);
			free(serv);
		}

		return 0;
	}
	
	if ( rc < BUFFER_SIZE )
	{
		if ( read_buf[rc-1] == '\n' )
			read_buf[rc-1] = '\0';
	}
	else
	{
		read_buf[BUFFER_SIZE-1] = '\0';
	}
	
	if ( strcmp("DB_INITIALIZATION_SUCCESS", read_buf) != 0 )
	{
		fprintf(stderr, "[%s] %s Database server unable to initialize himself.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		
		if ( serv )
		{
			if ( serv->sess_array )
				free(serv->sess_array);
			free(serv);
		}

		return 0;
	}
	printf("[%s] %s Database server successfully initialized.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	

	printf("[%s] %s Waiting for connections..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 1;
}

static int server_accept_client(void)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	char cur_time[CURRENT_TIME_SIZE];

	int client_sock = accept(serv->ls, (struct sockaddr*) &addr, &len);
	if ( client_sock == -1 )
	{
		fprintf(stderr, "[%s] %s accept() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return -1;
	}

	/* printf("client_sock = %d\n", client_sock); */
	if ( client_sock >= serv->sess_array_size )
	{
		int newlen = serv->sess_array_size;
		while ( client_sock >= newlen )
			newlen += serv->sess_array_size;

		serv->sess_array = realloc(serv->sess_array, newlen * sizeof(ClientSession*));
		
		int records_num = newlen - serv->sess_array_size;
		init_userinfo_database(records_num, 1);
		init_ext_userinfo_database(records_num, 1);

		int i;
		for (i = serv->sess_array_size; i < newlen; i++)
			serv->sess_array[i] = NULL;
		serv->sess_array_size = newlen;
		
		/* Записываем новый размер базы данных в конфиг-файл */
		char num[10];
		itoa(newlen, num, 9);
		char param_string[CONFIG_STRING_SIZE] = "currentDbSize=";
		strncat(param_string, num, strlen(num)+1);
		

		FILE* cfgPtr = NULL;
		if ( !(cfgPtr = fopen(CONFIG_NAME, "r+")) )
		{
			fprintf(stderr, "[%s] %s Unable to open \"%s\" config file.Trying to create one for you.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
			if ( !(cfgPtr = fopen(CONFIG_NAME, "w")) )
			{
				fprintf(stderr, "[%s] %s You don't have permission to create file in this directory.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				return -1;
			}
			fprintf(cfgPtr, "%s\n", param_string);
		}
		else
		{
			rewind(cfgPtr);
			fprintf(cfgPtr, "%s\n", param_string);
		}
		fclose(cfgPtr);
	}

	serv->sess_array[client_sock] = make_new_session(client_sock, &addr);
	printf("[%s] %s New connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, serv->sess_array[client_sock]->last_ip);

	return client_sock;
}
void server_close_session(int sock_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sock_num < 0 )
	{
		fprintf(stderr, "[%s] %s In function \"server_close_session\" \"sock_num\" is less than 0\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( serv->sess_array[sock_num]->muted )
		eval_mute_time_left(serv->sess_array[sock_num]);

	update_ext_usersinfo_records(serv->sess_array[sock_num]);

	if ( clients_online )
		sl_remove(&clients_online, serv->sess_array[sock_num]->login);
	
	if ( serv->sess_array[sock_num]->authorized )
		send_message_authorized(serv->sess_array[sock_num], "*USER_LEFT_CHAT|");

	close(sock_num);
	printf("[%s] %s Lost connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, serv->sess_array[sock_num]->last_ip);

	free(serv->sess_array[sock_num]);
	serv->sess_array[sock_num] = NULL;
}

int server_running(void)
{
	signal(SIGINT, stop_handler);

	fd_set readfds;
	int res, max_d;
	char cur_time[CURRENT_TIME_SIZE];

	while ( 1 )
	{
		FD_ZERO(&readfds);
		FD_SET(serv->ls, &readfds);
		max_d = serv->ls;

		int i;
		for ( i = 0; i < serv->sess_array_size; i++ )
		{
			if ( serv->sess_array[i] )
			{
				FD_SET(i, &readfds);
				if ( i > max_d )
					max_d = i;
			}
		}

		res = select(max_d+1, &readfds, NULL, NULL, NULL);
		if ( res == -1 )
		{
			if ( errno == EINTR )
			{
				if ( sig_number == SIGINT )
				{
					for ( i = 0; i < serv->sess_array_size; i++ )
						if ( serv->sess_array[i] )
							server_close_session(i);
					
					if ( serv->sess_array )
						free(serv->sess_array);
					free(serv);

					sl_clear(&clients_online);

					if ( exit_flag )
						break;
				}

				fprintf(stderr, "[%s] %s Got some signal (#%d)\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, errno);
			}
			else
			{
				fprintf(stderr, "[%s] %s select() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
			}
			continue;
		}

		if ( FD_ISSET(serv->ls, &readfds) )
			server_accept_client();

		for ( i = 0; i < serv->sess_array_size; i++ )
			if ( serv->sess_array[i] && FD_ISSET(i, &readfds) )
				if ( !session_do_read(serv->sess_array[i]) )
					server_close_session(i);
	}

	printf("\n[%s] %s Closing socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 0;
}

#endif

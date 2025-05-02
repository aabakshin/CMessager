#ifndef SERVERCORE_C_SENTRY
#define SERVERCORE_C_SENTRY

#include "serverCore.h"
#include "DatabaseStructures.h"
#include "DateTime.h"
#include "Commons.h"
#include "serverCommands.h"


static int session_do_read(Server* serv_ptr, ClientSession* sess);
static void session_check_lf(Server* serv_ptr, ClientSession* sess);
static void session_fsm_step(Server* serv_ptr, ClientSession* sess, char* client_line);

static void session_handler_has_account(Server* serv_ptr, ClientSession* sess, const char* client_line);
static void session_handler_login_wait_login(Server* serv_ptr, ClientSession* sess, const char* client_line);
static void session_handler_login_wait_pass(Server* serv_ptr, ClientSession* sess, const char* client_line);
static void session_handler_signup_wait_login(Server* serv_ptr, ClientSession* sess, const char* client_line);
static void session_handler_signup_wait_pass(Server* serv_ptr, ClientSession* sess, const char* client_line);
static void session_handler_wait_message(Server* serv_ptr, ClientSession* sess, const char* client_line);

static int check_client_answer(const char* answer);
static ClientSession* make_new_session(int sockfd, struct sockaddr_in* from);
static void send_message_authorized(Server* serv_ptr, ClientSession* sess, const char* str);
static void success_new_authorized(Server* serv_ptr, ClientSession* sess);
static int server_accept_client(Server* serv_ptr);


Server* server_ptr = NULL;

static int exit_flag = 0;
static int sig_number = 0;

enum
{				TIMEOUT 							=			 3,
				TOKENS_NUM							=			16,
				DEFAULT_MAX_SESSIONS_COUNT			=			10
};


void stop_handler(int signo)
{
	int save_errno = errno;
	signal(SIGINT, stop_handler);

	sig_number = signo;
	exit_flag = 1;

	errno = save_errno;
}

void server_force_stop(Server* serv_ptr)
{
	char cur_time[CURRENT_TIME_SIZE];

	printf("[%s] %s Stopping server...\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	close(serv_ptr->db_sock);

	if ( serv_ptr )
	{
		if ( serv_ptr->sess_array )
		{
			for ( int i = 0; i < serv_ptr->sess_array_size; i++)
			{
				if ( serv_ptr->sess_array[i] )
				{
					close(serv_ptr->sess_array[i]->sockfd);
					free(serv_ptr->sess_array[i]);
					serv_ptr->sess_array[i] = NULL;
				}
			}
			free(serv_ptr->sess_array);
		}
		sl_clear(&serv_ptr->clients_online);
		close(serv_ptr->ls);
		free(serv_ptr);
	}
	else
	{
		fprintf(stderr, "[%s] %s An error has occured while force stopping server!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
	}

	printf("\n[%s] %s Closing socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	exit(1);
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
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	int mes_size = strlen(str)+1;
	int bytes_sent = write(sess->sockfd, str, mes_size);
	printf("[%s] %s Sent %d bytes to %s\n", get_time_str(current_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, sess->last_ip);

	view_data(str, bytes_sent, 'c', 50);
	view_data(str, bytes_sent, 'd', 50);
}

int request_to_db(Server* serv_ptr, char* response, int response_size, const char** query_strings)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (query_strings == NULL) || (*query_strings == NULL) || (response == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"write_query_into_db\" \"strings_to_query\" or \"*strings_to_query\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	char request_to_db[BUFFER_SIZE];
	int bytes_to_send = concat_request_strings(request_to_db, BUFFER_SIZE, query_strings);

	if ( serv_ptr->db_sock < 0 )
	{
		fprintf(stderr, "[%s] %s In function \"write_query_into_db\" database socket is less than 0!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int wc = write(serv_ptr->db_sock, request_to_db, bytes_to_send);
	printf("[%s] %s Sent %d\\%d bytes to %s:%s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, bytes_to_send, SERVER_DATABASE_ADDR, SERVER_DATABASE_PORT);


	/* гарантируем, что следующий вызов read не заблокирует процесс */
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(serv_ptr->db_sock, &readfds);
	int max_d = serv_ptr->db_sock;

	struct timeval tm;
	tm.tv_sec = TIMEOUT;
	tm.tv_usec = 0;

	int res = select(max_d + 1, &readfds, NULL, NULL, &tm);
	if ( res < 1 )
	{
		if ( res == -1 )
		{
			if ( errno == EINTR )
			{
				if ( sig_number == SIGINT )
					if ( exit_flag )
						server_force_stop(serv_ptr);
				fprintf(stderr, "[%s] %s In function \"read_query_from_db\" received strange signal (#%d)\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sig_number);
				return 0;
			}

			fprintf(stderr, "[%s] %s In function \"read_query_from_db\" select() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
			return 0;
		}

		fprintf(stderr, "[%s] %s Database server didn't answer too long!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	memset(response, 0, response_size);
	int rc = read(serv_ptr->db_sock, response, response_size);
	if ( rc < 1 )
	{
		fprintf(stderr, "[%s] %s In function \"write_query_into_db\" database server close connection.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	if ( rc < BUFFER_SIZE )
	{
		if ( response[rc-1] == '\n' )
			response[rc-1] = '\0';
	}
	else
	{
		response[BUFFER_SIZE-1] = '\0';
	}

	return 1;
}

int get_field_from_db(Server* serv_ptr, char* field, const char* search_key, int field_code)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (search_key == NULL) || (field == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"get_field_from_db\" \"client_line\" or \"user_pass\" is NULL!", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	if ( (field_code < ID) || (field_code > REGISTRATION_DATE) )
	{
		fprintf(stderr, "[%s] %s In function \"get_field_from_db\" \"field_code\" has incorrect value!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	char response[BUFFER_SIZE];
	const char* request[] =
	{
			"DB_READLINE",
			search_key,
			NULL
	};

	if ( !request_to_db(serv_ptr, response, BUFFER_SIZE, request) )
	{
		return 0;
	}

	if ( strcmp("DB_LINE_NOT_FOUND", response) == 0 )
	{
		fprintf(stderr, "[%s] %s In function \"get_field_from_db\": Record with key \"%s\" isn't found in database tables!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, search_key);
		return 0;
	}

	char* parsed_options[TOKENS_NUM] = { NULL };
	int i = 0;
	char* istr = strtok(response, "|");
	while ( istr )
	{
		parsed_options[i] = istr;
		i++;
		if ( i >= TOKENS_NUM )
			break;

		istr = strtok(NULL, "|");
	}

	strcpy(field, parsed_options[field_code]);

	return 1;
}

static int check_client_answer(const char* answer)
{
	const char* check_for[] =
	{
					"y",
					"Y",
					"ye",
					"yE",
					"Ye",
					"YE",
					"yes",
					"Yes",
					"yEs",
					"yeS",
					"YEs",
					"YeS",
					"yES",
					"YES",
					NULL
	};

	for ( int i = 0; check_for[i]; i++ )
		if ( strcmp(answer, check_for[i]) == 0 )
			return 1;

	return 0;
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

static void session_handler_has_account(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_has_accout\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		if ( sess != NULL )
			sess->state = fsm_error;

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

static void session_handler_login_wait_login(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) || (strcmp(client_line, "undefined") == 0) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_login_wait_login\":\n"
						"params \"sess\" or \"client_line\" is NULL OR\n"
						"\"client_line\" is \"undefined\"\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	StringList* list = serv_ptr->clients_online;
	while ( list )
	{
		if ( strcmp(client_line, list->data) == 0 )
		{
			session_send_string(sess, "*LOGIN_ALREADY_AUTHORIZED\n");
			return;
		}
		list = list->next;
	}

	char id_param[100];
	if ( !get_field_from_db(serv_ptr, id_param, client_line, ID) )
	{
		sess->state = fsm_error;
		return;
	}

	int index = atoi(id_param);
	if ( index > -1 )
	{
		memcpy(sess->login, client_line, strlen(client_line)+1);
		session_send_string(sess, "*LOGIN_WAIT_PASS\n");
		sess->state = fsm_login_process_wait_pass;
		return;
	}

	session_send_string(sess, "*LOGIN_NOT_EXIST\n");
}

static void send_message_authorized(Server* serv_ptr, ClientSession* sess, const char* str)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (str == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"send_message_authorized\" params \"sess\" or \"str\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	const char* strings[] =
	{
				str,
				sess->login,
				NULL
	};

	char message[100];
	concat_request_strings(message, 100, strings);

	int i;
	for ( i = 0; i < serv_ptr->sess_array_size; i++ )
	{
		if ( ( serv_ptr->sess_array[i] ) )
		{
			if ( (i == sess->sockfd) || (!serv_ptr->sess_array[i]->authorized) )
				continue;

			write(i, message, strlen(message)+1);
		}
	}
}

static void success_new_authorized(Server* serv_ptr, ClientSession* sess)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"success_new_authorized\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char str[BUFSIZE];
	const char* strings[] =
	{
			"*SUCCESSFULLY_AUTHORIZED",
			sess->login,
			NULL
	};
	concat_request_strings(str, BUFSIZE, strings);

	session_send_string(sess, str);

	send_message_authorized(serv_ptr, sess, "*USER_AUTHORIZED");
}

static void session_handler_login_wait_pass(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_login_wait_pass\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		sess->state = fsm_error;
		return;
	}

	StringList* list = serv_ptr->clients_online;
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

	char id_param[ID_SIZE];
	if ( !get_field_from_db(serv_ptr, id_param, sess->login, ID) )
	{
		sess->state = fsm_error;
		return;
	}

	int index = atoi(id_param);
	if ( index < 0 )
	{
		fprintf(stderr, "[%s] %s Unable to find in database table user with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sess->login);
		session_send_string(sess, "*LOGIN_NOT_EXIST\n");
		sess->state = fsm_error;
		return;
	}

	char pass[PASS_SIZE];
	if ( !get_field_from_db(serv_ptr, pass, sess->login, PASS) )
	{
		sess->state = fsm_error;
		return;
	}

	if ( strcmp(client_line, pass) == 0 )
	{
		sess->ID = index;

		memcpy(sess->pass, client_line, strlen(client_line)+1);

		get_date_str(sess->last_date_in, CURRENT_DATE_BUF_SIZE);

		if ( !get_field_from_db(serv_ptr, sess->registration_date, sess->login, REGISTRATION_DATE) )
		{
			sess->state = fsm_error;
			return;
		}

		char rank[RANK_SIZE];
		set_user_rank(sess);
		rank[0] = get_user_rank(sess->rank);
		rank[1] = '\0';

		if ( sess->muted )
			eval_mute_time_left(sess);

		char smt[START_MUTE_TIME_SIZE];
		if ( !get_field_from_db(serv_ptr, smt, sess->login, START_MUTE_TIME) )
		{
			sess->state = fsm_error;
			return;
		}
		sess->start_mute_time = atoi(smt);

		char mt[MUTE_TIME_SIZE];
		if ( !get_field_from_db(serv_ptr, smt, sess->login, MUTE_TIME) )
		{
			sess->state = fsm_error;
			return;
		}
		sess->mute_time = atoi(mt);

		char muted[MUTED_SIZE];
		if ( !get_field_from_db(serv_ptr, smt, sess->login, MUTED) )
		{
			sess->state = fsm_error;
			return;
		}
		sess->muted = atoi(muted);

		char mtl[MUTE_TIME_LEFT_SIZE];
		itoa(sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

		const char* query_strings[] =
		{
						"DB_WRITELINE|",
						sess->login,
						"undefined",
						rank,
						"undefined",
						"undefined",
						"undefined",
						"undefined",
						"undefined",
						"undefined",
						mtl,
						sess->last_ip,
						sess->last_date_in,
						"undefined",
						"undefined",
						NULL
		};

		char response[BUFFER_SIZE];
		if ( !request_to_db(serv_ptr, response, BUFFER_SIZE, query_strings) )
		{
			sess->state = fsm_error;
			return;
		}

		if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
		{
			fprintf(stderr, "[%s] %s In function \"session_handler_login_wait_pass\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			sess->state = fsm_error;
			return;
		}

		sess->authorized = 1;

		sess->user_status = status_online;
		sl_insert(&serv_ptr->clients_online, sess->login);

		sess->state = fsm_wait_message;

		success_new_authorized(serv_ptr, sess);

		return;
	}

	session_send_string(sess, "*PASS_NOT_MATCH\n");
}

static void session_handler_signup_wait_login(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_signup_wait_login\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( is_valid_auth_str(client_line, 0) )
	{
		char id_param[ID_SIZE];
		if ( !get_field_from_db(serv_ptr, id_param, client_line, ID) )
		{
			sess->state = fsm_error;
			return;
		}

		int index = atoi(id_param);
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

static void session_handler_signup_wait_pass(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_pass\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( is_valid_auth_str(client_line, 1) )
	{
		memcpy(sess->pass, client_line, strlen(client_line)+1);

		get_date_str(sess->last_date_in, CURRENT_DATE_BUF_SIZE);
		get_date_str(sess->registration_date, CURRENT_DATE_BUF_SIZE);

		set_user_rank(sess);
		char rank[RANK_SIZE];
		rank[0] = get_user_rank(sess->rank);
		rank[1] = '\0';

		sess->muted = 0;
		sess->start_mute_time = 0;
		sess->mute_time = 0;
		sess->mute_time_left = 0;

		char muted[MUTED_SIZE];
		itoa(sess->muted, muted, MUTED_SIZE-1);

		char smt[START_MUTE_TIME_SIZE];
		itoa(sess->start_mute_time, smt, START_MUTE_TIME_SIZE-1);

		char mt[MUTE_TIME_SIZE];
		itoa(sess->mute_time, mt, MUTE_TIME_SIZE-1);

		char mtl[MUTE_TIME_LEFT_SIZE];
		itoa(sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

		const char* query_strings[] =
		{
						"DB_WRITELINE|",
						sess->login,
						sess->pass,
						rank,
						"undefined",
						"undefined",
						"undefined",
						muted,
						smt,
						mt,
						mtl,
						sess->last_ip,
						sess->last_date_in,
						"undefined",
						sess->registration_date,
						NULL
		};

		char response[BUFFER_SIZE];
		if ( !request_to_db(serv_ptr, response, BUFFER_SIZE, query_strings) )
		{
			sess->state = fsm_error;
			return;
		}

		if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
		{
			fprintf(stderr, "[%s] %s In function \"session_handler_signup_wait_pass\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			sess->state = fsm_error;
			return;
		}

		sess->authorized = 1;

		sess->user_status = status_online;
		sl_insert(&serv_ptr->clients_online, sess->login);

		sess->state = fsm_wait_message;

		success_new_authorized(serv_ptr, sess);

		return;
	}

	session_send_string(sess, "*NEW_PASS_INCORRECT\n");
}

static void session_handler_wait_message(Server* serv_ptr, ClientSession* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_message\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char** cmd_args;
	int args_num = 0;
	int cmd_num = 0;

	cmd_args = is_received_message_command(client_line, &cmd_num, &args_num);

	switch ( cmd_num )
	{
		case CMD_CODE_INTERNAL_SERVER_ERROR:
			session_send_string(sess, "*INTERNAL_ERROR\n");
			break;
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
				char response_victim[100];
				char mtl[10];
				itoa(sess->mute_time_left, mtl, 9);
				const char* strings[] =
				{
							"*MUTE_COMMAND_YOU_MUTED",
							mtl,
							NULL
				};

				concat_request_strings(response_victim, 100, strings);
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
				char response_victim[100];
				char mtl[10];
				itoa(sess->mute_time_left, mtl, 9);
				const char* strings[] =
				{
							"*MUTE_COMMAND_YOU_MUTED",
							mtl,
							NULL
				};
				concat_request_strings(response_victim, 100, strings);
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

static void session_fsm_step(Server* serv_ptr, ClientSession* sess, char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (client_line == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"session_fsm_step\" params \"sess\" or \"client_line\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	switch ( sess->state )
	{
		case fsm_client_has_account:
			session_handler_has_account(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_login_process_wait_login:
			session_handler_login_wait_login(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_login_process_wait_pass:
			session_handler_login_wait_pass(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_signup_wait_login:
			session_handler_signup_wait_login(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_signup_wait_pass:
			session_handler_signup_wait_pass(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_wait_message:
			session_handler_wait_message(serv_ptr, sess, client_line);
			free(client_line);
			break;
		case fsm_finish:
		case fsm_error:
			free(client_line);
	}
}

static void session_check_lf(Server* serv_ptr, ClientSession* sess)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"session_handler_wait_message\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

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
	session_fsm_step(serv_ptr, sess, client_line);
}

static int session_do_read(Server* serv_ptr, ClientSession* sess)
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

	session_check_lf(serv_ptr, sess);

	if ( sess->state == fsm_finish )
		return 0;

	return 1;
}

int server_init(int port, Server* serv_ptr)
{
	char cur_time[CURRENT_TIME_SIZE];


	printf("[%s] %s Trying to connect to database server..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	struct sockaddr_in db_addr;
	db_addr.sin_family = AF_INET;

	if ( !inet_aton(SERVER_DATABASE_ADDR, &db_addr.sin_addr) )
	{
		fprintf(stderr, "[%s] %s Incorrect server database address!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int db_port = atoi(SERVER_DATABASE_PORT);
	if ( db_port <= 1024 )
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

	/* Предотвращение "залипания" TCP порта */
	int opt = 1;
	setsockopt(srv_db_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if ( connect(srv_db_sock, (struct sockaddr*) &db_addr, sizeof(db_addr)) == -1 )
	{
		fprintf(stderr, "[%s] %s connect() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}
	printf("[%s] %s Connected.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	serv_ptr->db_sock = srv_db_sock;



	printf("[%s] %s Creating socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( listen_sock < 0 )
	{
		fprintf(stderr, "[%s] %s socket() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return 0;
	}

	/* Предотвращение "залипания" TCP порта */
	opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	printf("[%s] %s Binding socket to local address..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
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

	serv_ptr->ls = listen_sock;
	serv_ptr->clients_online = NULL;



	serv_ptr->sess_array = malloc(DEFAULT_MAX_SESSIONS_COUNT * sizeof(ClientSession*));
	if ( !serv_ptr->sess_array )
	{
		fprintf(stderr, "[%s] %s memory error in \"serv\" struct records!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}
	serv_ptr->sess_array_size = DEFAULT_MAX_SESSIONS_COUNT;

	for ( int i = 0; i < serv_ptr->sess_array_size; i++ )
		serv_ptr->sess_array[i] = NULL;



	printf("[%s] %s Waiting for connections..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 1;
}

static int server_accept_client(Server* serv_ptr)
{
	char cur_time[CURRENT_TIME_SIZE];

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	memset(&addr, 0, len);
	int client_sock = accept(serv_ptr->ls, (struct sockaddr*) &addr, &len);
	if ( client_sock == -1 )
	{
		fprintf(stderr, "[%s] %s accept() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		server_force_stop(serv_ptr);
		return -1;
	}

	if ( client_sock >= serv_ptr->sess_array_size )
	{
		int newlen = serv_ptr->sess_array_size * 2;

		serv_ptr->sess_array = realloc(serv_ptr->sess_array, newlen * sizeof(ClientSession*));
		if ( !serv_ptr->sess_array )
		{
			fprintf(stderr, "[%s] %s memory error in \"serv_ptr\" struct records!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			server_force_stop(serv_ptr);
			return -1;
		}

		for ( int i = serv_ptr->sess_array_size; i < newlen; i++ )
			serv_ptr->sess_array[i] = NULL;
		serv_ptr->sess_array_size = newlen;
	}

	serv_ptr->sess_array[client_sock] = make_new_session(client_sock, &addr);
	printf("[%s] %s New connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, serv_ptr->sess_array[client_sock]->last_ip);

	return client_sock;
}

void server_close_session(int sock_num, Server* serv_ptr)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( sock_num < 0 )
	{
		fprintf(stderr, "[%s] %s In function \"server_close_session\" \"sock_num\" is less than 0\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	ClientSession* close_sess = serv_ptr->sess_array[sock_num];
	if ( !close_sess )
	{
		fprintf(stderr, "[%s] %s In function \"server_close_session\" \"close_sess\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( close_sess->muted )
		eval_mute_time_left(close_sess);

	char muted[MUTED_SIZE];
	itoa(close_sess->muted, muted, MUTED_SIZE-1);

	char smt[START_MUTE_TIME_SIZE];
	itoa(close_sess->start_mute_time, smt, START_MUTE_TIME_SIZE-1);

	char mt[MUTE_TIME_SIZE];
	itoa(close_sess->mute_time, mt, MUTE_TIME_SIZE-1);

	char mtl[MUTE_TIME_LEFT_SIZE];
	itoa(close_sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

	char last_date_out[CURRENT_DATE_BUF_SIZE];
	get_date_str(last_date_out, CURRENT_DATE_BUF_SIZE);

	const char* query_strings[] =
	{
					"DB_WRITELINE|",
					close_sess->login,
					"undefined",
					"undefined",
					"undefined",
					"undefined",
					"undefined",
					muted,
					smt,
					mt,
					mtl,
					"undefined",
					"undefined",
					last_date_out,
					"undefined",
					NULL
	};

	char response[BUFFER_SIZE];
	if ( !request_to_db(serv_ptr, response, BUFFER_SIZE, query_strings) )
	{
		return;
	}

	if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
	{
		fprintf(stderr, "[%s] %s In function \"write_query_into_db\" unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( serv_ptr->clients_online )
		sl_remove(&serv_ptr->clients_online, close_sess->login);

	if ( close_sess->authorized )
		send_message_authorized(serv_ptr, close_sess, "*USER_LEFT_CHAT|");

	close(sock_num);
	printf("[%s] %s Lost connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, close_sess->last_ip);

	free(close_sess);
	close_sess = NULL;
}

int server_running(Server* serv_ptr)
{
	server_ptr = serv_ptr;

	char cur_time[CURRENT_TIME_SIZE];

	signal(SIGINT, stop_handler);

	while ( 1 )
	{
		fd_set readfds;
		int res, max_d;

		FD_ZERO(&readfds);
		FD_SET(serv_ptr->ls, &readfds);
		max_d = serv_ptr->ls;

		int i;
		for ( i = 0; i < serv_ptr->sess_array_size; i++ )
		{
			if ( serv_ptr->sess_array[i] )
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
					if ( exit_flag )
						server_force_stop(serv_ptr);

				fprintf(stderr, "[%s] %s Got some signal (#%d)\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, errno);
			}
			else
			{
				fprintf(stderr, "[%s] %s select() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
			}
			continue;
		}

		if ( FD_ISSET(serv_ptr->ls, &readfds) )
			server_accept_client(serv_ptr);

		for ( i = 0; i < serv_ptr->sess_array_size; i++ )
			if ( serv_ptr->sess_array[i] && FD_ISSET(i, &readfds) )
				if ( !session_do_read(serv_ptr, serv_ptr->sess_array[i]) )
					server_close_session(i, serv_ptr);
	}

	printf("\n[%s] %s Closing socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 0;
}

#endif

#ifndef SERVERCOMMANDS_C_SENTRY
#define SERVERCOMMANDS_C_SENTRY


#include "../includes/DatabaseStructures.h"
#include "../includes/StringList.h"
#include "../includes/Commons.h"
#include "../includes/DateTime.h"
#include "../includes/serverCommands.h"
#include "../includes/serverCore.h"
#include "../includes/serverConfigs.h"


enum
{
	VALID_STATUSES_NUM		=			  4
};

const char* valid_commands[] =
{
			"",
			"/help",
			"/whoih",
			"/changepassword",
			"/op",
			"/deop",
			"/pm",
			"/status",
			"/record",
			"/mute",
			"/unmute",
			"/kick",
			"/table",
			"/ban",
			"/unban",
			NULL
};


static const char* get_status_str(enum status user_status);
static ResponseRecord* user_show_record(ClientSession* sess, const char* registered_username, int show_record_flag);
static ResponseDebugRecord* debug_show_record(ClientSession* sess, const char* registered_username, int show_record_flag);
static void send_record_response(ClientSession* sess, const char* username, const char* type);
static void draw_line(int line_length, int c);
static void send_mute_response(ClientSession* sess, const char* username);


extern Server* server_ptr;
extern const char* server_codes_list[SERVER_CODES_COUNT];
extern const char* subcommands_codes_list[SUBCOMMANDS_CODES_COUNT];

int clear_cmd_args(char** cmd_args, int args_num)
{
	if ( (cmd_args == NULL) || (*cmd_args == NULL) || (args_num < 1) )
		return 0;

	int i;
	int deleted = 0;

	for ( i = 0; i < args_num; i++ )
	{
		if ( cmd_args[i] )
		{
			free(cmd_args[i]);
			deleted++;
		}
	}
	free(cmd_args);

	if ( deleted == args_num )
		return 1;

	return 0;
}

void command_overlimit_length_handler(ClientSession *sess)
{
	if ( sess == NULL )
		return;

	char buf[100];
	char max_length_arg_str[10];
	itoa(CMD_ARGS_MAX_LENGTH, max_length_arg_str, 9);

	const char* strings[] =
	{
			server_codes_list[CMD_ARG_OVERLIMIT_LENGTH_CODE],
			max_length_arg_str,
			NULL
	};
	concat_request_strings(buf, 100, strings);
	session_send_string(sess, buf);
}

void help_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	const char* user_cmd_list[] =
	{
										"*HELP_COMMAND_SUCCESS",
										"/help - list all valid commands",
										"/whoih - list all online users",
										"/changepassword <new_password> - set new password for account",
										"/record [<\"username\"/realname/age/quote>] [<\"string\"/\"integer\"/\"string\">] - show user's personal card",
										"/pm <user> <message> - send private message to user",
										"/status [status_name/list] - Show your status or set status to your record/show list valid statuses",
										NULL
	};

	const char* admin_cmd_list[] =
	{
										"*HELP_COMMAND_SUCCESS",
										"/help - list all valid commands",
										"/whoih - list all online users",
										"/changepassword <new_password> - set new password for account",
										"/record [<\"username\"/debug/realname/age/quote>] [<\"string\"/\"string\"/\"integer\"/\"string\">] - show user's personal card",
										"/pm <user> <message> - send private message to user",
										"/status [status_name/list] - Show your status or set status to your record/show list valid statuses",
										"/table <list/\"db_name\"> - print table with specified name",
										"/mute <user> <duration_in_seconds> - turn off possibility for user to chat",
										"/unmute <user> - turn on possibility for user to chat",
										"/ban <username/ip> <\"name\"/\"ip\"> <temp/perm> [duration_in_seconds] - block the user using \"username\" or \"ip\" rule",
										"/unban <user> - unblock user",
										"/kick <user> - force disconnect user from chat",
										"/op <user> - move user in Admin's group",
										"/deop <user> - remove user from Admin's group",
										NULL
	};

	char cur_time[CURRENT_TIME_SIZE];
	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"help_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( args_num > 1 )
	{
		session_send_string(sess, server_codes_list[COMMAND_PARAMS_NO_NEED_CODE]);
		return;
	}

	char send_buf[BUFSIZE];
	if ( sess->rank != ADMIN_RANK_VALUE )
	{
		concat_request_strings(send_buf, BUFSIZE, user_cmd_list);
	}
	else
	{
		concat_request_strings(send_buf, BUFSIZE, admin_cmd_list);
	}
	session_send_string(sess, send_buf);
}

void whoih_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char cur_time[CURRENT_TIME_SIZE];
	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"whoih_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( args_num > 1 )
	{
		session_send_string(sess, server_codes_list[COMMAND_PARAMS_NO_NEED_CODE]);
		return;
	}

	int online_count = sl_get_size(server_ptr->clients_online);
	int strings_size = online_count+2;

	char* strings[strings_size];
	strings[0] = (char*) server_codes_list[WHOIH_COMMAND_SUCCESS_CODE];
	int j;
	for ( j = 1; j < strings_size; j++ )
		strings[j] = NULL;
	j = 1;

	StringList* clients_list = server_ptr->clients_online;
	while ( clients_list )
	{
		int i;
		for ( i = 0; i < server_ptr->sess_array_size; i++ )
			if ( server_ptr->sess_array[i] )
				if ( strcmp(clients_list->data, server_ptr->sess_array[i]->login) == 0 )
					break;

		if ( (server_ptr->sess_array[i]->user_status != status_invisible) || (sess->rank == ADMIN_RANK_VALUE) )
		{
			strings[j] = clients_list->data;
			j++;
		}

		clients_list = clients_list->next;
	}

	char send_buf[BUFSIZE];
	concat_request_strings(send_buf, BUFSIZE, (const char**)strings);
	session_send_string(sess, send_buf);
}

void chgpass_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char buffer_pass[100];
	char cur_time[CURRENT_TIME_SIZE];

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"CHGPWD",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"chgpass_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}

	int len = strlen(cmd_args[1]);
	memcpy(buffer_pass, cmd_args[1], len+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"chgpass_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( is_valid_auth_str(buffer_pass, 1) )
	{
		int len = strlen(buffer_pass);
		memcpy(sess->pass, buffer_pass, len+1);

		char rank[RANK_SIZE];
		set_user_rank(sess);
		rank[0] = get_user_rank(sess->rank);
		rank[1] = '\0';

		if ( sess->muted )
			eval_mute_time_left(sess);

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
						"DB_WRITELINE",
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
						"undefined",
						"undefined",
						"undefined",
						"undefined",
						NULL
		};

		char response[BUFFER_SIZE];
		if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
		{
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
		{
			fprintf(stderr, "[%s] %s In function \"chgpass_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		session_send_string(sess, server_codes_list[CHGPWD_COMMAND_SUCCESS_CODE]);
		return;
	}

	session_send_string(sess, server_codes_list[CHGPWD_COMMAND_INCORRECT_PASS_CODE]);
}

int eval_rank_num(const char* last_date_in, const char* registration_date)
{
	if ( (last_date_in == NULL) || (registration_date == NULL) )
	{
		return -1;
	}

	unsigned long long time_user_exist = get_date_num(last_date_in) - get_date_num(registration_date);

	if ( time_user_exist < 7*86400 )
		return FRESHMAN_RANK_VALUE;

	if ( (time_user_exist >= 7*86400) && (time_user_exist < 30*86400) )
		return MEMBER_RANK_VALUE;

	if ( (time_user_exist >= 30*86400) && (time_user_exist < 365*86400) )
		return WISDOM_RANK_VALUE;

	return OLDMAN_RANK_VALUE;
}

void set_user_rank(ClientSession *sess)
{
	if ( sess == NULL )
		return;

	int is_op = 0;
	int ops_count = 0;

	char** ops_strings;
	ops_strings = parse_ops_file(&ops_count);

	int j;
	for ( j = 0; j < ops_count; j++ )
	{
		if ( strcmp(sess->login, ops_strings[j]) == 0 )
		{
			sess->rank = ADMIN_RANK_VALUE;
			is_op = 1;
		}

		free(ops_strings[j]);
		ops_strings[j] = NULL;
	}
	free(ops_strings);
	ops_strings = NULL;

	if ( is_op )
	{
		return;
	}

	sess->rank = eval_rank_num(sess->last_date_in, sess->registration_date);
}

char get_user_rank(int rank)
{
	if ( (rank < FRESHMAN_RANK_VALUE) || (rank > ADMIN_RANK_VALUE) )
		return 'u';

	switch ( rank )
	{
		case FRESHMAN_RANK_VALUE:
			return 'F';
		case MEMBER_RANK_VALUE:
			return 'M';
		case WISDOM_RANK_VALUE:
			return 'W';
		case OLDMAN_RANK_VALUE:
			return 'O';
		case ADMIN_RANK_VALUE:
			return 'A';
	}
	return 'u';
}

void op_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"OP",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"op_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}

	char buffer_username[100];
	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"op_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	char id_param[ID_SIZE];
	if ( !get_field_from_db(server_ptr, id_param, buffer_username, ID) )
	{
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	int index = atoi(id_param);
	if ( index < 0 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"OP",
					subcommands_codes_list[USER_NOT_FOUND],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return;
	}

	int is_online = 0;
	StringList* cl_onl = server_ptr->clients_online;
	while ( cl_onl != NULL )
	{
		if ( strcmp(cl_onl->data, buffer_username) == 0 )
		{
			is_online = 1;
			break;
		}
		cl_onl = cl_onl->next;
	}

	if ( is_online )
	{
		int i;
		for ( i = 0; i < server_ptr->sess_array_size; i++ )
			if ( server_ptr->sess_array[i] )
				if ( strcmp(server_ptr->sess_array[i]->login, buffer_username) == 0 )
					break;

		if ( server_ptr->sess_array[i]->rank != ADMIN_RANK_VALUE )
		{
			server_ptr->sess_array[i]->rank = ADMIN_RANK_VALUE;

			char rank[RANK_SIZE];
			rank[0] = get_user_rank(server_ptr->sess_array[i]->rank);
			rank[1] = '\0';

			if ( server_ptr->sess_array[i]->muted )
				eval_mute_time_left(server_ptr->sess_array[i]);

			char smt[START_MUTE_TIME_SIZE];
			if ( !get_field_from_db(server_ptr, smt, server_ptr->sess_array[i]->login, START_MUTE_TIME) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->start_mute_time = atoi(smt);

			char mt[MUTE_TIME_SIZE];
			if ( !get_field_from_db(server_ptr, mt, server_ptr->sess_array[i]->login, MUTE_TIME) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->mute_time = atoi(mt);

			char muted[MUTED_SIZE];
			if ( !get_field_from_db(server_ptr, muted, server_ptr->sess_array[i]->login, MUTED) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->muted = atoi(muted);

			char mtl[MUTE_TIME_LEFT_SIZE];
			itoa(server_ptr->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

			const char* query_strings[] =
			{
							"DB_WRITELINE",
							server_ptr->sess_array[i]->login,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							muted,
							smt,
							mt,
							mtl,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};

			char response[BUFFER_SIZE];
			if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
			{
				fprintf(stderr, "[%s] %s In function \"op_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
		}
		else
		{
			session_send_string(sess, server_codes_list[OP_COMMAND_USER_ALREADY_ADMIN_CODE]);
			return;
		}
	}
	else
	{
		char rank[RANK_SIZE];
		if ( !get_field_from_db(server_ptr, rank, buffer_username, RANK) )
		{
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		if ( rank[0] != 'A' )
		{
			rank[0] = 'A';
			const char* query_strings[] =
			{
							"DB_WRITELINE",
							buffer_username,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};

			char response[BUFFER_SIZE];
			if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
			{
				fprintf(stderr, "[%s] %s In function \"chgpass_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
		}
		else
		{
			session_send_string(sess, server_codes_list[OP_COMMAND_USER_ALREADY_ADMIN_CODE]);
			return;
		}
	}


	char** ops_strings = NULL;
	int strings_count = 0;

	ops_strings = parse_ops_file(&strings_count);

	FILE* dbops = NULL;
	if ( !(dbops = fopen(OPS_NAME, "w+")) )
	{
		fprintf(stderr, "[%s] %s You don't have permission to rewrite \"%s\" file\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, OPS_NAME);
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[NO_PERM_TO_CREATE_FILE_CODE]);

		int i;
		for ( i = 0; i < strings_count; i++ )
		{
			if ( ops_strings[i] )
			{
				free( ops_strings[i] );
				ops_strings[i] = NULL;
			}
		}

		if ( strings_count > 0 )
			free(ops_strings);
		ops_strings = NULL;

		return;
	}

	if ( strings_count > 0 )
		ops_strings = realloc( ops_strings, sizeof(char*) * ( strings_count + 1 ) );
	else
		ops_strings = malloc( sizeof(char*) * ( strings_count + 1 ) );

	int user_len = strlen(buffer_username);
	ops_strings[strings_count] = malloc( sizeof(char) * user_len + 1 );
	memcpy(ops_strings[strings_count], buffer_username, user_len + 1);
	strings_count++;

	int i;
	for (i = 0; i < strings_count; i++)
	{
		fprintf(dbops, "%s\n", ops_strings[i]);
		if ( ops_strings[i] )
		{
			free(ops_strings[i]);
			ops_strings[i] = NULL;
		}
	}

	if ( dbops )
		fclose(dbops);

	if ( ops_strings )
		free(ops_strings);

	session_send_string(sess, server_codes_list[OP_COMMAND_SUCCESS_CODE]);
}

void deop_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"DEOP",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"deop_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}


	char buffer_username[100];
	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"deop_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	/* удаляем пользователя из списка админов в файле ops.txt и перезаписываем этот файл */
	char** ops_strings = NULL;
	int strings_count = 0;

	ops_strings = parse_ops_file(&strings_count);

	FILE* dbops = NULL;
	if ( !(dbops = fopen(OPS_NAME, "w+")) )
	{
		fprintf(stderr, "[%s] %s You don't have permission to rewrite \"%s\" file\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, OPS_NAME);
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[NO_PERM_TO_CREATE_FILE_CODE]);

		int i;
		for ( i = 0; i < strings_count; i++ )
			if ( ops_strings[i] )
				free(ops_strings[i]);

		if ( strings_count > 0 )
			free(ops_strings);

		return;
	}

	if ( strings_count < 1 )
	{
		if ( ops_strings )
			free(ops_strings);

		if ( dbops )
			fclose(dbops);

		session_send_string(sess, server_codes_list[DEOP_COMMAND_USER_ADREADY_USER_CODE]);
		return;
	}

	int i;
	for ( i = 0; i < strings_count; i++ )
		if ( strcmp(ops_strings[i], buffer_username) == 0 )
			break;

	if ( i >= strings_count )
	{
		int i;
		for ( i = 0; i < strings_count; i++ )
			if ( ops_strings[i] )
				free(ops_strings[i]);

		if ( ops_strings )
			free(ops_strings);

		if ( dbops )
			fclose(dbops);

		session_send_string(sess, server_codes_list[DEOP_COMMAND_USER_ADREADY_USER_CODE]);
		return;
	}

	int pos = i;

	int len = 0;
	for ( i = 0; ops_strings[strings_count-1][i]; i++ )
		len++;

	if ( strings_count > 1 )
	{
		ops_strings[pos] = realloc(ops_strings[pos], sizeof(char)*len+1);
		memcpy(ops_strings[pos], ops_strings[strings_count-1], len+1);

		free(ops_strings[strings_count-1]);
		strings_count--;

		for ( i = 0; i < strings_count; i++ )
		{
			fprintf(dbops, "%s\n", ops_strings[i]);
			free(ops_strings[i]);
		}
	}
	else
	{
		if ( ops_strings[0] )
			free(ops_strings[0]);
	}

	if ( ops_strings )
		free(ops_strings);

	if ( dbops )
		fclose(dbops);
	/* удаляем пользователя из списка админов в файле ops.txt и перезаписываем этот файл */


	char id_param[ID_SIZE];
	if ( !get_field_from_db(server_ptr, id_param, buffer_username, ID) )
	{
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	int index = atoi(id_param);
	if ( index < 0 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"DEOP",
					subcommands_codes_list[USER_NOT_FOUND],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return;
	}

	int is_online = 0;
	StringList* cl_onl = server_ptr->clients_online;
	while ( cl_onl != NULL )
	{
		if ( strcmp(cl_onl->data, buffer_username) == 0 )
		{
			is_online = 1;
			break;
		}
		cl_onl = cl_onl->next;
	}

	if ( is_online )
	{
		int i;
		for ( i = 0; i < server_ptr->sess_array_size; i++ )
			if ( server_ptr->sess_array[i] )
				if ( strcmp(server_ptr->sess_array[i]->login, buffer_username) == 0 )
					break;

		if ( server_ptr->sess_array[i]->rank != ADMIN_RANK_VALUE )
		{
			session_send_string(sess, server_codes_list[DEOP_COMMAND_USER_ADREADY_USER_CODE]);
			return;
		}
		else
		{
			set_user_rank(server_ptr->sess_array[i]);

			char rank[RANK_SIZE];
			rank[0] = get_user_rank(server_ptr->sess_array[i]->rank);
			rank[1] = '\0';

			if ( server_ptr->sess_array[i]->muted )
				eval_mute_time_left(server_ptr->sess_array[i]);

			char smt[START_MUTE_TIME_SIZE];
			if ( !get_field_from_db(server_ptr, smt, server_ptr->sess_array[i]->login, START_MUTE_TIME) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->start_mute_time = atoi(smt);

			char mt[MUTE_TIME_SIZE];
			if ( !get_field_from_db(server_ptr, mt, server_ptr->sess_array[i]->login, MUTE_TIME) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->mute_time = atoi(mt);

			char muted[MUTED_SIZE];
			if ( !get_field_from_db(server_ptr, muted, server_ptr->sess_array[i]->login, MUTED) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
			server_ptr->sess_array[i]->muted = atoi(muted);

			char mtl[MUTE_TIME_LEFT_SIZE];
			itoa(server_ptr->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

			const char* query_strings[] =
			{
							"DB_WRITELINE",
							server_ptr->sess_array[i]->login,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							muted,
							smt,
							mt,
							mtl,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};

			char response[BUFFER_SIZE];
			if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
			{
				fprintf(stderr, "[%s] %s In function \"chgpass_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
		}
	}
	else
	{
		char rank[RANK_SIZE];
		if ( !get_field_from_db(server_ptr, rank, buffer_username, RANK) )
		{
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		if ( rank[0] != 'A' )
		{
			session_send_string(sess, server_codes_list[DEOP_COMMAND_USER_ADREADY_USER_CODE]);
			return;
		}
		else
		{
			char ldi[LAST_DATE_IN_SIZE];
			if ( !get_field_from_db(server_ptr, ldi, buffer_username, LAST_DATE_IN) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			char rd[REG_DATE_SIZE];
			if ( !get_field_from_db(server_ptr, rd, buffer_username, REGISTRATION_DATE) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			int rank_num = eval_rank_num(ldi, rd);
			rank[0] = get_user_rank(rank_num);

			const char* query_strings[] =
			{
							"DB_WRITELINE",
							buffer_username,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};

			char response[BUFFER_SIZE];
			if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
			{
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}

			if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
			{
				fprintf(stderr, "[%s] %s In function \"chgpass_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				sess->state = fsm_error;
				session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
				return;
			}
		}
	}

	session_send_string(sess, server_codes_list[DEOP_COMMAND_SUCCESS_CODE]);
}

void pm_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	int user_offline = 1;
	char buffer_username[100] = { 0 };
	char buffer_message[BUFSIZE];
	char cur_time[CURRENT_TIME_SIZE];

	if ( args_num != 3 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"PM",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( args_num > 0 )
		{
			if ( !clear_cmd_args(cmd_args, args_num) )
			{
				fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"pm_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
				cmd_args = NULL;
			}
		}
		return;
	}

	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);
	memcpy(buffer_message, cmd_args[2], strlen(cmd_args[2])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"pm_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( strcmp(buffer_username, sess->login) == 0 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"PM",
					subcommands_codes_list[SELF_USE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return;
	}

	StringList* list = server_ptr->clients_online;
	while ( list )
	{
		if ( strcmp(list->data, buffer_username) == 0 )
		{
			user_offline = 0;
			text_message_handler(sess, buffer_message, 1, buffer_username);
			break;
		}
		list = list->next;
	}

	if ( user_offline )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"PM",
					subcommands_codes_list[USER_OFFLINE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);
	}
}

static const char* get_status_str(enum status user_status)
{
	switch ( user_status )
	{
		case status_offline:
			return "offline";
		case status_online:
			return "online";
		case status_busy:
			return "busy";
		case status_away:
			return "away";
		case status_work:
			return "work";
		case status_invisible:
			return "invisible";
	}
	return "undefined";
}

void status_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char cur_time[CURRENT_TIME_SIZE];

	if ( args_num > 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"STATUS",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}


	const char* cur_user_status = get_status_str(sess->user_status);
	int strings_size = VALID_STATUSES_NUM + 2;
	char* strings[strings_size];

	strings[0] = (char*) server_codes_list[STATUS_COMMAND_SUCCESS_CODE];
	int j;
	for ( j = 1; j < strings_size; j++ )
		strings[j] = NULL;
	j = 1;


	char buffer_message[BUFSIZE];
	if ( args_num == 2 )
	{
		char status_arg[100];
		memcpy(status_arg, cmd_args[1], strlen(cmd_args[1])+1);

		if ( strcmp(cur_user_status, status_arg) == 0 )
		{
			session_send_string(sess, server_codes_list[STATUS_COMMAND_ALREADY_SET_CODE]);
			return;
		}

		if ( strcmp(status_arg, "list") == 0 )
		{
			int i;
			for ( i = 1; i <= VALID_STATUSES_NUM; i++ )
			{
				const char* stat_ptr = get_status_str(i);
				strings[j] = (char*) stat_ptr;
				j++;
			}

			if ( sess->rank == ADMIN_RANK_VALUE )
			{
				const char* stat_ptr = get_status_str(SECRET_NUMBER);
				strings[j] = (char*) stat_ptr;
				j++;
			}

			concat_request_strings(buffer_message, BUFSIZE, (const char**) strings);
			session_send_string(sess, buffer_message);
		}
		else
		{
			int i;
			for ( i = 1; i <= VALID_STATUSES_NUM; i++ )
			{
				const char* stat_str = get_status_str(i);

				if ( strcmp(stat_str, status_arg) == 0 )
				{
					sess->user_status = i;
					session_send_string(sess, server_codes_list[STATUS_COMMAND_SUCCESS_CODE]);
					break;
				}
			}

			if ( i > VALID_STATUSES_NUM )
			{
				int flag = 0;
				if ( sess->rank == ADMIN_RANK_VALUE )
				{
					const char* stat_str = get_status_str(SECRET_NUMBER);

					if ( strcmp(stat_str, status_arg) == 0 )
					{
						sess->user_status = SECRET_NUMBER;
						session_send_string(sess, server_codes_list[STATUS_COMMAND_SUCCESS_CODE]);
						flag = 1;
					}
				}

				if ( !flag )
				{
					session_send_string(sess, server_codes_list[STATUS_COMMAND_INCORRECT_STATUS_CODE]);
				}
			}
		}

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}

	strings[j] = (char*) cur_user_status;
	j++;

	concat_request_strings(buffer_message, BUFSIZE, (const char**) strings);
	session_send_string(sess, buffer_message);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}
}

/* ResponseRecord - ответ сервера, т.е информация о клиенте в отформатированном виде */
static ResponseRecord* user_show_record(ClientSession* sess, const char* registered_username, int show_record_flag)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (registered_username == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" argument \"registered username\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return NULL;
	}

	/* возвращаемая в качестве результата запись */
	ResponseRecord* response_struct = malloc(sizeof(ResponseRecord));
	if ( !response_struct )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" memory error(\"response_struct\" is NULL)\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}
	memset(response_struct, 0, sizeof(ResponseRecord));


	/* Проверка, существует ли запись с таким именем в таблице БД */
	char response[BUFFER_SIZE];
	const char* request[] =
	{
				"DB_READLINE",
				registered_username,
				NULL
	};

	if ( !request_to_db(server_ptr, response, BUFFER_SIZE, request) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}

	if ( strcmp("DB_LINE_NOT_FOUND", response) == 0 )
	{
		if ( response_struct )
			free(response_struct);

		fprintf(stderr, "[%s] %s In function \"user_show_record\" unable to find record with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, registered_username);

		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"RECORD",
					subcommands_codes_list[USER_NOT_FOUND],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);
		return NULL;
	}


	/* извлечение поля RANK из таблицы */
	char rank[RANK_SIZE];
	if ( !get_field_from_db(server_ptr, rank, registered_username, RANK) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}

	switch ( rank[0] )
	{
		case 'u':
			strcpy(response_struct->rank, "undefined");
			break;
		case 'F':
			strcpy(response_struct->rank, "FRESHMAN");
			break;
		case 'M':
			strcpy(response_struct->rank, "MEMBER");
			break;
		case 'W':
			strcpy(response_struct->rank, "WISDOM");
			break;
		case 'O':
			strcpy(response_struct->rank, "OLD");
			break;
		case 'A':
			strcpy(response_struct->rank, "ADMIN");
	}


	/* извлечение поля AGE из таблицы */
	if ( !get_field_from_db(server_ptr, response_struct->age, registered_username, AGE) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;

	}


	/* извлечение поля REALNAME из таблицы */
	if ( !get_field_from_db(server_ptr, response_struct->realname, registered_username, REALNAME) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}


	/* извлечение поля QUOTE из таблицы */
	if ( !get_field_from_db(server_ptr, response_struct->quote, registered_username, QUOTE) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;

	}


	/* вычисление значения STATUS заданного клиента */
	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
		if ( server_ptr->sess_array[i] )
			if ( server_ptr->sess_array[i]->authorized )
				if ( (strcmp(registered_username, server_ptr->sess_array[i]->login) == 0) )
				{
					const char* status = get_status_str(server_ptr->sess_array[i]->user_status);
					strcpy(response_struct->status, status);
					break;
				}

	if ( i == server_ptr->sess_array_size )
	{
		const char* offline = "offline";
		strcpy(response_struct->status, offline);
	}


	/* извлечение поля REGISTRATION_DATE из таблицы */
	char registration_date[REG_DATE_SIZE];
	if ( !get_field_from_db(server_ptr, registration_date, registered_username, REGISTRATION_DATE) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}
	strcpy(response_struct->regdate, registration_date);


	/* запись USERNAME */
	strcpy(response_struct->username, registered_username);


	/* Отладочная печать в stdout содержимого получившейся записи перед отправкой клиенту */
	if ( show_record_flag )
	{
		char* args[USER_RECORD_FIELDS_NUM] =
		{
			response_struct->username,
			response_struct->status,
			response_struct->rank,
			response_struct->realname,
			response_struct->age,
			response_struct->regdate,
			response_struct->quote
		};

		print_record(args, USER_RECORD_FIELDS_NUM, 0);
	}

	return response_struct;
}

static ResponseDebugRecord* debug_show_record(ClientSession* sess, const char* registered_username, int show_record_flag)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (registered_username == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"debug_show_record\" argument \"registered username\" or \"sess\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return NULL;
	}


	ResponseDebugRecord* response_struct = malloc(sizeof(ResponseDebugRecord));
	if ( !response_struct )
	{
		fprintf(stderr, "[%s] %s In function \"debug_show_record\" memory error(\"response_struct\" is NULL)\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}
	memset(response_struct, 0, sizeof(ResponseDebugRecord));


	char response[BUFFER_SIZE];
	const char* request[] =
	{
				"DB_READLINE",
				registered_username,
				NULL
	};

	if ( !request_to_db(server_ptr, response, BUFFER_SIZE, request) )
	{
		if ( response_struct )
			free(response_struct);

		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}

	if ( strcmp("DB_LINE_NOT_FOUND", response) == 0 )
	{
		if ( response_struct )
			free(response_struct);

		fprintf(stderr, "[%s] %s In function \"debug_show_record\" unable to find record with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, registered_username);

		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"RECORD",
					subcommands_codes_list[USER_NOT_FOUND],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return NULL;
	}


	char rank[RANK_SIZE];
	if ( !get_field_from_db(server_ptr, rank, registered_username, RANK) )
	{
		if ( response_struct )
			free(response_struct);

		fprintf(stderr, "[%s] %s [1] In function \"debug_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return NULL;
	}

	switch ( rank[0] )
	{
		case 'u':
			strcpy(response_struct->rank, "undefined");
			break;
		case 'F':
			strcpy(response_struct->rank, "FRESHMAN");
			break;
		case 'M':
			strcpy(response_struct->rank, "MEMBER");
			break;
		case 'W':
			strcpy(response_struct->rank, "WISDOM");
			break;
		case 'O':
			strcpy(response_struct->rank, "OLD");
			break;
		case 'A':
			strcpy(response_struct->rank, "ADMIN");
	}


	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
		if ( server_ptr->sess_array[i] )
			if ( server_ptr->sess_array[i]->authorized )
				if ( (strcmp(registered_username, server_ptr->sess_array[i]->login) == 0) )
				{
					const char* status = get_status_str(server_ptr->sess_array[i]->user_status);
					strcpy(response_struct->status, status);

					itoa(server_ptr->sess_array[i]->ID, response_struct->id, ID_SIZE-1);
					itoa(server_ptr->sess_array[i]->authorized, response_struct->auth, AUTH_STR_SIZE-1);
					itoa(server_ptr->sess_array[i]->buf_used, response_struct->used, USED_STR_SIZE-1);

					strcpy(response_struct->last_date_in, server_ptr->sess_array[i]->last_date_in);
					strcpy(response_struct->last_ip, server_ptr->sess_array[i]->last_ip);
					strcpy(response_struct->regdate, server_ptr->sess_array[i]->registration_date);
					strcpy(response_struct->pass, server_ptr->sess_array[i]->pass);

					itoa(server_ptr->sess_array[i]->sockfd, response_struct->sock, SOCK_STR_SIZE-1);
					itoa(server_ptr->sess_array[i]->state, response_struct->state, STATE_STR_SIZE-1);

					eval_mute_time_left(server_ptr->sess_array[i]);

					itoa(server_ptr->sess_array[i]->muted, response_struct->muted, MUTED_SIZE-1);
					itoa(server_ptr->sess_array[i]->mute_time, response_struct->mute_time, MUTE_TIME_SIZE-1);
					itoa(server_ptr->sess_array[i]->mute_time_left, response_struct->mute_time_left, MUTE_TIME_LEFT_SIZE-1);
					itoa(server_ptr->sess_array[i]->start_mute_time, response_struct->start_mute_time, START_MUTE_TIME_SIZE-1);

					char rank[RANK_SIZE];
					set_user_rank(server_ptr->sess_array[i]);
					rank[0] = get_user_rank(server_ptr->sess_array[i]->rank);
					rank[1] = '\0';

					const char* query_strings[] =
					{
									"DB_WRITELINE",
									registered_username,
									"undefined",
									rank,
									"undefined",
									"undefined",
									"undefined",
									response_struct->muted,
									response_struct->start_mute_time,
									response_struct->mute_time,
									response_struct->mute_time_left,
									"undefined",
									"undefined",
									"undefined",
									"undefined",
									NULL
					};

					char response[BUFFER_SIZE];
					if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
					{
						sess->state = fsm_error;
						session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
						return NULL;
					}

					if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
					{
						fprintf(stderr, "[%s] %s In function \"debug_show_record\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
						sess->state = fsm_error;
						session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
						return NULL;
					}

					break;
				}

	if ( i == server_ptr->sess_array_size )
	{
		const char* offline = "offline";
		memcpy(response_struct->status, offline, strlen(offline)+1);

		if (
				!get_field_from_db(server_ptr, response_struct->id, registered_username, ID)							||
				!get_field_from_db(server_ptr, response_struct->last_date_in, registered_username, LAST_DATE_IN)		||
				!get_field_from_db(server_ptr, response_struct->last_ip, registered_username, LAST_IP)					||
				!get_field_from_db(server_ptr, response_struct->regdate, registered_username, REGISTRATION_DATE)		||
				!get_field_from_db(server_ptr, response_struct->pass, registered_username, PASS)						||
				!get_field_from_db(server_ptr, response_struct->muted, registered_username, MUTED)						||
				!get_field_from_db(server_ptr, response_struct->mute_time, registered_username, MUTE_TIME)				||
				!get_field_from_db(server_ptr, response_struct->mute_time_left, registered_username, MUTE_TIME_LEFT)	||
				!get_field_from_db(server_ptr, response_struct->start_mute_time, registered_username, START_MUTE_TIME)
		)
		{
			if ( response_struct )
				free(response_struct);

			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return NULL;
		}

		response_struct->auth[0] = '0';
		response_struct->auth[1] = '\0';
	}


	if ( show_record_flag )
	{
		char* args[DEBUG_RECORD_FIELDS_NUM] =
		{
			response_struct->username,
			response_struct->id,
			response_struct->auth,
			response_struct->used,
			response_struct->last_date_in,
			response_struct->last_ip,
			response_struct->regdate,
			response_struct->username,
			response_struct->pass,
			response_struct->rank,
			response_struct->sock,
			response_struct->state,
			response_struct->status,
			response_struct->muted,
			response_struct->mute_time,
			response_struct->mute_time_left,
			response_struct->start_mute_time
		};

		print_record(args, DEBUG_RECORD_FIELDS_NUM, 0);
	}

	return response_struct;
}

static void send_record_response(ClientSession* sess, const char* username, const char* type)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (username == NULL) || (type == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"send_record_response\" \"sess\", \"username\", or \"type\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}


	char success_string[BUFSIZE];
	int strings_size = DEBUG_RECORD_FIELDS_NUM + 2;
	char* strings[strings_size];

	strings[0] = (char*) server_codes_list[RECORD_COMMAND_SUCCESS_CODE];
	strings[1] = (char*) type;
	int j;
	for ( j = 2; j < strings_size; j++ )
		strings[j] = NULL;
	j = 2;


	if ( strcmp(type, "record") == 0 )
	{
		ResponseRecord* response = user_show_record(sess, username, 0);
		if ( !response )
		{
			sess->state = fsm_error;
			return;
		}

		const char* args[] =
		{
				"10",
				response->username,
				response->status,
				response->rank,
				response->realname,
				response->age,
				response->regdate,
				response->quote
		};

		int i;
		for ( i = 0; i < USER_RECORD_FIELDS_NUM+1; i++ )
		{
			strings[j] = (char*) args[i];
			j++;
		}
		free(response);
	}
	else if ( strcmp(type, "debug") == 0 )
	{
		ResponseDebugRecord* debug_response = debug_show_record(sess, username, 0);
		if ( !debug_response )
		{
			sess->state = fsm_error;
			return;
		}

		const char* args[] =
		{
				"20",
				debug_response->username,
				debug_response->id,
				debug_response->auth,
				debug_response->used,
				debug_response->last_date_in,
				debug_response->last_ip,
				debug_response->regdate,
				debug_response->username,
				debug_response->pass,
				debug_response->rank,
				debug_response->sock,
				debug_response->state,
				debug_response->status,
				debug_response->muted,
				debug_response->mute_time,
				debug_response->mute_time_left,
				debug_response->start_mute_time
		};

		int i;
		for ( i = 0; i < DEBUG_RECORD_FIELDS_NUM+1; i++ )
		{
			strings[j] = (char*) args[i];
			j++;
		}
		free(debug_response);
	}

	concat_request_strings(success_string, BUFSIZE, (const char**) strings);
	session_send_string(sess, success_string);
}

void record_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num > 3 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"RECORD",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"record_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}


	char buffer_param[100];
	char buffer_param_value[100];
	if ( (args_num == 2) || (args_num == 3) )
	{
		int len = strlen(cmd_args[1]);
		memcpy(buffer_param, cmd_args[1], len+1);

		if ( args_num == 3 )
		{
			len = strlen(cmd_args[2]);
			memcpy(buffer_param_value, cmd_args[2], len+1);
		}
	}

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"record_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	if ( args_num == 1 )
	{
		send_record_response(sess, sess->login, "record");
	}
	else if ( args_num == 2 )
	{
		char response[BUFFER_SIZE];
		const char* request[] =
		{
					"DB_READLINE",
					buffer_param,
					NULL
		};

		if ( !request_to_db(server_ptr, response, BUFFER_SIZE, request) )
		{
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		if ( strcmp("DB_LINE_NOT_FOUND", response) == 0 )
		{
			fprintf(stderr, "[%s] %s In function \"record_command_handler\" unable to find record with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, buffer_param);
			char send_buf[BUFFER_SIZE];
			const char* strings[] =
			{
						server_codes_list[COMMAND_INVALID_PARAMS_CODE],
						"RECORD",
						subcommands_codes_list[USER_NOT_FOUND],
						NULL
			};
			concat_request_strings(send_buf, BUFFER_SIZE, strings);
			session_send_string(sess, send_buf);
			return;
		}

		char rank[RANK_SIZE];
		if ( !get_field_from_db(server_ptr, rank, buffer_param, RANK))
		{
			sess->state = fsm_error;
			session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
			return;
		}

		if ( rank[0] == 'A' )
		{
			if ( sess->rank != ADMIN_RANK_VALUE )
			{
				session_send_string(sess, server_codes_list[COMMAND_NO_PERMS_CODE]);
				return;
			}
		}

		send_record_response(sess, buffer_param, "record");
	}
	else if ( args_num == 3 )
	{
		enum { REALNAME, AGE, QUOTE, DEBUG };
		const char* valid_params[] = { "realname", "age", "quote", "debug", NULL };
		int valid_param_flag = 0;

		/* отправитель команды /record хочет получить расширенную информацию о другом пользователе(возможно и о себе) */
		if ( (strcmp(buffer_param, valid_params[DEBUG]) == 0) )
		{
			if ( sess->rank != ADMIN_RANK_VALUE )
			{
				session_send_string(sess, server_codes_list[COMMAND_NO_PERMS_CODE]);
			}
			else
			{
				char response[BUFFER_SIZE];
				const char* request[] =
				{
								"DB_READLINE",
								buffer_param_value,
								NULL
				};

				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, request) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_NOT_FOUND", response) == 0 )
				{
					fprintf(stderr, "[%s] %s In function \"record_command_handler\" unable to find record with \"%s\" name!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, buffer_param_value);
					char send_buf[BUFFER_SIZE];
					const char* strings[] =
					{
								server_codes_list[COMMAND_INVALID_PARAMS_CODE],
								"RECORD",
								subcommands_codes_list[USER_NOT_FOUND],
								NULL
					};
					concat_request_strings(send_buf, BUFFER_SIZE, strings);
					session_send_string(sess, send_buf);
					return;
				}

				send_record_response(sess, buffer_param_value, "debug");
			}

			valid_param_flag = 1;
		}
		/* отправитель команды /record хочет изменить в своей записи поле REALNAME */
		else if ( strcmp(buffer_param, valid_params[REALNAME]) == 0 )
		{
			/* проверка корректности нового значения поля REALNAME */
			const char* valid_syms = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
			int is_correct_symbol;

			int k, j;
			for ( j = 0; buffer_param_value[j]; j++ )
			{
				is_correct_symbol = 0;
				for ( k = 0; valid_syms[k]; k++ )
				{
					if ( buffer_param_value[j] == valid_syms[k] )
					{
						is_correct_symbol = 1;
						break;
					}
				}

				if ( !is_correct_symbol )
					break;
			}

			/* обновляем данные по ключу sess->login в таблице БД, т.к пользователь изменил содержимое своей записи */
			if ( is_correct_symbol && (j < REALNAME_SIZE) )
			{
				valid_param_flag = 1;

				set_user_rank(sess);

				char rank[RANK_SIZE];
				rank[0] = get_user_rank(sess->rank);
				rank[1] = '\0';

				if ( sess->muted )
					eval_mute_time_left(sess);

				char smt[START_MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, smt, sess->login, START_MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->start_mute_time = atoi(smt);

				char mt[MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, mt, sess->login, MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->mute_time = atoi(mt);

				char muted[MUTED_SIZE];
				if ( !get_field_from_db(server_ptr, muted, sess->login, MUTED) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->muted = atoi(muted);

				char mtl[MUTE_TIME_LEFT_SIZE];
				itoa(sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

				const char* query_strings[] =
				{
								"DB_WRITELINE",
								sess->login,
								"undefined",
								rank,
								buffer_param_value,
								"undefined",
								"undefined",
								muted,
								smt,
								mt,
								mtl,
								"undefined",
								"undefined",
								"undefined",
								"undefined",
								NULL
				};

				char response[BUFFER_SIZE];
				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
				{
					fprintf(stderr, "[%s] %s In function \"record_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				/* Данные были успешно обновлены */
			}
		}
		/* отправитель команды /record хочет изменить в своей записи поле AGE */
		else if ( strcmp(buffer_param, valid_params[AGE]) == 0 )
		{
			/* проверка корректности нового значения поля AGE */
			int age = atoi(buffer_param_value);

			/* обновляем данные по ключу sess->login в таблице БД, т.к пользователь изменил содержимое своей записи */
			if ( (age >= MIN_AGE_VALUE) && (age <= MAX_AGE_VALUE) )
			{
				valid_param_flag = 1;

				set_user_rank(sess);

				char rank[RANK_SIZE];
				rank[0] = get_user_rank(sess->rank);
				rank[1] = '\0';

				if ( sess->muted )
					eval_mute_time_left(sess);

				char smt[START_MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, smt, sess->login, START_MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->start_mute_time = atoi(smt);

				char mt[MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, mt, sess->login, MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->mute_time = atoi(mt);

				char muted[MUTED_SIZE];
				if ( !get_field_from_db(server_ptr, muted, sess->login, MUTED) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->muted = atoi(muted);

				char mtl[MUTE_TIME_LEFT_SIZE];
				itoa(sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

				const char* query_strings[] =
				{
								"DB_WRITELINE",
								sess->login,
								"undefined",
								rank,
								"undefined",
								buffer_param_value,
								"undefined",
								muted,
								smt,
								mt,
								mtl,
								"undefined",
								"undefined",
								"undefined",
								"undefined",
								NULL
				};

				char response[BUFFER_SIZE];
				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
				{
					fprintf(stderr, "[%s] %s In function \"record_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				/* Данные были успешно обновлены */
			}
		}
		/* отправитель команды /record хочет изменить в своей записи поле QUOTE */
		else if ( strcmp(buffer_param, valid_params[QUOTE]) == 0 )
		{
			/* проверка корректности нового значения поля QUOTE */
			const char* valid_syms = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
			int is_correct_symbol;

			int k, j;
			for ( j = 0; buffer_param_value[j]; j++ )
			{
				is_correct_symbol = 0;
				for ( k = 0; valid_syms[k]; k++ )
				{
					if ( buffer_param_value[j] == valid_syms[k] )
					{
						is_correct_symbol = 1;
						break;
					}
				}

				if ( !is_correct_symbol )
					break;
			}

			/* обновляем данные по ключу sess->login в таблице БД, т.к пользователь изменил содержимое своей записи */
			if ( is_correct_symbol && (j < QUOTE_SIZE) )
			{
				valid_param_flag = 1;

				set_user_rank(sess);

				char rank[RANK_SIZE];
				rank[0] = get_user_rank(sess->rank);
				rank[1] = '\0';

				if ( sess->muted )
					eval_mute_time_left(sess);

				char smt[START_MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, smt, sess->login, START_MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->start_mute_time = atoi(smt);

				char mt[MUTE_TIME_SIZE];
				if ( !get_field_from_db(server_ptr, mt, sess->login, MUTE_TIME) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->mute_time = atoi(mt);

				char muted[MUTED_SIZE];
				if ( !get_field_from_db(server_ptr, muted, sess->login, MUTED) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				sess->muted = atoi(muted);

				char mtl[MUTE_TIME_LEFT_SIZE];
				itoa(sess->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

				const char* query_strings[] =
				{
								"DB_WRITELINE",
								sess->login,
								"undefined",
								rank,
								"undefined",
								"undefined",
								buffer_param_value,
								muted,
								smt,
								mt,
								mtl,
								"undefined",
								"undefined",
								"undefined",
								"undefined",
								NULL
				};

				char response[BUFFER_SIZE];
				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
				{
					fprintf(stderr, "[%s] %s In function \"record_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}
				/* Данные были успешно обновлены */
			}
		}

		if ( !valid_param_flag )
		{
			char send_buf[BUFFER_SIZE];
			const char* strings[] =
			{
						server_codes_list[COMMAND_INVALID_PARAMS_CODE],
						"RECORD",
						subcommands_codes_list[INCORRECT_STRING_VALUE],
						NULL
			};
			concat_request_strings(send_buf, BUFFER_SIZE, strings);
			session_send_string(sess, send_buf);
		}
	}
}

static void send_mute_response(ClientSession* sess, const char* username)
{
	if ( (sess == NULL) || (username == NULL) || (username[0] == '\0') )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}


	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
		if ( server_ptr->sess_array[i] )
			if ( strcmp(server_ptr->sess_array[i]->login, username) == 0 )
				break;
	int index = i;


	char response_to_victim[100];
	char mt[MUTE_TIME_LEFT_SIZE];
	itoa(server_ptr->sess_array[index]->mute_time_left, mt, MUTE_TIME_LEFT_SIZE-1);
	const char* victim_strings[] =
	{
					server_codes_list[MUTE_COMMAND_YOU_MUTED_CODE],
					mt,
					NULL
	};
	concat_request_strings(response_to_victim, 100, victim_strings);
	session_send_string(server_ptr->sess_array[index], response_to_victim);


	char response_sender[100];
	const char* sender_strings[] =
	{
					server_codes_list[MUTE_COMMAND_SUCCESS_CODE],
					username,
					mt,
					NULL
	};
	concat_request_strings(response_sender, 100, sender_strings);
	session_send_string(sess, response_sender);
}

void eval_mute_time_left(ClientSession* sess)
{
	time_t cur_time = time(0);
	int diff = cur_time - sess->start_mute_time;

	if ( diff >= sess->mute_time )
	{
		sess->muted = 0;
		sess->start_mute_time = 0;
		sess->mute_time_left = 0;
		sess->mute_time = 0;
	}
	else
	{
		sess->muted = 1;
		sess->mute_time_left = sess->mute_time-diff;
	}
}

void mute_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 3 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"MUTE",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);


		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}


	char username_buf[LOGIN_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1]) + 1);

	char time_val[START_MUTE_TIME_SIZE];
	memcpy(time_val, cmd_args[2], strlen(cmd_args[2]) + 1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	StringList* users_list = server_ptr->clients_online;
	int is_online = 0;
	while ( users_list )
	{
		if ( strcmp(username_buf, users_list->data) == 0 )
		{
			if ( strcmp(username_buf, sess->login) == 0 )
			{
				char send_buf[BUFFER_SIZE];
				const char* strings[] =
				{
							server_codes_list[COMMAND_INVALID_PARAMS_CODE],
							"MUTE",
							subcommands_codes_list[SELF_USE],
							NULL
				};
				concat_request_strings(send_buf, BUFFER_SIZE, strings);
				session_send_string(sess, send_buf);

				return;
			}
			else
			{
				is_online = 1;
				break;
			}
		}
		users_list = users_list->next;
	}

	if ( !is_online )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"MUTE",
					subcommands_codes_list[USER_OFFLINE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return;
	}

	char muted[MUTED_SIZE];
	if ( !get_field_from_db(server_ptr, muted, username_buf, MUTED) )
	{
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	int is_muted = atoi(muted);
	if ( is_muted )
	{
		session_send_string(sess, server_codes_list[MUTE_COMMAND_USER_ALREADY_MUTED_CODE]);
		return;
	}

	int mute_time = atoi(time_val);
	if ( (mute_time < MIN_MUTE_TIME_SEC) || (mute_time > MAX_MUTE_TIME_SEC) )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"MUTE",
					subcommands_codes_list[INCORRECT_TIME_RANGE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		return;
	}

	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++)
	{
		if ( server_ptr->sess_array[i] )
		{
			if ( strcmp(server_ptr->sess_array[i]->login, username_buf) == 0 )
			{
				server_ptr->sess_array[i]->muted = 1;
				server_ptr->sess_array[i]->mute_time = mute_time;
				server_ptr->sess_array[i]->mute_time_left = mute_time;
				time_t t = time(0);
				server_ptr->sess_array[i]->start_mute_time = t;

				char rank[RANK_SIZE];
				set_user_rank(server_ptr->sess_array[i]);
				rank[0] = get_user_rank(server_ptr->sess_array[i]->rank);
				rank[1] = '\0';

				itoa(server_ptr->sess_array[i]->muted, muted, MUTED_SIZE-1);

				char smt[START_MUTE_TIME_SIZE];
				itoa(server_ptr->sess_array[i]->start_mute_time, smt, START_MUTE_TIME_SIZE-1);

				char mt[MUTE_TIME_SIZE];
				itoa(server_ptr->sess_array[i]->mute_time, mt, MUTE_TIME_SIZE-1);

				char mtl[MUTE_TIME_LEFT_SIZE];
				itoa(server_ptr->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

				const char* query_strings[] =
				{
								"DB_WRITELINE",
								username_buf,
								"undefined",
								rank,
								"undefined",
								"undefined",
								"undefined",
								muted,
								smt,
								mt,
								mtl,
								"undefined",
								"undefined",
								"undefined",
								"undefined",
								NULL
				};

				char response[BUFFER_SIZE];
				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
				{
					fprintf(stderr, "[%s] %s In function \"mute_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				break;
			}
		}
	}

	send_mute_response(sess, username_buf);
}

void unmute_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"UNMUTE",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unmute_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}


	char username_buf[LOGIN_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unmute_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	if ( strcmp(sess->login, username_buf) == 0 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"UNMUTE",
					subcommands_codes_list[SELF_USE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);
		return;
	}

	char muted[MUTED_SIZE];
	if ( !get_field_from_db(server_ptr, muted, username_buf, MUTED) )
	{
		sess->state = fsm_error;
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	int is_muted = atoi(muted);
	if ( !is_muted )
	{
		session_send_string(sess, server_codes_list[UNMUTE_COMMAND_USER_NOT_MUTED_CODE]);
		return;
	}

	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
	{
		if ( server_ptr->sess_array[i] )
		{
			if ( strcmp(server_ptr->sess_array[i]->login, username_buf) == 0 )
			{
				server_ptr->sess_array[i]->muted = 0;
				server_ptr->sess_array[i]->mute_time = 0;
				server_ptr->sess_array[i]->mute_time_left = 0;
				server_ptr->sess_array[i]->start_mute_time = 0;

				char rank[RANK_SIZE];
				set_user_rank(server_ptr->sess_array[i]);
				rank[0] = get_user_rank(server_ptr->sess_array[i]->rank);
				rank[1] = '\0';

				itoa(server_ptr->sess_array[i]->muted, muted, MUTED_SIZE-1);

				char smt[START_MUTE_TIME_SIZE];
				itoa(server_ptr->sess_array[i]->start_mute_time, smt, START_MUTE_TIME_SIZE-1);

				char mt[MUTE_TIME_SIZE];
				itoa(server_ptr->sess_array[i]->mute_time, mt, MUTE_TIME_SIZE-1);

				char mtl[MUTE_TIME_LEFT_SIZE];
				itoa(server_ptr->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

				const char* query_strings[] =
				{
								"DB_WRITELINE",
								username_buf,
								"undefined",
								rank,
								"undefined",
								"undefined",
								"undefined",
								muted,
								smt,
								mt,
								mtl,
								"undefined",
								"undefined",
								"undefined",
								"undefined",
								NULL
				};

				char response[BUFFER_SIZE];
				if ( !request_to_db(server_ptr, response, BUFFER_SIZE, query_strings) )
				{
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				if ( strcmp("DB_LINE_WRITE_SUCCESS", response) != 0 )
				{
					fprintf(stderr, "[%s] %s In function \"unmute_command_handler\": unable to write a record into database tables.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
					sess->state = fsm_error;
					session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
					return;
				}

				session_send_string(server_ptr->sess_array[i], server_codes_list[UNMUTE_COMMAND_YOU_UNMUTED_CODE]);
				break;
			}
		}
	}

	char buffer[100];
	const char* strings[] =
	{
				server_codes_list[UNMUTE_COMMAND_SUCCESS_CODE],
				username_buf,
				NULL
	};
	concat_request_strings(buffer, 100, strings);
	session_send_string(sess, buffer);
}

void kick_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"KICK",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"kick_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}

	char username_buf[LOGIN_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"kick_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	StringList* users_list = server_ptr->clients_online;
	int is_online = 0;
	while ( users_list )
	{
		if ( strcmp(username_buf, users_list->data) == 0 )
		{
			if ( strcmp(username_buf, sess->login) == 0 )
			{
				char send_buf[BUFFER_SIZE];
				const char* strings[] =
				{
							server_codes_list[COMMAND_INVALID_PARAMS_CODE],
							"KICK",
							subcommands_codes_list[SELF_USE],
							NULL
				};
				concat_request_strings(send_buf, BUFFER_SIZE, strings);
				session_send_string(sess, send_buf);
				return;
			}
			else
			{
				is_online = 1;
				break;
			}
		}
		users_list = users_list->next;
	}

	if ( !is_online )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"KICK",
					subcommands_codes_list[USER_OFFLINE],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);
		return;
	}

	int i;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
	{
		if ( server_ptr->sess_array[i] )
		{
			if ( strcmp(server_ptr->sess_array[i]->login, username_buf) == 0 )
			{
				char send_buf[BUFFER_SIZE];
				const char* strings[] =
				{
							server_codes_list[KICK_COMMAND_SUCCESS_CODE],
							"VICTIM",
							NULL
				};
				concat_request_strings(send_buf, BUFFER_SIZE, strings);
				session_send_string(sess, send_buf);

				int sock = server_ptr->sess_array[i]->sockfd;
				server_close_session(sock, server_ptr);
			}
		}
	}

	char response[100];
	const char* strings[] =
	{
				server_codes_list[KICK_COMMAND_SUCCESS_CODE],
				"SENDER",
				username_buf,
				NULL
	};
	concat_request_strings(response, 100, strings);
	session_send_string(sess, response);
}

void table_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	if ( args_num != 2 )
	{
		char send_buf[BUFFER_SIZE];
		const char* strings[] =
		{
					server_codes_list[COMMAND_INVALID_PARAMS_CODE],
					"TABLE",
					subcommands_codes_list[TOO_MUCH_ARGS],
					NULL
		};
		concat_request_strings(send_buf, BUFFER_SIZE, strings);
		session_send_string(sess, send_buf);

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"table_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}


	char buffer_value[100];
	memcpy(buffer_value, cmd_args[1], strlen(cmd_args[1]) + 1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"table_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	char id[ID_SIZE];
	if ( !get_field_from_db(server_ptr, id, buffer_value, ID) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char username[LOGIN_SIZE];
	if ( !get_field_from_db(server_ptr, username, buffer_value, USERNAME) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char pass[PASS_SIZE];
	if ( !get_field_from_db(server_ptr, pass, buffer_value, PASS) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char rank[RANK_SIZE];
	if ( !get_field_from_db(server_ptr, rank, buffer_value, RANK) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char registration_date[REG_DATE_SIZE];
	if ( !get_field_from_db(server_ptr, registration_date, buffer_value, REGISTRATION_DATE) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char ldi[LAST_DATE_IN_SIZE];
	if ( !get_field_from_db(server_ptr, ldi, buffer_value, LAST_DATE_IN) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char ldo[LAST_DATE_IN_SIZE];
	if ( !get_field_from_db(server_ptr, ldo, buffer_value, LAST_DATE_OUT) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}

	char last_ip[LAST_IP_SIZE];
	if ( !get_field_from_db(server_ptr, last_ip, buffer_value, LAST_IP) )
	{
		session_send_string(sess, server_codes_list[INTERNAL_ERROR_CODE]);
		return;
	}


	char response_buffer[BUFSIZE];
	const char* args[] =
	{
			server_codes_list[TABLE_COMMAND_SUCCESS_CODE],
			"4",
			id,
			username,
			pass,
			rank,
			"4",
			registration_date,
			ldi,
			ldo,
			last_ip,
			NULL
	};
	concat_request_strings(response_buffer, BUFSIZE, args);
	session_send_string(sess, response_buffer);
}

void ban_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char cur_time[CURRENT_TIME_SIZE];

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"ban_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	return;
}

void unban_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
	{
		if ( sess != NULL )
			sess->state = fsm_error;

		return;
	}

	char cur_time[CURRENT_TIME_SIZE];

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unban_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CURRENT_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	return;
}

static void draw_line(int line_length, int c)
{
	const int offset = 18;

	if ( c == 0 )
		printf("%c", '|');
	else
		printf("%s", "\n|");

	int i;
	for (i = 1; i <= (offset + 3*line_length + line_length-1); i++ )
		printf("%c", '-');

	if ( c == 0 )
		printf("%c", '|');
	else
		printf("%s", "|\n");
}

void view_data(const char* str, int str_size, char mode, int line_length)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( line_length >= str_size )
		line_length = str_size-1;

	draw_line(line_length, 0);

	int k;
	for ( k = 0; k < str_size; k++ )
	{
		if ( ( k % line_length ) == 0 )
			printf("\n[%s] %s ", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

		if ( mode == 'c' )
		{
			if ( str[k] == '\n' )
				continue;

			printf("%3c ", str[k]);
		}
		else if ( mode == 'd' )
		{
			printf("%3d ", str[k]);
		}
		else
		{
			printf("%s", "Incorrect \"mode\" value\n");
			return;
		}
	}

	draw_line(line_length, 1);
}

void text_message_handler(ClientSession *sess, const char *msg, int is_private, const char *adresat)
{
	char str[BUFSIZE];
	char buf[BUFSIZE];
	char cur_time[CURRENT_TIME_SIZE];

	int i;
	for ( i = 0; msg[i]; i++ )
	{
		if ( msg[i] == '|' )
		{
			buf[i] = ' ';
			continue;
		}
		buf[i] = msg[i];
	}
	buf[i] = '\0';
	int buf_len = strlen(buf);

	int cur_pos;
	if ( sess->rank == ADMIN_RANK_VALUE )
	{
		const char* prefix = "[ADMIN]";
		for ( i = 0; prefix[i]; i++ )
			str[i] = prefix[i];
	}
	else
	{
		i = 0;
		str[i] = ' ';
		i++;
	}
	str[i] = '|';
	i++;
	cur_pos = i;

	if ( is_private )
	{
		str[cur_pos] = '~';
		cur_pos++;
	}

	int j;
	for ( j = 0; sess->login[j]; cur_pos++, j++ )
		str[cur_pos] = sess->login[j];
	str[cur_pos] = '|';
	cur_pos++;


	for ( i = 0; i < SERVER_CODES_COUNT; i++ )
	{
		char* sub_str = strstr(buf, server_codes_list[i]);
		if ( sub_str )
		{
			int sub_str_len = strlen(sub_str);
			int idx = buf_len - sub_str_len;
			int code_len = strlen(server_codes_list[i]);

			int k;
			for ( k = idx; k < (idx+code_len); k++ )
				buf[k] = ' ';
			i--;
		}
	}

	char* buffer_str = malloc(sizeof(char) * BUFSIZE);
	if ( !buffer_str )
	{
		sess->state = fsm_error;
		return;
	}

	int k;
	for ( k = 0, j = 0; buf[j]; j++, k++ )
	{
		if ( buf[j] == ' ' )
		{
			buffer_str[k] = ' ';
			for ( ; buf[j] == ' '; j++ )
				{ }

			if ( buf[j] != ' ' )
				j--;
		}
		else
			buffer_str[k] = buf[j];
	}
	buffer_str[k] = '\0';


	for ( j = 0; buffer_str[j]; j++, cur_pos++ )
		str[cur_pos] = buffer_str[j];
	str[cur_pos] = '\n';
	cur_pos++;
	str[cur_pos] = '\0';
	cur_pos++;

	free(buffer_str);

	int mes_size = strlen(str)+1;
	for ( i = 0; i < server_ptr->sess_array_size; i++ )
	{
		if ( server_ptr->sess_array[i] )
		{
			if ( (i == sess->sockfd) || (!server_ptr->sess_array[i]->authorized) )
				continue;

			if ( is_private )
			{
				if ( strcmp(server_ptr->sess_array[i]->login, adresat) != 0 )
					continue;

				int bytes_sent = write(i, str, mes_size);
				printf("[%s] %s Sent %d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, server_ptr->sess_array[i]->last_ip);
				view_data(str, bytes_sent, 'c', 50);
				view_data(str, bytes_sent, 'd', 50);
				break;
			}

			int bytes_sent = write(i, str, mes_size);
			printf("[%s] %s Sent %d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, server_ptr->sess_array[i]->last_ip);
			view_data(str, bytes_sent, 'c', 50);
			view_data(str, bytes_sent, 'd', 50);
		}
	}
}

char** is_received_message_command(const char* msg, int* cmd_num, int* args_num)
{
	char buf[BUFSIZE];
	char aux_buf[BUFSIZE];
	char** cmd_args;
	*args_num = 0;

	memcpy(buf, msg, strlen(msg)+1);
	memcpy(aux_buf, buf, strlen(buf)+1);

	if ( buf[0] == '/' )
	{
		char* istr = strtok(aux_buf, " ");
		while ( istr )
		{
			(*args_num)++;
			istr = strtok(NULL, " ");
		}

		cmd_args = malloc(sizeof(char*) * (*args_num));
		if ( !cmd_args )
		{
			*cmd_num = CMD_CODE_INTERNAL_SERVER_ERROR;
			*args_num = 0;
			return NULL;
		}

		int k = 0;
		int i;
		istr = strtok(buf, " ");
		while ( istr )
		{
			int arg_length = 0;
			for ( i = 0; istr[i]; i++ )
				arg_length++;

			if ( arg_length > CMD_ARGS_MAX_LENGTH )
			{
				free(cmd_args);
				*cmd_num = CMD_CODE_OVERLIMIT_LENGTH;
				*args_num = 0;
				return NULL;
			}

			cmd_args[k] = malloc(sizeof(char) * (arg_length + 1));
			if ( !cmd_args[k] )
			{
				for ( int i = 0; i < k; i++ )
				{
					if ( cmd_args[i] )
						free(cmd_args[i]);
					cmd_args[i] = NULL;
				}

				free(cmd_args);

				*cmd_num = CMD_CODE_INTERNAL_SERVER_ERROR;
				*args_num = 0;
				return NULL;
			}

			memcpy(cmd_args[k], istr, arg_length+1);

			k++;
			istr = strtok(NULL, " ");
		}

		if ( strcmp(valid_commands[CMD_CODE_COMMAND_HELP], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_HELP;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_WHOIH], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_WHOIH;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_CHGPASS], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_CHGPASS;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_OP], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_OP;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_DEOP], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_DEOP;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_PM], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_PM;

			if ( *args_num > 3 )
			{
				char buffer_command[100];
				char buffer_name[100];
				char buffer_message[BUFSIZE];

				const char* args[] =
				{
						buffer_command,
						buffer_name,
						buffer_message
				};


				memcpy(buffer_command, cmd_args[0], strlen(cmd_args[0])+1);
				memcpy(buffer_name, cmd_args[1], strlen(cmd_args[1])+1);

				int k = 0;
				int j;
				int i;
				for ( i = 2; i < *args_num; i++ )
				{
					for ( j = 0; cmd_args[i][j]; j++ )
					{
						buffer_message[k] = cmd_args[i][j];
						k++;
					}
					buffer_message[k] = ' ';
					k++;
				}
				buffer_message[k-1] = '\0';


				for ( i = 0; i < *args_num; i++ )
					free(cmd_args[i]);
				free(cmd_args);


				*args_num = 3;
				cmd_args = malloc(sizeof(char*) * (*args_num));
				if ( !cmd_args )
				{
					*cmd_num = CMD_CODE_INTERNAL_SERVER_ERROR;
					*args_num = 0;
					return NULL;
				}

				for ( i = 0; i < *args_num; i++ )
				{
					int str_len = strlen(args[i]);
					cmd_args[i] = malloc(sizeof(char) * str_len + 1);
					if ( !cmd_args[i] )
					{
						for ( int k = 0; k < i; k++ )
						{
							if ( cmd_args[k] )
								free(cmd_args[k]);
							cmd_args[k] = NULL;
						}

						free(cmd_args);

						*cmd_num = CMD_CODE_INTERNAL_SERVER_ERROR;
						*args_num = 0;
						return NULL;
					}

					memcpy(cmd_args[i], args[i], str_len + 1);
				}
			}
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_STATUS], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_STATUS;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_RECORD], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_RECORD;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_MUTE], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_MUTE;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_UNMUTE], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_UNMUTE;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_KICK], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_KICK;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_TABLE], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_TABLE;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_BAN], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_BAN;
			return cmd_args;
		}
		if ( strcmp(valid_commands[CMD_CODE_COMMAND_UNBAN], cmd_args[0]) == 0 )
		{
			*cmd_num = CMD_CODE_COMMAND_UNBAN;
			return cmd_args;
		}

		for ( i = 0; i < *args_num; i++ )
			free(cmd_args[i]);
		free(cmd_args);

		*cmd_num = CMD_CODE_UNKNOWN_COMMAND;
		*args_num = 0;

		return NULL;
	}

	*cmd_num = CMD_CODE_TEXT_MESSAGE;
	*args_num = 0;

	return NULL;
}


#endif

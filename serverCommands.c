#ifndef SERVERCOMMANDS_C_SENTRY
#define SERVERCOMMANDS_C_SENTRY

#include "DatabaseStructures.h"
#include "StringList.h"
#include "Commons.h"
#include "DateTime.h"
#include "serverCommands.h"
#include "serverCore.h"
#include "serverConfigs.h"

enum
{
	CUR_TIME_SIZE			=			100,
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


static void make_send_buf(char* send_buf, int cur_pos, const char* cmd_list[]);
static const char* get_status_str(enum status user_status);
static ResponseRecord* user_show_record(ClientSession* sess, const char* registered_username, int show_record_flag);
static ResponseDebugRecord* debug_show_record(ClientSession* sess, const char* registered_username, int show_record_flag);
static void send_record_response(ClientSession* sess, const char* username, const char* type);
static void draw_line(int line_length, int c);
static void send_mute_response(ClientSession* sess, const char* username);


extern StringList* clients_online;
extern Server* serv;
extern const char* server_codes_list[SERVER_CODES_COUNT];


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

static void make_send_buf(char* send_buf, int cur_pos, const char* cmd_list[])
{
	if ( (send_buf == NULL) || (cur_pos < 0) || (cmd_list == NULL) )
		return;

	int i = 0;
	while ( cmd_list[i] )
	{
		int j;
		for ( j = 0; cmd_list[i][j]; j++, cur_pos++ )
			send_buf[cur_pos] = cmd_list[i][j];

		i++;
		if ( cmd_list[i] )
		{
			send_buf[cur_pos] = '|';
			cur_pos++;
		}
		else
		{
			send_buf[cur_pos] = '\n';
			cur_pos++;
			send_buf[cur_pos] = '\0';
		}
	}
}

void command_overlimit_length_handler(ClientSession *sess)
{
	if ( sess == NULL )
		return;

	char buf[100] = "*CMD_ARG_OVERLIMIT_LENGTH|";
	char max_length_arg_str[10];
	itoa(CMD_ARGS_MAX_LENGTH, max_length_arg_str, 9);

	int len = strlen(max_length_arg_str);
	int pos = len;
	strncat(buf, max_length_arg_str, len);
	buf[pos] = '\n';
	pos++;
	buf[pos] = '\0';
	
	session_send_string(sess, buf);
}

void help_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;

	char cur_time[CUR_TIME_SIZE];
	char send_buf[BUFSIZE] = { 0 };

	const char* user_cmd_list[] = {
										"/help - list all valid commands",
										"/whoih - list all online users",
										"/changepassword <new_password> - set new password for account",
										"/record [<\"username\"/realname/age/quote>] [<\"string\"/\"string\"/\"integer\"/\"string\">] - show user's personal card",
										"/pm <user> <message> - send private message to user",
										"/status [status_name/list] - Show your status or set status to your record/show list valid statuses",
										NULL
								  };

	const char* admin_cmd_list[] = {
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

	/*const char* server_cmd_list[] = {
										"/help - list all valid commands",
										"/whoih - list all online users",
										"/record <debug> <\"username\"> - show user's personal card",
										"/table <list/\"db_name\"> - print table with specified name",
										"/mute <user> <duration_in_seconds> - turn off possibility for user to chat",
										"/unmute <user> - turn on possibility for user to chat",
										"/ban <username/ip> <\"name\"/\"ip\"> <temp/perm> [duration_in_seconds] - block the user using \"username\" or \"ip\" rule",
										"/unban <user> - unblock user",
										"/kick <user> - force disconnect user from chat",
										"/op <user> - move user in Admin's group",
										"/deop <user> - remove user from Admin's group",
										"/stop - save config files, update db records and then stop server",
										NULL
									};*/
	
	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"help_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( args_num > 1 )
	{
		session_send_string(sess, "*COMMAND_PARAMS_NO_NEED\n");
		return;
	}
	
	const char* help = "*HELP_COMMAND_SUCCESS|";
	int len = strlen(help);
	int pos = len;
	memcpy(send_buf, help, len+1);

	if ( sess->rank != ADMIN_RANK_VALUE )
		make_send_buf(send_buf, pos, user_cmd_list);
	else
		make_send_buf(send_buf, pos, admin_cmd_list);
	
	
	session_send_string(sess, send_buf);
}

void whoih_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;

	char send_buf[BUFSIZE] = { 0 };
	char cur_time[CUR_TIME_SIZE];

	StringList* clients_list = clients_online;
	
	/*sl_print(clients_list);*/

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"whoih_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;	
	}

	if ( args_num > 1 )
	{
		session_send_string(sess, "*COMMAND_PARAMS_NO_NEED\n");
		return;
	}
	
	const char* whoih = "*WHOIH_COMMAND_SUCCESS|";
	int len = strlen(whoih);
	int pos = len;
	memcpy(send_buf, whoih, len+1);

	while ( clients_list )
	{
		int i;
		for ( i = 0; i < serv->sess_array_size; i++ )
			if ( serv->sess_array[i] )
				if ( strcmp(clients_list->data, serv->sess_array[i]->login) == 0 )
					break;

		if ( (serv->sess_array[i]->user_status != status_invisible) || (sess->rank == ADMIN_RANK_VALUE) )
		{
			len = strlen(clients_list->data);
			pos += len;
			strncat(send_buf, clients_list->data, len);
		}
		
		clients_list = clients_list->next;

		if ( clients_list )
		{
			send_buf[pos] = '|';
			pos++;
		}
	}
	
	send_buf[pos] = '\n';
	pos++;
	send_buf[pos] = '\0';

	session_send_string(sess, send_buf);
}

void chgpass_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;

	char buffer_pass[100];
	char cur_time[CUR_TIME_SIZE];

	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|CHGPWD|TOO_MUCH_ARGS\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"chgpass_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}

	int len = strlen(cmd_args[1]);
	memcpy(buffer_pass, cmd_args[1], len+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"chgpass_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
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
						"undefined",
						"undefined",
						"undefined",
						"undefined",
						NULL
		};
		
		if ( !write_query_into_db(query_strings) )
		{
			return;
		}

		session_send_string(sess, "*CHGPWD_COMMAND_SUCCESS\n");
		return;
	}
	
	session_send_string(sess, "*CHGPWD_COMMAND_INCORRECT_PASS\n");
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
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
		return;

	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|OP|TOO_MUCH_ARGS\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"op_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}
	
	char buffer_username[100];
	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"op_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}
	

	char id_param[ID_SIZE];
	if ( !get_field_from_db(id_param, buffer_username, ID) )
	{
		return;
	}

	int index = atoi(id_param);
	if ( index < 0 )
	{	
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|OP|USER_NOT_FOUND\n");
		return;
	}
	
	int is_online = 0;
	StringList* cl_onl = clients_online;
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
		for ( i = 0; i < serv->sess_array_size; i++ )
			if ( serv->sess_array[i] )
				if ( strcmp(serv->sess_array[i]->login, buffer_username) == 0 )
					break;

		if ( serv->sess_array[i]->rank != ADMIN_RANK_VALUE )
		{
			serv->sess_array[i]->rank = ADMIN_RANK_VALUE;

			char rank[RANK_SIZE];
			rank[0] = get_user_rank(serv->sess_array[i]->rank);
			rank[1] = '\0';

			if ( serv->sess_array[i]->muted )
				eval_mute_time_left(serv->sess_array[i]);
		
			char smt[START_MUTE_TIME_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, START_MUTE_TIME) )
			{
				return;
			}
			serv->sess_array[i]->start_mute_time = atoi(smt);

			char mt[MUTE_TIME_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, MUTE_TIME) )
			{
				return;
			}
			serv->sess_array[i]->mute_time = atoi(mt);

			char muted[MUTED_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, MUTED) )
			{
				return;
			}
			serv->sess_array[i]->muted = atoi(muted);
			
			char mtl[MUTE_TIME_LEFT_SIZE];
			itoa(serv->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

			const char* query_strings[] = 
			{
							"DB_WRITELINE|",
							serv->sess_array[i]->login,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							muted,
							smt,
							mtl,
							mtl,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};
			
			if ( !write_query_into_db(query_strings) )
			{
				return;
			}
		}
		else
		{
			session_send_string(sess, "*OP_COMMAND_USER_ALREADY_ADMIN\n");
			return;
		}
	}
	else
	{
		char rank[RANK_SIZE];
		if ( !get_field_from_db(rank, buffer_username, RANK) )
		{
			return;
		}

		if ( rank[0] != 'A' )
		{
			rank[0] = 'A';
			const char* query_strings[] = 
			{
							"DB_WRITELINE|",
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
			
			if ( !write_query_into_db(query_strings) )
			{
				return;
			}
		}
		else
		{
			session_send_string(sess, "*OP_COMMAND_USER_ALREADY_ADMIN\n");
			return;
		}
	}


	char** ops_strings = NULL;
	int strings_count = 0;
	
	ops_strings = parse_ops_file(&strings_count);
	
	FILE* dbops = NULL;
	if ( !(dbops = fopen(OPS_NAME, "w")) )
	{
		fprintf(stderr, "[%s] %s You don't have permission to rewrite \"%s\" file\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, OPS_NAME);
		sess->state = fsm_error;
		session_send_string(sess, "*NO_PERM_TO_CREATE_FILE\n");

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

	session_send_string(sess, "*OP_COMMAND_SUCCESS\n");
}

void deop_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) )
		return;

	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|DEOP|TOO_MUCH_ARGS\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"deop_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	char buffer_username[100];
	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"deop_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;	
	}



	/* удаляем пользователя из списка админов в файле ops.txt и перезаписываем этот файл */
	char** ops_strings = NULL;
	int strings_count = 0;
	
	ops_strings = parse_ops_file(&strings_count);
	
	FILE* dbops = NULL;
	if ( !(dbops = fopen(OPS_NAME, "w")) )
	{
		fprintf(stderr, "[%s] %s You don't have permission to rewrite \"%s\" file\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, OPS_NAME);
		sess->state = fsm_error;
		session_send_string(sess, "*NO_PERM_TO_CREATE_FILE\n");

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
		
		session_send_string(sess, "*DEOP_COMMAND_USER_ALREADY_USER\n");
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
		
		session_send_string(sess, "*DEOP_COMMAND_USER_ALREADY_USER\n");
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
	if ( !get_field_from_db(id_param, buffer_username, ID) )
	{
		return;
	}

	int index = atoi(id_param);
	if ( index < 0 )
	{	
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|DEOP|USER_NOT_FOUND\n");
		return;
	}

	int is_online = 0;
	StringList* cl_onl = clients_online;
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
		for ( i = 0; i < serv->sess_array_size; i++ )
			if ( serv->sess_array[i] )
				if ( strcmp(serv->sess_array[i]->login, buffer_username) == 0 )
					break;

		if ( serv->sess_array[i]->rank != ADMIN_RANK_VALUE )
		{
			session_send_string(sess, "*DEOP_COMMAND_USER_ALREADY_USER\n");
			return;
		}
		else
		{
			set_user_rank(serv->sess_array[i]);

			char rank[RANK_SIZE];
			rank[0] = get_user_rank(serv->sess_array[i]->rank);
			rank[1] = '\0';

			if ( serv->sess_array[i]->muted )
				eval_mute_time_left(serv->sess_array[i]);
		
			char smt[START_MUTE_TIME_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, START_MUTE_TIME) )
			{
				return;
			}
			serv->sess_array[i]->start_mute_time = atoi(smt);

			char mt[MUTE_TIME_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, MUTE_TIME) )
			{
				return;
			}
			serv->sess_array[i]->mute_time = atoi(mt);

			char muted[MUTED_SIZE];
			if ( !get_field_from_db(smt, serv->sess_array[i]->login, MUTED) )
			{
				return;
			}
			serv->sess_array[i]->muted = atoi(muted);
			
			char mtl[MUTE_TIME_LEFT_SIZE];
			itoa(serv->sess_array[i]->mute_time_left, mtl, MUTE_TIME_LEFT_SIZE-1);

			const char* query_strings[] = 
			{
							"DB_WRITELINE|",
							serv->sess_array[i]->login,
							"undefined",
							rank,
							"undefined",
							"undefined",
							"undefined",
							muted,
							smt,
							mtl,
							mtl,
							"undefined",
							"undefined",
							"undefined",
							"undefined",
							NULL
			};
			
			if ( !write_query_into_db(query_strings) )
			{
				return;
			}
		}
	}
	else
	{
		char rank[RANK_SIZE];
		if ( !get_field_from_db(rank, buffer_username, RANK) )
		{
			return;
		}

		if ( rank[0] != 'A' )
		{
			session_send_string(sess, "*DEOP_COMMAND_USER_ALREADY_USER\n");
			return;
		}
		else
		{
			char ldi[LAST_DATE_IN_SIZE];
			if ( !get_field_from_db(ldi, buffer_username, LAST_DATE_IN) )
			{
				return;
			}

			char rd[REG_DATE_SIZE];
			if ( !get_field_from_db(rd, buffer_username, REGISTRATION_DATE) )
			{
				return;
			}

			int rank_num = eval_rank_num(ldi, rd);
			rank[0] = get_user_rank(rank_num);

			const char* query_strings[] = 
			{
							"DB_WRITELINE|",
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
			
			if ( !write_query_into_db(query_strings) )
			{
				return;
			}
		}		
	}

	session_send_string(sess, "*DEOP_COMMAND_SUCCESS\n");
}

void pm_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) )
		return;
	
	int user_offline = 1;
	char buffer_username[100] = { 0 };
	char buffer_message[BUFSIZE];
	char cur_time[CUR_TIME_SIZE];

	if ( args_num != 3 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|PM|TOO_MUCH_ARGS\n");
		if ( args_num > 0 )
		{
			if ( !clear_cmd_args(cmd_args, args_num) )
			{
				fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"pm_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
				cmd_args = NULL;
			}
		}
		return;
	}
	
	memcpy(buffer_username, cmd_args[1], strlen(cmd_args[1])+1);
	memcpy(buffer_message, cmd_args[2], strlen(cmd_args[2])+1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"pm_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( strcmp(buffer_username, sess->login) == 0 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|PM|SELF_USE\n");
		return;
	}

	StringList* list = clients_online;
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
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|PM|USER_OFFLINE\n");
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
		return;
	

	const char* success = "*STATUS_COMMAND_SUCCESS|";
	char buffer_message[BUFSIZE] = { 0 };
	char buffer_status[100];
	char cur_time[CUR_TIME_SIZE];

	if ( args_num > 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|STATUS|TOO_MUCH_ARGS\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	const char* user_status_ptr = get_status_str(sess->user_status);
	
	int len = strlen(success);
	int pos = len;
	memcpy(buffer_message, success, len+1);

	if ( args_num == 2 )
	{
		memcpy(buffer_status, cmd_args[1], strlen(cmd_args[1])+1);
		
		if ( strcmp(user_status_ptr, buffer_status) == 0 )
		{
			session_send_string(sess, "*STATUS_COMMAND_ALREADY_SET\n");	
			return;
		}

		if ( strcmp(buffer_status, "list") == 0 )
		{
			int i;
			for ( i = 1; i <= VALID_STATUSES_NUM; i++ )
			{
				const char* stat_ptr = get_status_str(i);
				len = strlen(stat_ptr);
				pos += len;
				strncat(buffer_message, stat_ptr, len);
				buffer_message[pos] = '|';
				pos++;
			}

			if ( sess->rank == ADMIN_RANK_VALUE )
			{
				const char* stat_ptr = get_status_str(SECRET_NUMBER);
				len = strlen(stat_ptr);
				pos += len;
				strncat(buffer_message, stat_ptr, len);
			}
			
			buffer_message[pos] = '\n';
			pos++;
			buffer_message[pos] = '\0';
			
			session_send_string(sess, buffer_message);
		}
		else
		{
			int i;
			for ( i = 1; i <= VALID_STATUSES_NUM; i++ )
			{
				const char* stat_str = get_status_str(i);

				if ( strcmp(stat_str, buffer_status) == 0 )
				{
					sess->user_status = i;
					session_send_string(sess, "*STATUS_COMMAND_SUCCESS\n");
					break;
				}
			}

			if ( i > VALID_STATUSES_NUM )
			{
				int flag = 0;
				if ( sess->rank == ADMIN_RANK_VALUE )
				{
					const char* stat_str = get_status_str(SECRET_NUMBER);

					if ( strcmp(stat_str, buffer_status) == 0 )
					{
						sess->user_status = SECRET_NUMBER;
						session_send_string(sess, "*STATUS_COMMAND_SUCCESS\n");
						flag = 1;
					}
				}

				if ( !flag )
				{
					session_send_string(sess, "*STATUS_COMMAND_INCORRECT_STATUS\n");
				}
			}
		}

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}

		return;
	}
	
	len = strlen(user_status_ptr); 
	pos += len;
	strncat(buffer_message, user_status_ptr, len);
	buffer_message[pos] = '\n';
	pos++;
	buffer_message[pos] = '\0';
		
	session_send_string(sess, buffer_message);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"status_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}
}

/* ResponseRecord - ответ сервера, т.е информация о клиенте в отформатированном виде */
static ResponseRecord* user_show_record(ClientSession* sess, const char* registered_username, int show_record_flag)
{
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (registered_username == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" argument \"registered username\" is NULL!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}


	ResponseRecord* response_struct = malloc(sizeof(ResponseRecord));
	if ( !response_struct )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" memory error(\"response_struct\" is NULL)\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	
	/* Проверка, существует ли запись с таким именем в таблице БД */
	char response[BUFFER_SIZE];
	if ( read_query_from_db(response, registered_username) == -1 )
	{
		if ( response_struct )
			free(response_struct);

		fprintf(stderr, "[%s] %s In function \"user_show_record\" unable to find record with \"%s\" name!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, registered_username);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		return NULL;
	}


	/* извлечение поля RANK из таблицы */
	char rank[RANK_SIZE];
	if ( !get_field_from_db(rank, registered_username, RANK) )
	{
		if ( response_struct )
			free(response_struct);
	
		fprintf(stderr, "[%s] %s [1] In function \"user_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
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
	if ( !get_field_from_db(response_struct->age, registered_username, AGE) )
	{
		if ( response_struct )
			free(response_struct);
	
		fprintf(stderr, "[%s] %s [2] In function \"user_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		return NULL;

	}


	/* извлечение поля REALNAME из таблицы */
	if ( !get_field_from_db(response_struct->realname, registered_username, REALNAME) )
	{
		if ( response_struct )
			free(response_struct);
	
		fprintf(stderr, "[%s] %s [3] In function \"user_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		return NULL;

	}
	

	/* извлечение поля QUOTE из таблицы */
	if ( !get_field_from_db(response_struct->quote, registered_username, QUOTE) )
	{
		if ( response_struct )
			free(response_struct);
	
		fprintf(stderr, "[%s] %s [4] In function \"user_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		return NULL;

	}
	

	/* вычисление значения STATUS заданного клиента */
	int i;
	for ( i = 0; i < serv->sess_array_size; i++ )
		if ( serv->sess_array[i] )
			if ( serv->sess_array[i]->authorized )
				if ( (strcmp(registered_username, serv->sess_array[i]->login) == 0) )
				{
					const char* status = get_status_str(serv->sess_array[i]->user_status);
					strcpy(response_struct->status, status);
					break;
				}

	if ( i == serv->sess_array_size )
	{
		const char* offline = "offline";
		strcpy(response_struct->status, offline);
	}
	
	
	/* извлечение поля REGISTRATION_DATE из таблицы */
	char registration_date[REG_DATE_SIZE];
	if ( !get_field_from_db(registration_date, registered_username, REGISTRATION_DATE) )
	{
		if ( response_struct )
			free(response_struct);
	
		fprintf(stderr, "[%s] %s [5] In function \"user_show_record\" database server sent invalid answer!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
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
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (registered_username == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"debug_show_record\" argument \"registered username\" or \"sess\" is NULL!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}


	int records_size = 0;
	evaluate_size_databases(&records_size);
	

	FILE* dbusers = NULL;
	if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
	{
		fprintf(stderr, "[%s] %s Unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME);
		return NULL;
	}
	
	
	ResponseDebugRecord* response_struct = malloc(sizeof(ResponseRecord));
	if ( !response_struct )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" memory error(\"response_struct\" is NULL)\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}


	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"user_show_record\" memory error.\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		if ( dbusers )
			fclose(dbusers);

		return NULL;
	}

	int i;
	for ( i = 0; i < records_size; i++ )
	{
		memset(record, 0, sizeof(DBUsersInformation));
		fseek(dbusers, i * sizeof(DBUsersInformation), SEEK_SET);
		fread(record, sizeof(DBUsersInformation), 1, dbusers);
		
		if ( strcmp(record->username, registered_username) == 0 )
		{
			const char* undef = "undefined";
			const char* fresh = "FRESHMAN";
			const char* member = "MEMBER";
			const char* wisdom = "WISDOM";
			const char* old = "OLD";
			const char* admin = "ADMIN";
			

			int j;
			switch ( record->rank[0] )
			{
				case 'u':
					memcpy(response_struct->rank, undef, strlen(undef)+1);
					break;
				case 'F':
					memcpy(response_struct->rank, fresh, strlen(fresh)+1);
					break;
				case 'M':
					memcpy(response_struct->rank, member, strlen(member)+1);
					break;
				case 'W':
					memcpy(response_struct->rank, wisdom, strlen(wisdom)+1);
					break;
				case 'O':
					memcpy(response_struct->rank, old, strlen(old)+1);
					break;
				case 'A':
					memcpy(response_struct->rank, admin, strlen(admin)+1);		
			}
			
			
			char buffer_username[LOGIN_SIZE] = { 0 };
			const char* ends = "'s\"";
			buffer_username[0] = '"';
			
			for ( j = 0, i = 1; registered_username[j]; j++, i++ )
				buffer_username[i] = registered_username[j];
			
			for ( j = 0; ends[j]; j++, i++ )
				buffer_username[i] = ends[j];
			buffer_username[i] = '\0';
			
			memcpy(response_struct->username, buffer_username, strlen(buffer_username)+1);

			break;
		}
	}
	
	if ( dbusers )
		fclose(dbusers);

	if ( record )
		free(record);


	if ( i >= records_size )
	{
		fprintf(stderr, "[%s] %s In function \"debug_show_record\" unable to find record in \"%s\" with \"%s\" name!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME, registered_username);
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		return NULL;
	}
	
	int index = i;

	for ( i = 0; i < serv->sess_array_size; i++ )
		if ( serv->sess_array[i] )
			if ( serv->sess_array[i]->authorized )
				if ( (strcmp(registered_username, serv->sess_array[i]->login) == 0) )
				{
					const char* status = get_status_str(serv->sess_array[i]->user_status);
					memcpy(response_struct->status, status, strlen(status)+1);
					
					index = i;
					break;
				}

	if ( i == serv->sess_array_size )
	{
		const char* offline = "offline";
		memcpy(response_struct->status, offline, strlen(offline)+1);
	}

	itoa(serv->sess_array[index]->ID, response_struct->id, ID_SIZE-1);
	itoa(serv->sess_array[index]->authorized, response_struct->auth, AUTH_STR_SIZE-1);
	itoa(serv->sess_array[index]->buf_used, response_struct->used, USED_STR_SIZE-1);
	memcpy(response_struct->last_date_in, serv->sess_array[index]->last_date_in, LAST_DATE_IN_SIZE);
	memcpy(response_struct->last_ip, serv->sess_array[index]->last_ip, LAST_IP_SIZE);
	memcpy(response_struct->regdate, serv->sess_array[index]->registration_date, REG_DATE_SIZE);
	memcpy(response_struct->pass, serv->sess_array[index]->pass, PASS_SIZE);
	itoa(serv->sess_array[index]->sockfd, response_struct->sock, SOCK_STR_SIZE-1);
	itoa(serv->sess_array[index]->state, response_struct->state, STATE_STR_SIZE-1);

	eval_mute_time_left(serv->sess_array[index]);
	update_ext_usersinfo_records(serv->sess_array[index]);

	itoa(serv->sess_array[index]->muted, response_struct->muted, MUTED_SIZE-1);
	itoa(serv->sess_array[index]->mute_time, response_struct->mute_time, MUTE_TIME_SIZE-1);
	itoa(serv->sess_array[index]->mute_time_left, response_struct->mute_time_left, MUTE_TIME_LEFT_SIZE-1);
	itoa(serv->sess_array[index]->start_mute_time, response_struct->start_mute_time, START_MUTE_TIME_SIZE-1);

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
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (username == NULL) || (type == NULL) )
	{
		fprintf(stderr, "[%s] %s In function \"send_record_response\" \"sess\", \"username\", or \"type\" is NULL\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char success_string[BUFSIZE] = { 0 };
	const char* success_code = "*RECORD_COMMAND_SUCCESS|";

	
	int len = strlen(success_code);
	int cur_pos = len;
	strncat(success_string, success_code, len);

	len = strlen(type);
	cur_pos += len;
	strncat(success_string, type, len);	
	success_string[cur_pos] = '|';
	cur_pos++;

	if ( strcmp(type, "record") == 0 )
	{
		ResponseRecord* response = user_show_record(sess, username, 0);
		const char* args_num = "10";
		

		const char* args[] = 
		{
				args_num,
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
			len = strlen(args[i]);
			cur_pos += len;
			strncat(success_string, args[i], len);
			success_string[cur_pos] = '|';
			cur_pos++;
		}
		
		success_string[cur_pos-1] = '\n';
		success_string[cur_pos] = '\0';

		free(response);
	}
	else if ( strcmp(type, "debug") == 0 )
	{
		ResponseDebugRecord* response = debug_show_record(sess, username, 0);
		const char* args_num = "20";

		
		const char* args[] = 
		{
				args_num,
				response->username,
				response->id,
				response->auth,
				response->used,
				response->last_date_in,
				response->last_ip,
				response->regdate,
				response->username,
				response->pass,
				response->rank,
				response->sock,
				response->state,
				response->status,
				response->muted,
				response->mute_time,
				response->mute_time_left,
				response->start_mute_time
		};
		
		int i;
		for ( i = 0; i < DEBUG_RECORD_FIELDS_NUM+1; i++ )
		{
			len = strlen(args[i]);
			cur_pos += len;
			strncat(success_string, args[i], len);
			success_string[cur_pos] = '|';
			cur_pos++;
		}
		
		success_string[cur_pos-1] = '\n';
		success_string[cur_pos] = '\0';

		free(response);
	}

	session_send_string(sess, success_string);
}

void record_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;
	

	if ( args_num > 3 )
	{	
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|TOO_MUCH_ARGS\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"record_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
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
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"record_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}


	int records_size = 0;
	evaluate_size_databases(&records_size);


	if ( args_num == 1 )
	{
		send_record_response(sess, sess->login, "record");
	}
	else if ( args_num == 2 )
	{		
		int index = get_record_index_by_name(buffer_param, DB_USERINFO_NAME);
		if ( index != -1 )
		{
			FILE* dbusers = NULL;
			if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
			{
				fprintf(stderr, "[%s] %s Unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, DB_USERINFO_NAME);
				return;
			}

			DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
			fseek(dbusers, index * sizeof(DBUsersInformation), SEEK_SET);
			fread(record, sizeof(DBUsersInformation), 1, dbusers);
			
			if ( dbusers )
				fclose(dbusers);

			if ( record->rank[0] == 'A' )
			{
				if ( sess->rank != ADMIN_RANK_VALUE )
				{
					session_send_string(sess, "*COMMAND_NO_PERMS\n");
					free(record);
					return;
				}
			}

			send_record_response(sess, buffer_param, "record");
			free(record);
		}
		else
		{
			session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
		}
	}
	else if ( args_num == 3 )
	{
		enum { REALNAME, AGE, QUOTE, DEBUG };
		const char* valid_params[] = { "realname", "age", "quote", "debug", NULL };
		int valid_param_flag = 0;
		int update_flag = 0;


		int index = -1;
		index = get_record_index_by_name(sess->login, DB_USERINFO_NAME);
		if ( index == -1 )
		{
			fprintf(stderr, "[%s] %s In function \"record_command_handler\" unable to find record in \"%s\" with \"%s\" name!\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME, sess->login);
			return;
		}

		FILE* dbusers = NULL;
		if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
		{
			fprintf(stderr, "[%s] %s Unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME);
			return;
		}
		
		
		DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
		memset(record, 0, sizeof(DBUsersInformation));
		fseek(dbusers, index * sizeof(DBUsersInformation), SEEK_SET);
		fread(record, sizeof(DBUsersInformation), 1, dbusers);
		fclose(dbusers);
		dbusers = NULL;

		if ( (strcmp(buffer_param, valid_params[DEBUG]) == 0) )
		{
			if ( sess->rank != ADMIN_RANK_VALUE )
			{
				session_send_string(sess, "*COMMAND_NO_PERMS\n");
			}
			else
			{
				if ( get_record_index_by_name(buffer_param_value, DB_USERINFO_NAME) != -1 )
					send_record_response(sess, buffer_param_value, "debug");
				else
					session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|USER_NOT_FOUND\n");
			}
			valid_param_flag = 1;
		}
		else if ( strcmp(buffer_param, valid_params[REALNAME]) == 0 )
		{
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
			
			if ( is_correct_symbol )
			{
				if ( j < REALNAME_SIZE )
				{
					valid_param_flag = 1;
					memcpy(record->realname, buffer_param_value, strlen(buffer_param_value)+1);
					update_flag = 1;
				}
			}
		}
		else if ( strcmp(buffer_param, valid_params[AGE]) == 0 )
		{
			int age = atoi(buffer_param_value);
			if ( (age > MIN_AGE_VALUE) && (age < MAX_AGE_VALUE) )
			{
				record->age = age;
				valid_param_flag = 1;
			}
			else
			{
				record->age = -1;
			}

			update_flag = 1;
		}
		else if ( strcmp(buffer_param, valid_params[QUOTE]) == 0 )
		{
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

			if ( is_correct_symbol )
			{
				if ( j < QUOTE_SIZE )
				{
					valid_param_flag = 1;
					memcpy(record->quote, buffer_param_value, strlen(buffer_param_value)+1);
					update_flag = 1;
				}
			}
		}
			

		if ( !valid_param_flag )
			session_send_string(sess, "*COMMAND_INVALID_PARAMS|RECORD|INCORRECT_STRING_VALUE\n");
		
		if ( update_flag )
		{
			if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb+")) )
			{
				fprintf(stderr, "[%s] %s Unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE, DB_USERINFO_NAME);
				session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
				
				if ( record )
					free(record);
			}

			fseek(dbusers, index * sizeof(DBUsersInformation), SEEK_SET);
			fwrite(record, sizeof(DBUsersInformation), 1, dbusers);
			
			if ( dbusers )
				fclose(dbusers);
		}

		if ( record )
			free(record);
	}
}

static void send_mute_response(ClientSession* sess, const char* username)
{
	const char* victim_message = "*MUTE_COMMAND_YOU_MUTED|";
	char response_victim[100];


	int len = strlen(victim_message);
	int pos = len;
	memcpy(response_victim, victim_message, len+1);
	
	int j;
	for ( j = 0; j < serv->sess_array_size; j++ )
		if ( serv->sess_array[j] )
			if ( strcmp(serv->sess_array[j]->login, username) == 0 )
				break;
	int index = j;

		
	char mt[MUTE_TIME_LEFT_SIZE];
	itoa(serv->sess_array[index]->mute_time_left, mt, MUTE_TIME_LEFT_SIZE-1);
	
	int mt_len = strlen(mt);
	pos += mt_len;
	strncat(response_victim, mt, mt_len);
	response_victim[pos] = '\n';
	pos++;
	response_victim[pos] = '\0';

	session_send_string(serv->sess_array[index], response_victim);
	


	const char* mute_success = "*MUTE_COMMAND_SUCCESS|";
	char response_sender[100];

	len = strlen(mute_success);
	pos = len;
	memcpy(response_sender, mute_success, len+1);

	
	len = strlen(username);
	pos += len;
	strncat(response_sender, username, len);
	response_sender[pos] = '|';
	pos++;
	response_sender[pos] = '\0';

	pos += mt_len;
	strncat(response_sender, mt, mt_len);
	response_sender[pos] = '\n';
	pos++;
	response_sender[pos] = '\0';

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

void mute_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;
	
	StringList* users_list = clients_online;

	if ( args_num != 3 )
	{	
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|TOO_MUCH_ARGS\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	char username_buf[LOGIN_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1])+1);

	if ( !is_valid_auth_str(username_buf, 0) )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|INCORRECT_USERNAME\n");

		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	int index = get_record_index_by_name(username_buf, DB_USERINFO_NAME);
	if ( index == -1 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|USER_NOT_FOUND\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[3]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	int is_online = 0;
	while ( users_list )
	{
		if ( strcmp(username_buf, users_list->data) == 0 )
		{
			if ( strcmp(username_buf, sess->login) == 0 )
			{
				session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|SELF_USE\n");
			
				if ( !clear_cmd_args(cmd_args, args_num) )
				{
					fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[4]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
					cmd_args = NULL;
				}
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
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|USER_OFFLINE\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[5]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	FILE* dbxusers = NULL;
	if ( !(dbxusers = fopen(DB_XUSERINFO_NAME, "rb")) )
	{	
		session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[6]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}

	DBXUsersInformation* record = malloc(sizeof(DBXUsersInformation));
	fseek(dbxusers, index * sizeof(DBXUsersInformation), SEEK_SET);
	fread(record, sizeof(DBXUsersInformation), 1, dbxusers);
	
	if ( dbxusers )
		fclose(dbxusers);
	
	int is_muted = record->muted;
	free(record);

	if ( is_muted )
	{	
		session_send_string(sess, "*MUTE_COMMAND_USER_ALREADY_MUTED\n");
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[7]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	char time_val[START_MUTE_TIME_STR_SIZE];
	memcpy(time_val, cmd_args[2], strlen(cmd_args[2]) + 1);

	int mute_time = atoi(time_val);
	if ( (mute_time < MIN_MUTE_TIME_SEC) || (mute_time > MAX_MUTE_TIME_SEC) )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|MUTE|INCORRECT_TIME_RANGE\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[8]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	int i;
	for ( i = 0; i < serv->sess_array_size; i++)
	{
		if ( serv->sess_array[i] )
		{
			if ( strcmp(serv->sess_array[i]->login, username_buf) == 0 )
			{
				serv->sess_array[i]->muted = 1;
				serv->sess_array[i]->mute_time = mute_time;
				serv->sess_array[i]->mute_time_left = mute_time;
				time_t t = time(0);
				serv->sess_array[i]->start_mute_time = t;
				
				update_ext_usersinfo_records(serv->sess_array[i]);
				break;
			}
		}
	}
	
	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"mute_command_handler\"[9]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}
		
	send_mute_response(sess, username_buf);
}

void unmute_command_handler(ClientSession *sess, char **cmd_args, int args_num)
{
	char cur_time[CUR_TIME_SIZE];

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;

	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|UNMUTE|TOO_MUCH_ARGS\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unmute_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}


	char username_buf[USERNAME_STR_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1])+1);
	

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unmute_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( !is_valid_auth_str(username_buf, 0) )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|UNMUTE|INCORRECT_USERNAME\n");
		return;
	}
	
	int user_index = get_record_index_by_name(username_buf, DB_USERINFO_NAME);
	if ( user_index == -1 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|UNMUTE|USER_NOT_FOUND\n");
		return;
	}
	
	if ( strcmp(sess->login, username_buf) == 0 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|UNMUTE|SELF_USE\n");
		return;
	}
	
	FILE* dbxusers = NULL;
	if ( !(dbxusers = fopen(DB_XUSERINFO_NAME, "rb")) )
	{	
		session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
		return;
	}

	DBXUsersInformation* record = malloc(sizeof(DBXUsersInformation));
	fseek(dbxusers, user_index * sizeof(DBXUsersInformation), SEEK_SET);
	fread(record, sizeof(DBXUsersInformation), 1, dbxusers);
	fclose(dbxusers);

	int is_muted = record->muted;
	free(record);

	if ( !is_muted )
	{	
		session_send_string(sess, "*UNMUTE_COMMAND_USER_NOT_MUTED\n");
		return;
	}
	
	int i;
	for ( i = 0; i < serv->sess_array_size; i++ )
	{
		if ( serv->sess_array[i] )
		{
			if ( strcmp(serv->sess_array[i]->login, username_buf) == 0 )
			{
				serv->sess_array[i]->muted = 0;
				serv->sess_array[i]->mute_time = 0;
				serv->sess_array[i]->mute_time_left = 0;
				serv->sess_array[i]->start_mute_time = 0;
				update_ext_usersinfo_records(serv->sess_array[i]);

				session_send_string(serv->sess_array[i], "*UNMUTE_COMMAND_YOU_UNMUTED\n");
				break;
			}
		}
	}

	char buffer[100];
	const char* unmute_success = "*UNMUTE_COMMAND_SUCCESS|";
	int len = strlen(unmute_success);
	int pos = len;
	memcpy(buffer, unmute_success, len+1);

	len = strlen(username_buf);
	pos += len;
	strncat(buffer, username_buf, len);
	buffer[pos] = '\n';
	pos++;
	buffer[pos] = '\0';

	session_send_string(sess, buffer);
}

void kick_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	char cur_time[CUR_TIME_SIZE];	

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;

	StringList* users_list = clients_online;

	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|KICK|TOO_MUCH_ARGS\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"kick_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}

	
	char username_buf[USERNAME_STR_SIZE];
	memcpy(username_buf, cmd_args[1], strlen(cmd_args[1])+1);
	

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"kick_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}
	
	if ( !is_valid_auth_str(username_buf, 0) )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|KICK|INCORRECT_USERNAME\n");
		return;
	}
	
	int user_index = get_record_index_by_name(username_buf, DB_USERINFO_NAME);
	if ( user_index == -1 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|KICK|USER_NOT_FOUND\n");
		return;
	}
	
	int is_online = 0;
	while ( users_list )
	{
		if ( strcmp(username_buf, users_list->data) == 0 )
		{
			if ( strcmp(username_buf, sess->login) == 0 )
			{
				session_send_string(sess, "*COMMAND_INVALID_PARAMS|KICK|SELF_USE\n");
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
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|KICK|USER_OFFLINE\n");
		return;
	}
	
	int i;
	for ( i = 0; i < serv->sess_array_size; i++ )
	{
		if ( serv->sess_array[i] )
		{
			if ( strcmp(serv->sess_array[i]->login, username_buf) == 0 )
			{
				session_send_string(serv->sess_array[i], "*KICK_COMMAND_SUCCESS|VICTIM\n");
				int sock = serv->sess_array[i]->sockfd;
				server_close_session(sock);
			}
		}
	}
	
	const char* str = "*KICK_COMMAND_SUCCESS|SENDER|";
	char response[100];

	int len = strlen(str);
	int pos = len;
	memcpy(response, str, len+1);

	len = strlen(username_buf);
	pos += len;
	strncat(response, username_buf, len);
	response[pos] = '\n';
	pos++;
	response[pos] = '\0';

	session_send_string(sess, response);
}

void table_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{	
	char cur_time[CUR_TIME_SIZE];	

	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;


	if ( args_num != 2 )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|TABLE|TOO_MUCH_ARGS\n");
		
		if ( !clear_cmd_args(cmd_args, args_num) )
		{
			fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"table_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
			cmd_args = NULL;
		}
		return;
	}
	
	char buffer_value[100];
	memcpy(buffer_value, cmd_args[1], strlen(cmd_args[1]) + 1);

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"table_command_handler\"[2]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	if ( strcmp(buffer_value, "list") == 0 )
	{
		session_send_string(sess, "*TABLE_COMMAND_SUCCESS|LIST|users_data.dat|users_sessions_info.dat\n");
		return;
	}

	int is_userdata = (strcmp(buffer_value, DB_USERINFO_NAME) == 0) ? 1 : 0;
	int is_session_userinfo = (strcmp(buffer_value, DB_XUSERINFO_NAME) == 0) ? 1 : 0;
	
	if ( (!is_userdata) && (!is_session_userinfo) )
	{
		session_send_string(sess, "*COMMAND_INVALID_PARAMS|TABLE|INCORRECT_STRING_VALUE\n");
		return;
	}

	const char* resp_start = "*TABLE_COMMAND_SUCCESS|DATA|";
	char response_buffer[BUFSIZE] = { 0 };
	
	int len = strlen(resp_start);
	int pos = len;
	memcpy(response_buffer, resp_start, len+1);
		
	
	int records_size = 0;
	evaluate_size_databases(&records_size);
		

	char rec_size[10];
	itoa(records_size, rec_size, 9);
	len = strlen(rec_size);
	pos += len;
	strncat(response_buffer, rec_size, len);
	response_buffer[pos] = '|';
	pos++;
	

	FILE* dbusers = NULL;
	if ( !(dbusers = fopen(DB_USERINFO_NAME, "rb")) )
	{	
		session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
		return;
	}
	
	
	int* db_non_empty_idxs = malloc(sizeof(int) * records_size);

	int i;
	for ( i = 0; i < records_size; i++ )
		db_non_empty_idxs[i] = -1;


	char non_empty_rec[10];
	int non_empty_recs_counter = 0;
	

	for (i = 0; i < records_size; i++ )
	{
		DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
		fseek(dbusers, i * sizeof(DBUsersInformation), SEEK_SET);
		fread(record, sizeof(DBUsersInformation), 1, dbusers);

		if ( (record->ID == -1) || (strcmp(record->pass, "undefined") == 0) || (strcmp(record->username, "undefined") == 0) || (record->rank[0] == 'u') )
		{
			free(record);
			continue;
		}

		db_non_empty_idxs[i] = i;
		non_empty_recs_counter++;

		free(record);
	}


	itoa(non_empty_recs_counter, non_empty_rec, 9);
	len = strlen(non_empty_rec);
	pos += len;
	strncat(response_buffer, non_empty_rec, len);
	response_buffer[pos] = '|';
	pos++;

	if ( is_userdata )
	{
		const char* dbu = "USERINFO|";
		
		len = strlen(dbu);
		pos += len;
		strncat(response_buffer, dbu, len);


		DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
		if ( !record )
		{
			if ( db_non_empty_idxs )
				free(db_non_empty_idxs);
			
			if ( dbusers )
				fclose(dbusers);

			fprintf(stderr, "[%s] %s In function \"table_command_handler\" memory error with \"record\"\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
			return;
		}

		for ( i = 0; i < records_size; i++ )
		{
			memset(record, 0, sizeof(DBUsersInformation));
			fseek(dbusers, i * sizeof(DBUsersInformation), SEEK_SET);
			fread(record, sizeof(DBUsersInformation), 1, dbusers);

			if ( db_non_empty_idxs[i] > -1 )
			{
				char buf_id[ID_STR_SIZE];
				itoa(record->ID, buf_id, ID_STR_SIZE-1);
				
				enum { ARGS = 4 };
				const char* args[ARGS] = 
				{
					buf_id,
					record->username,
					record->pass,
					record->rank
				};

				int j;
				for ( j = 0; j < ARGS; j++ )
				{
					len = strlen(args[j]);
					pos += len;
					strncat(response_buffer, args[j], len);
					response_buffer[pos] = '|';
					pos++;
				}
			}
		}

		if ( dbusers )
			fclose(dbusers);

		if ( record )
			free(record);
	}
	else
	{
		if ( dbusers )
			fclose(dbusers);

		FILE* dbxusers = NULL;
		if ( !(dbxusers = fopen(DB_XUSERINFO_NAME, "rb")) )
		{	
			if ( db_non_empty_idxs )
				free(db_non_empty_idxs);

			session_send_string(sess, "*CANNOT_CONNECT_DATABASE\n");
			return;
		}

		const char* dbx = "XUSERINFO|";
		len = strlen(dbx);
		pos += len;
		strncat(response_buffer, dbx, len);

	
		DBXUsersInformation* record = malloc(sizeof(DBXUsersInformation));
		if ( !record )
		{
			if ( db_non_empty_idxs )
				free(db_non_empty_idxs);
			
			if ( dbxusers )
				fclose(dbxusers);

			fprintf(stderr, "[%s] %s In function \"table_command_handler\" memory error with \"record\"\n", get_time_str(cur_time, CUR_TIME_SIZE), ERROR_MESSAGE_TYPE);
			return;
		}

		for ( i = 0; i < records_size; i++ )
		{
			memset(record, 0, sizeof(DBXUsersInformation));
			fseek(dbxusers, i * sizeof(DBXUsersInformation), SEEK_SET);
			fread(record, sizeof(DBXUsersInformation), 1, dbxusers);

			if ( db_non_empty_idxs[i] > -1 )
			{
				char buf_id[10];
				itoa(record->ID, buf_id, 10);
				
				enum { ARGS = 5 };
				const char* args[] =
				{
						buf_id,
						record->registration_date,
						record->last_date_in,
						record->last_date_out,
						record->last_ip
				};
				
				int j;
				for ( j = 0; j < ARGS; j++ )
				{
					len = strlen(args[j]);
					pos += len;
					strncat(response_buffer, args[j], len);
					response_buffer[pos] = '|';
					pos++;
				}

			}
		}
		
		if ( record )
			free(record);

		if ( dbxusers )
			fclose(dbxusers);
	}

	response_buffer[pos-1] = '\n';
	response_buffer[pos] = '\0';
	
	if ( db_non_empty_idxs )
		free(db_non_empty_idxs);

	session_send_string(sess, response_buffer);
}

void ban_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;
	
	char cur_time[CUR_TIME_SIZE];

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"ban_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
		cmd_args = NULL;
	}

	return;
}

void unban_command_handler(ClientSession* sess, char** cmd_args, int args_num)
{
	if ( (sess == NULL) || (cmd_args == NULL) || (args_num < 1) )
		return;
	
	char cur_time[CUR_TIME_SIZE];

	if ( !clear_cmd_args(cmd_args, args_num) )
	{
		fprintf(stderr, "[%s] %s Unable to clear cmd args(in \"unban_command_handler\"[1]). \"cmd_args\" value is %p\n", get_time_str(cur_time, CUR_TIME_SIZE), WARN_MESSAGE_TYPE, cmd_args);
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
	char cur_time[CUR_TIME_SIZE];

	if ( line_length >= str_size )
		line_length = str_size-1;
	
	draw_line(line_length, 0);

	int k;
	for ( k = 0; k < str_size; k++ )
	{
		if ( ( k % line_length ) == 0 )
			printf("\n[%s] %s ", get_time_str(cur_time, CUR_TIME_SIZE), INFO_MESSAGE_TYPE);

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
	char cur_time[CUR_TIME_SIZE];

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

	int k;
	char* buffer_str = malloc(sizeof(char) * BUFSIZE);
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
	for ( i = 0; i < serv->sess_array_size; i++ )
	{
		if ( serv->sess_array[i] )
		{
			if ( (i == sess->sockfd) || (!serv->sess_array[i]->authorized) )
				continue;

			if ( is_private )
			{
				if ( strcmp(serv->sess_array[i]->login, adresat) != 0 )
					continue;

				int bytes_sent = write(i, str, mes_size); 
				printf("[%s] %s Sent %d bytes to %s\n", get_time_str(cur_time, CUR_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, serv->sess_array[i]->last_ip);
				view_data(str, bytes_sent, 'c', 50);
				view_data(str, bytes_sent, 'd', 50);
				break;
			}

			int bytes_sent = write(i, str, mes_size); 
			printf("[%s] %s Sent %d bytes to %s\n", get_time_str(cur_time, CUR_TIME_SIZE), INFO_MESSAGE_TYPE, bytes_sent, serv->sess_array[i]->last_ip);
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
				*cmd_num = -2;
				*args_num = 0;
				return NULL;
			}

			cmd_args[k] = malloc(sizeof(char) * (arg_length + 1));
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
				
				for ( i = 0; i < *args_num; i++ )
				{
					int str_len = strlen(args[i]);
					cmd_args[i] = malloc(sizeof(char) * str_len + 1);
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

		*cmd_num = -1;
		*args_num = 0;

		return NULL;
	}

	*cmd_num = 0;
	*args_num = 0;

	return NULL;
}


#endif

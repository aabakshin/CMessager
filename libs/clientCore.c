#ifndef CLIENTCORE_C_SENTRY
#define CLIENTCORE_C_SENTRY

#include "../includes/Commons.h"
#include "../includes/DateTime.h"
#include "../includes/clientCore.h"
#include "../includes/Input.h"
#include "../includes/Graphics.h"

enum
{
	HAS_ACCOUNT_VALUE_LENGTH			=			   3,
	VALID_SYMBOLS_NUM					=			  62,
	MAX_TOKENS_NUM						=			 341,	/* если BUFSIZE = 1024 */
};


static int send_answer(int peer_sock, const char** box_messages, int box_messages_size, int max_read_chars);
static int view_record_success_result(char** response_tokens, int fields_num, int debug_mode);


extern const char* server_codes_list[SERVER_CODES_COUNT];
extern const char* subcommands_codes_list[SUBCOMMANDS_CODES_COUNT];

char* get_code(void)
{
	char cur_time[CURRENT_TIME_SIZE];
	const char* symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	char* buf = malloc(CAPTCHA_CODE_LENGTH+1);
	if ( !buf )
	{
		fprintf(stderr, "\n[%s] %s In function \"get_code\" memory error\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int i;
	for ( i = 0; i < CAPTCHA_CODE_LENGTH; i++ )
		buf[i] = symbols[rand() % VALID_SYMBOLS_NUM];
	buf[i] = '\0';

	return buf;
}

int restrict_message_length(char* read)
{
	int length = strlen(read);

	if ( length > MAX_MESSAGE_LENGTH )
	{
		read[MAX_MESSAGE_LENGTH] = '\0';

		return MAX_MESSAGE_LENGTH;
	}

	return length;
}

void delete_extra_spaces(char* read, int read_size)
{
	/* Удаляет пробелов из начала сообщения */
	///////////////////////////////////////////////////////////
	int i = 0, c = 0;

	while ( read[i++] == ' ' )
		c++;

	for ( i = c; i < read_size; i++ )
		read[i-c] = read[i];
	///////////////////////////////////////////////////////////


	/* Удаление пробелов из середины сообщения */
	///////////////////////////////////////////////////////////
	char* message_tokens[MAX_TOKENS_NUM] = { NULL };
	char* istr = strtok(read, " ");

	int m = 0;
	i = 0;
	c = 0;

	while ( istr != NULL )
	{
		message_tokens[i] = istr;
		while ( message_tokens[i][m++] == ' ' )
			c++;

		for ( m = c; m < strlen(message_tokens[i])+1; m++ )
			message_tokens[i][m-c] = message_tokens[i][m];

		i++;
		m = 0;
		c = 0;
		istr = strtok(NULL, " ");
	}
	int size = i;
	///////////////////////////////////////////////////////////


	/* Удаление пробелов с конца сообщения */
	///////////////////////////////////////////////////////////
	int k = 0;
	int j;
	for ( i = 0; i < size; i++ )
	{
		for ( j = 0; j < strlen(message_tokens[i]); j++ )
			read[k++] = message_tokens[i][j];
		read[k++] = ' ';
	}
	read[k-1] = '\0';
	///////////////////////////////////////////////////////////
}

static int send_answer(int peer_sock, const char** box_messages, int box_messages_size, int max_read_chars)
{
	// Размер message должен быть как минимум max_read_chars+2 байт
	char message[100] = { 0 };
	int len = 0;

	do
	{
		printf("\033c");
		show_logo();
		print_greeting_text_frame(box_messages, box_messages_size);
		len = input(message, max_read_chars);

		if ( len == EXIT_CODE )
		{
			return 0;
		}
	}
	while ( len < 2 );

	if ( len > max_read_chars )
		clear_stdin();

	sendall(peer_sock, message, &len);
	printf("Sent %d bytes\n", len);

	return 1;
}

static int view_record_success_result(char** response_tokens, int fields_num, int debug_mode)
{
	char** args = malloc(sizeof(char*) * fields_num);
	if ( !args )
		return 0;

	for ( int i = 0, j = 3; i < fields_num; i++, j++ )
	{
		args[i] = malloc(sizeof(char) * strlen(response_tokens[j]) + 1 );
		if ( !(args[i]) )
		{
			for ( int m = 0; m < i; m++ )
				if ( args[m] )
					free(args[m]);

			if ( args )
				free(args);

			return 0;
		}

		int k;
		for ( k = 0; response_tokens[j][k]; k++ )
			args[i][k] = response_tokens[j][k];
		args[i][k] = '\0';
	}

	print_record(args, fields_num, debug_mode);

	for ( int i = 0; i < fields_num; i++ )
		if ( args[i] )
			free(args[i]);

	if ( args )
		free(args);

	return 1;
}

int client_init(const char* address, const char* port)
{
	int ok;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	ok = inet_aton(address, &(addr.sin_addr));
	if ( !ok )
	{
		fprintf(stderr, "%s", "Incorrect address!\n");
		return -1;
	}

	int port_number = atoi(port);
	if ( port_number < 1024 )
	{
		fprintf(stderr, "%s", "Incorrect port number!\n");
		return -1;
	}
	addr.sin_port = htons(port_number);

	printf("%s\n", "Creating socket..");
	int peer_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( peer_sock == -1 )
	{
		fprintf(stderr, "socket() failed. {%d}\n", errno);
		return -1;
	}

	printf("%s\n", "Connecting...");
	if ( connect(peer_sock, (struct sockaddr*) &addr, sizeof(addr)) == -1 )
	{
		fprintf(stderr, "connect() failed. {%d}\n", errno);
		return -1;
	}
	printf("%s\n", "Connected.");

	return peer_sock;
}

/* Обработка полученной информации от сервера в соответствии с протоколом общения */
int check_server_response(int peer_sock, char** response_tokens, int response_tokens_size, int* authorized)
{
	if ( strcmp(response_tokens[0], server_codes_list[CLIENT_HAS_ACCOUNT_CODE] ) == 0 )
	{
		int max_read_chars = HAS_ACCOUNT_VALUE_LENGTH;
		const char* box_messages[] = { "Have you already have an account?", "Enter \"y\"or\"n\"", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_WAIT_LOGIN_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "Enter nickname for your account", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_ALREADY_AUTHORIZED_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "This account is already authorized", "Try to use another login", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_ALREADY_USED_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "This login is already exist in database", "Try another one",  NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_INCORRECT_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "Incorrect login!", "Please, check it and try again", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_NOT_EXIST_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "This login doesn't exist", "Check your input string", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[SIGNUP_WAIT_LOGIN_CODE]) == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "Enter nickname to create", "new account", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[LOGIN_WAIT_PASS_CODE]) == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = { "Enter password for your account", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[NEW_PASS_INCORRECT_CODE]) == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = { "Password incorrect!", "Check it and try again", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[PASS_NOT_MATCH_CODE]) == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = { "Password doesn't match with this account", "Try again", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[SIGNUP_WAIT_PASS_CODE]) == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = { "Enter pass for your new account", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		if ( !send_answer(peer_sock, box_messages, box_messages_size, max_read_chars) )
			return -1;

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[USER_AUTHORIZED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("\"%s\" joined to GLOBAL chat\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[USER_LEFT_CHAT_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("\"%s\" left GLOBAL chat\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[CANNOT_CONNECT_DATABASE_CODE]) == 0 )
	{
		printf("%s", "\033c");
		printf("%s", "\n                            ");
		printf("%s", "Server cannot connect to database server. Try later\n");

		return -1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[CMD_ARG_OVERLIMIT_LENGTH_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("Some command or argument in your input is too long! Max length is %s\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[HELP_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "--- List of all valid commands ---\n");
		for ( int k = 1; k < response_tokens_size; k++ )
			printf("[%d]: %s\n", k, response_tokens[k]);
		printf("%s", "----------------------------------\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[WHOIH_COMMAND_SUCCESS_CODE]) == 0 )
	{
		int users_counter = response_tokens_size-1;

		if ( users_counter >= 1 )
		{
			printf("%s", "There ");
			(users_counter > 1) ? printf("are %d users online:\n", users_counter) : printf("is %d user online:\n", users_counter);

			for ( int i = 1; i <= users_counter; i++ )
				printf("%s\n", response_tokens[i]);
			printf("%s\n", "------------------------");
		}
		else
			printf("%s\n", "There is nobody online :(");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[CHGPWD_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "Your password has been successfully changed\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[CHGPWD_COMMAND_INCORRECT_PASS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("Incorrect password. Try to follow next rules:\n");
		printf("%s", "\n                            ");
		printf("Password must be not less than 4 symbols\n");
		printf("%s", "\n                            ");
		printf("Password must be not more than 20 symbols\n");
		printf("%s", "\n                            ");
		printf("Password has to consist correct symbols(a-zA-Z0-9 and special chars)\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[DEOP_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "User has been removed from Admin group\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[DEOP_COMMAND_USER_ADREADY_USER_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "This user is not an administrator\n"
				"or has been removed from admin's group earlier\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[OP_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "User has been added to Admin's group\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[OP_COMMAND_USER_ALREADY_ADMIN_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "User is already in Admin's group\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[STATUS_COMMAND_INCORRECT_STATUS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "Incorrect status! Type \"/status list\" to see list of valid statuses\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[STATUS_COMMAND_ALREADY_SET_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "You have already set this status!\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[STATUS_COMMAND_SUCCESS_CODE]) == 0 )
	{
		if ( response_tokens_size == 1 )
		{
			printf("%s", "\n                            ");
			printf("%s", "Your status has been successfully changed.\n");
		}
		else if ( response_tokens_size == 2 )
		{
			printf("%s", "\n                            ");
			printf("Your current status: %s\n", response_tokens[1]);
		}
		else if ( response_tokens_size > 2 )
		{
			printf("%s", "List of all valid statuses:\n");
			for ( int i = 1; i < response_tokens_size; i++ )
			{
				printf("- ");
				printf("%s", response_tokens[i]);
				printf("\n");
			}
			printf("%s", "----------------------\n");
		}

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[RECORD_COMMAND_SUCCESS_CODE]) == 0 )
	{
		char cur_time[CURRENT_TIME_SIZE];

		printf("%s", "\033c");
		printf("%s", "\n                            ");

		if ( strcmp(response_tokens[1], "debug") == 0 )
		{
			if ( !view_record_success_result(response_tokens, DEBUG_RECORD_FIELDS_NUM, 1) )
			{
				printf("[%s] %s: An error has occured while allocating memory to string buffers!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				return -1;
			}
		}
		else if ( strcmp(response_tokens[1], "record") == 0 )
		{
			if ( !view_record_success_result(response_tokens, USER_RECORD_FIELDS_NUM, 0) )
			{
				printf("[%s] %s: An error has occured while allocating memory to string buffers!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				return -1;
			}
		}

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[MUTE_COMMAND_USER_ALREADY_MUTED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s\n", "Unable to execute a command. User has already muted!");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[MUTE_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("User \"%s\" has been muted for %s seconds.\n", response_tokens[1], response_tokens[2]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[MUTE_COMMAND_YOU_MUTED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("You have been muted for %s seconds.\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[UNMUTE_COMMAND_USER_NOT_MUTED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("Unable to execute a command. User is already unmuted!\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[UNMUTE_COMMAND_SUCCESS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("User \"%s\" has been successfully unmuted.\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[UNMUTE_COMMAND_YOU_UNMUTED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "You have been unmuted.\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[KICK_COMMAND_SUCCESS_CODE]) == 0 )
	{
		if ( strcmp(response_tokens[1], "SENDER") == 0 )
		{
			printf("%s", "\n                            ");
			printf("User \"%s\" has been kicked from the chat.\n", response_tokens[2]);
		}
		else if ( strcmp(response_tokens[1], "VICTIM") == 0 )
		{
			printf("%s", "\033c");
			printf("%s", "\n                            ");
			printf("%s", "You have been kicked from the chat.\n");
		}

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[TABLE_COMMAND_SUCCESS_CODE]) == 0 )
	{
		int fields_count1 = atoi(response_tokens[1]);
		int fields_count2 = atoi(response_tokens[6]);
		if ( (fields_count1 < 1) || (fields_count2 < 1) || (response_tokens_size < 11) )
		{
			printf("%s", "\n                            ");
			printf("%s", "Unable extract field from response string properly!\n");

			return -1;
		}

		int i = 2;
		printf("------------------------------------------------------\n");
		printf("| %-4s | %-16s | %-20s | %-1s |\n", "ID", "Username", "Password", "R");
		printf("------------------------------------------------------\n");
		printf("| %-4s | %-16s | %-20s | %-1s |\n", response_tokens[i], response_tokens[i+1], response_tokens[i+2], response_tokens[i+3]);
		printf("------------------------------------------------------\n");
		i += fields_count1;

		printf("\n\n\n");

		printf("------------------------------------------------------\n");
		printf("| %-4s | %-25s | %-25s | %-25s | %-25s |\n", "ID", "Reg. Date", "Last In Date", "Last Out Date", "Last IP");
		printf("------------------------------------------------------\n");
		printf("| %-4s | %-25s | %-25s | %-25s | %-25s |\n", response_tokens[2], response_tokens[i+1], response_tokens[i+2], response_tokens[i+3], response_tokens[i+4]);
		printf("------------------------------------------------------\n");

		printf("\n\n\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[COMMAND_INVALID_PARAMS_CODE]) == 0 )
	{
		if (
				(strcmp(response_tokens[1], "CHGPWD") == 0)	||
				(strcmp(response_tokens[1], "KICK")   == 0)	||
				(strcmp(response_tokens[1], "MUTE")   == 0)	||
				(strcmp(response_tokens[1], "UNMUTE") == 0)	||
				(strcmp(response_tokens[1], "PM")     == 0)	||
				(strcmp(response_tokens[1], "DEOP")   == 0)	||
				(strcmp(response_tokens[1], "OP")     == 0)	||
				(strcmp(response_tokens[1], "RECORD") == 0)	||
				(strcmp(response_tokens[1], "STATUS") == 0)	||
				(strcmp(response_tokens[1], "TABLE")  == 0)	||
				(strcmp(response_tokens[1], "BAN")    == 0)	||
				(strcmp(response_tokens[1], "UNBAN")  == 0)
			)
		{
			printf("%s", "Incorrect command usage. ");

			if ( strcmp(response_tokens[2], subcommands_codes_list[TOO_MUCH_ARGS] ) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "Number of arguments too much or few than command needs.\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[SELF_USE]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "You can not apply this command to yourself!\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[INCORRECT_USERNAME]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "Check property of username.\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[USER_NOT_FOUND]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "User not found in database file. Is it registered?\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[USER_OFFLINE]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "Unable to execute a command. User is offline.\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[INCORRECT_TIME_VALUE]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "Time argument is not a number!\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[INCORRECT_TIME_RANGE]) == 0 )
			{
				printf("%s", "\n                            ");
				printf("%s", "Time value in invalid range.\n");

				return 1;
			}
			if ( strcmp(response_tokens[2], subcommands_codes_list[INCORRECT_STRING_VALUE]) == 0)
			{
				printf("%s", "\n                            ");
				printf("%s", "Your parameter has an incorrect value!\n");

				return 1;
			}
		}
		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[COMMAND_PARAMS_NO_NEED_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "This command should be executed without params.\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[COMMAND_NO_PERMS_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "You don't have permission to execute this command.\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[UNKNOWN_COMMAND_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "Unknown command. Type \"/help\" to see commands list.\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[NO_PERM_TO_CREATE_FILE_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "Unable to create file\n"
				"Do you have permission to create files in this folder?\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[INTERNAL_ERROR_CODE]) == 0 )
	{
		printf("%s", "\n                            ");
		printf("%s", "An internal error has occured on server. Try later.\n");

		return -1;
	}
	else if ( strcmp(response_tokens[0], server_codes_list[SUCCESSFULLY_AUTHORIZED_CODE]) == 0 )
	{
		printf("\033c");
		printf("%s", "\n                            ");
		printf("Welcome to GLOBAL chat room!\n");
		printf("%s", "\n                            ");
		printf("You authorized here as \"");
		printf("%s", response_tokens[1]);
		printf("\"\n");
		printf("%s", "\n                            ");
		printf("To start chatting, just type text with ending ENTER\n");
		printf("%s", "\n                            ");
		printf("Type \"/help\" to show valid commands for your group\n\n");

		*authorized = 1;
		return 1;
	}

	return 0;
}

#endif

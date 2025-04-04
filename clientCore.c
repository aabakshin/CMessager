#ifndef CLIENTCORE_C_SENTRY
#define CLIENTCORE_C_SENTRY

#include "Commons.h"
#include "clientCore.h"

enum
{
	MAX_STRING_LENGTH					=			52,
	HAS_ACCOUNT_VALUE_LENGTH			=			 3
};

/* Буфер предыдущих отпралвенных команд */
CommandsHistoryList* chl_list = NULL;

/* Всп. указатель для списка chl_list */
static CommandsHistoryList* cur_pos = NULL;

/* Конец ввода строки */
static int end_flag = 1;

extern int exit_flag;


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

static void show_logo(void)
{
	printf( "%s",  
					"   $$$$$$$$\\  $$$$$$\\  $$$$$$$\\        $$$$$$\\  $$\\   $$\\  $$$$$$\\ $$$$$$$$\\\n"
					"   \\__$$  __|$$  __$$\\ $$  __$$\\      $$  __$$\\ $$ |  $$ |$$  __$$\\\\__$$  __|\n"
					"      $$ |   $$ /  \\__|$$ |  $$ |     $$ /  \\__|$$ |  $$ |$$ /  $$ |  $$ |   \n"
					"      $$ |   $$ |      $$$$$$$  |     $$ |      $$$$$$$$ |$$$$$$$$ |  $$ |    \n"
					"      $$ |   $$ |      $$  ____/      $$ |      $$  __$$ |$$  __$$ |  $$ |    \n"
					"      $$ |   $$ |  $$\\ $$ |           $$ |  $$\\ $$ |  $$ |$$ |  $$ |  $$ |    \n"
					"      $$ |   \\$$$$$$  |$$ |           \\$$$$$$  |$$ |  $$ |$$ |  $$ |  $$ |    \n"
					"      \\__|    \\______/ \\__|            \\______/ \\__|  \\__|\\__|  \\__|  \\__|\n"
					"\n\n"
		  );
}

static void print_horizontal_line(int offset, int line_length, char char_line)
{
	int i;
	for ( i = 1; i <= offset; i++ )
		putchar(' ');

	for ( i = 1; i <= line_length; i++ )
		putchar(char_line);

    for ( i = 1; i <= offset; i++ )
		putchar(' ');
}

static void print_greeting_text_frame(const char** text_strings, int text_strings_size)
{
	print_horizontal_line(0, 14, ' ');
	printf("%s\n", "Welcome to authorization page of my simple TCP chat.");
	print_horizontal_line(14, MAX_STRING_LENGTH, '#');
	putchar('\n');

	int i;
	for ( i = 0; i < text_strings_size; i++ )
	{
		int len = strlen(text_strings[i]);
		
		if ( len > MAX_STRING_LENGTH-4 )
			len = MAX_STRING_LENGTH-4;

		print_horizontal_line(0, 14, ' ');
		print_horizontal_line(0, 2, '#');
		
		int is_odd = 0;
		int offset_len; 
		( ((offset_len = MAX_STRING_LENGTH-4-len) % 2) != 0 ) ? is_odd = 1 : is_odd; 
		offset_len /= 2;

		print_horizontal_line(0, offset_len, ' ');
		int j;
		for ( j = 0; j < len; j++ )
			putchar(text_strings[i][j]);
		( is_odd == 1 ) ? print_horizontal_line(0, offset_len+1, ' ') : print_horizontal_line(0, offset_len, ' ') ; 

		print_horizontal_line(0, 2, '#');
		putchar('\n');
	}

	print_horizontal_line(14, MAX_STRING_LENGTH, '#');
	putchar('\n');
	print_horizontal_line(0, 14, ' ');
	printf("%s", "Your answer: ");
}

static void send_answer(int peer_sock, const char** box_messages, int box_messages_size, int max_read_chars)
{
	/* Размер message должен быть как минимум max_read_chars+2 байт*/
	char message[100] = { 0 };
	int len = 0;

	do
	{
		clear_screen();
		show_logo();
		print_greeting_text_frame(box_messages, box_messages_size);
		len = get_str(message, max_read_chars);
		
		if ( len == EXIT_CODE )
		{
			exit_flag = 1;
			return;
		}
	}
	while ( len < 2 );

	if ( len > max_read_chars )
		clear_stdin();

	sendall(peer_sock, message, &len);
	printf("Sent %d bytes\n", len);
}

static int view_record_success_result(char** response_tokens, int fields_num, int debug_mode)
{
	char** args = malloc(sizeof(char*) * fields_num);
	if ( !args )
		return 0;

	int i, j, k;
	for ( i = 0, j = 3; i < fields_num; i++, j++ )
	{
		args[i] = malloc(sizeof(char) * strlen(response_tokens[j]) + 1 );
		if ( !(args[i]) )
		{
			int m;
			for (m = 0; m < i; m++)
				if ( args[m] )
					free(args[m]);
			return 0;
		}

		for ( k = 0; response_tokens[j][k]; k++ )
			args[i][k] = response_tokens[j][k];
		args[i][k] = '\0';
	}

	print_record(args, fields_num, debug_mode);
	
	for ( i = 0; i < fields_num; i++ )
		if ( args[i] )
			free(args[i]);

	if ( args )
		free(args);

	return 1;
}


/* 
 * Обработка терминального ввода в ручном режиме при помощи termios
 * Реализация некоторых возможностей терминала
 * Получение строки из станд.потока ввода через низкоуровненые функции и обработка
 * содержимого с учётом использования многобайтных символов
 */

int get_str(char* buffer, int buffer_size)
{
	if ( (buffer == NULL) || (buffer_size < 2) )
		return -1;

	char read_sym[10] = { 0 };
	int i = 0;
	int left_offset = 0;

	while ( 1 )
	{
		int rc = read(0, read_sym, 6);	/* 6 - макс. размер в байтах кода клавиши на клавиатуре(F1-F12) */
		if ( rc < 1 )
			continue;
		
		if ( rc == 1 )
		{
			if ( read_sym[0] == 3 ) /* Ctrl-C */
			{
				/* завершение программы */
				return EXIT_CODE;
			}
			else if ( (read_sym[0] == 4) || (read_sym[0] == '\n') ) /* 4 => EOF или Ctrl-D */
			{
				end_flag = 1;
				write(1, &read_sym[0], 1);

				if ( i < buffer_size-1 )
				{
					buffer[i] = '\n';
					i++;
					buffer[i] = '\0';
					break;
				}
				buffer[buffer_size-2] = '\n';
				buffer[buffer_size-1] = '\0';
				
				i = buffer_size-1;
				
				break;
			}
			else if ( (read_sym[0] == '\b') || (read_sym[0] == 127) )	/* Обработка backspace */
			{
				if ( left_offset < 1 )
				{
					if ( i > 0 )
						i--;
					printf("%s", "\b \b");
					fflush(stdout);
				}
				else
				{
					char buf[buffer_size];
					int x, z = 0;

					i -= left_offset;
					if ( i < 1 )
					{
						i += left_offset;
						continue;
					}
					
					for ( x = i; x < i+left_offset; x++, z++ )
						buf[z] = buffer[x];
					buf[z] = '\0';

					i--;
					for ( x = 0; buf[x]; x++ )
					{
						buffer[i] = buf[x];
						i++;
					}

					int shift = 0;
					putchar('\b');
					for ( x = 0; buf[x]; x++ )
					{
						putchar(buf[x]);
						shift++;
					}
					putchar(' ');
					shift++;
					for ( x = 1; x <= shift; x++ )
						putchar('\b');
					fflush(stdout);
				}

				continue;
			}
			else if ( read_sym[0] == 23 ) /* Ctrl-W удаление последнего слова */
			{
				if ( i < 1 )
					continue;

				int last_ch = i-1;
				int cur_pos = i-left_offset;
				int pos = cur_pos;

				char buf[buffer_size];
				if ( cur_pos > 0 )
				{
					if ( buffer[cur_pos-1] == ' ' )
						while ( (cur_pos > 0) && (buffer[cur_pos-1] == ' ')  )
							cur_pos--;

					if ( cur_pos > 0 )
						while ( (cur_pos > 0) && (buffer[cur_pos-1] != ' ') )
							cur_pos--;

					i = cur_pos;
					int save_i = i;

					int k;
					for ( k = 1; k <= (pos-cur_pos); k++ )
					{
						printf("\b \b");
						fflush(stdout);
					}

					int x = 0;
					for ( k = pos; k <= last_ch; k++ )
					{
						buf[x] = buffer[k];
						x++;
					}
					buf[x] = '\0';

					for ( x = 0; buf[x]; x++ )
					{
						buffer[i] = buf[x];
						putchar(buffer[i]);
						i++;
					}
					fflush(stdout);

					if ( buf[0] != '\0' )
					{
						for ( x = i; x <= last_ch; x++ )
							putchar(' ' );
						for ( ; x > save_i; x-- )
							putchar('\b');
						fflush(stdout);
					}
				}
				continue;
			}
			
			int spec_flag = 0;
			int save_pos = 0; 

			if ( left_offset > 0 )
			{
				spec_flag = 1;
				char buf[buffer_size];
				int x, j = 0;
				int last_ch = i-1;

				i -= left_offset;
				save_pos = i;
				for ( x = i; x <= last_ch; x++ )
				{
					buf[j] = buffer[x];
					j++;
				}
				buf[j] = '\0';

				buffer[i] = read_sym[0];
				i++;
				
				for ( x = 0; buf[x]; x++ )
				{
					if ( i < buffer_size-1 )
					{
						buffer[i] = buf[x];
						i++;
					}
					else
						break;
				}
			}
			else
			{
				buffer[i] = read_sym[0];
				i++;
			}

			if ( i > buffer_size-2 )
			{
				buffer[buffer_size-2] = '\n';
				buffer[buffer_size-1] = '\0';
				
				i = buffer_size-1;

				break;
			}
			
			
			if ( spec_flag )
			{
				spec_flag = 0;
				int l_char = i-1;
				int cur_pos = i;
				while ( cur_pos >= 0 )
				{
					putchar('\b');
					fflush(stdout);
					cur_pos--;
				}
				for ( cur_pos = 0; cur_pos <= l_char; cur_pos++ )
					write(1, &buffer[cur_pos], 1);

				while ( cur_pos > save_pos+1 )
				{
					putchar('\b');
					fflush(stdout);
					cur_pos--;
				}
			}
			else
			{
				write(1, &read_sym[0], 1);
			}
		}
		else if ( rc == 3 )
		{
			/* обработка клавиши ARROW_LEFT с 3-байтным кодом */
			if (
						( read_sym[0] == 0x1b )		&&			/* 27 */
						( read_sym[1] == 0x5b )		&&			/* 91 */
						( read_sym[2] == 0x44 )					/* 68 */
			   )
			{
				if ( left_offset < i )
				{
					putchar('\b');
					fflush(stdout);
					left_offset++;
				}
			}

			/* обработка клавиши ARROW_RIGHT с 3-байтным кодом */
			else if (
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x43 )				/* 67 */
					)
			{
				if ( left_offset > 0 )
				{
					putchar(' ');
					putchar('\b');
					putchar(buffer[i-left_offset]);
					left_offset--;
				}
				fflush(stdout);
			}
			
			/* обработка клавиши ARROW_UP с 3-байтным кодом */
			else if (     
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x41 )				/* 65 */
					)
			{
					if ( chl_list != NULL )
					{
						if ( i > 0 )
						{
							i--;		
							while ( i >= 0 )
							{
								buffer[i] = 0;
								printf("%s", "\b \b");
								i--;
							}
							i = 0;
							fflush(stdout);
						}

						
						if ( end_flag )
						{
							end_flag = 0;
							if ( chl_list->prev != NULL)
								cur_pos = chl_list->prev;
							else
								cur_pos = chl_list;							
						}
						else
						{
							if ( cur_pos->prev != NULL )
								cur_pos = cur_pos->prev;
						}

						int j;
						for ( j = 0; cur_pos->command[j] && (j < buffer_size-2); j++, i++ )
							buffer[i] = cur_pos->command[j];
						

						printf("%s", cur_pos->command);
						fflush(stdout);
					}
			}

			/* обработка клавиши ARROW_DOWN с 3-х байтным кодом */
			else if (     
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x42 )				/* 66 */
					)
			{
					if ( chl_list != NULL )
					{
						if ( i > 0 )
						{
							i--;		
							while ( i >= 0 )
							{
								buffer[i] = 0;
								printf("%s", "\b \b");
								i--;
							}
							i = 0;
							fflush(stdout);
						}

						
						if ( end_flag )
						{
							end_flag = 0;
							cur_pos = chl_list;
						}
						else
						{
							if ( cur_pos->next != NULL )
								cur_pos = cur_pos->next;
						}
						
						int j;
						for ( j = 0; cur_pos->command[j] && (j < buffer_size-2); j++, i++ )
							buffer[i] = cur_pos->command[j];
						

						printf("%s", cur_pos->command);
						fflush(stdout);
					}
			}
		}
		else if ( rc == 4 )
		{
			/* обработка клавиши DEL с 4-х байтным кодом */
			if (
						( read_sym[0] == 0x1b )		&&		/* 27 */
						( read_sym[1] == 0x5b )		&&		/* 91 */
						( read_sym[2] == 0x33 )		&&		/* 51 */
						( read_sym[3] == 0x7e )				/* 126 */
			   )
			{
				if ( left_offset > 0 )
				{
					char buf[buffer_size];
					int last_ch = i-1;
					int cur_pos = i-left_offset;
					int k;
					int x = 0;
					for ( k = cur_pos+1; k <= last_ch; k++ )
					{
						buf[x] = buffer[k];
						x++;
					}
					buf[x] = '\0';

					x = 0;
					for ( k = cur_pos; buf[x]; x++, k++ )
					{
						buffer[k] = buf[x];
						putchar(buffer[k]);
					}
					putchar(' ');
					fflush(stdout);

					for ( ; k >= cur_pos; k-- )
						putchar('\b');
					fflush(stdout);

					if ( i > 0 )
						i--;
					left_offset--;
				}
			}
		}
	}

	return i;
}

/* Обработка полученной информации от сервера в соответствии с протоколом общения */
int check_server_response(int peer_sock, char **response_tokens, int response_tokens_size, int* authorized)
{
	if ( strcmp(response_tokens[0], "*CLIENT_HAS_ACCOUNT") == 0 )
	{
		int max_read_chars = HAS_ACCOUNT_VALUE_LENGTH;
		const char* box_messages[] = { "Have you already have an account?", "Enter \"y\"or\"n\"", NULL };

		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_WAIT_LOGIN") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "Enter nickname for your account", NULL };
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_ALREADY_AUTHORIZED") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = { "This account is already authorized", "Try to use another login", NULL };
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_ALREADY_USED") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = {"This login is already exist in database", "Try another one",  NULL };
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_INCORRECT") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = {"Incorrect login!", "Please, check it and try again", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_NOT_EXIST") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = {"This login doesn't exist", "Check your input string", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*SIGNUP_WAIT_LOGIN") == 0 )
	{
		int max_read_chars = MAX_LOGIN_LENGTH;
		const char* box_messages[] = {"Enter nickname to create", "new account", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*LOGIN_WAIT_PASS") == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = {"Enter password for your account", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*NEW_PASS_INCORRECT") == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = {"Password incorrect!", "Check it and try again", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*PASS_NOT_MATCH") == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = {"Password doesn't match with this account", "Try again", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*SIGNUP_WAIT_PASS") == 0 )
	{
		int max_read_chars = MAX_PASS_LENGTH;
		const char* box_messages[] = {"Enter pass for your new account", NULL};
		
		int box_messages_size = 0;
		while ( box_messages[box_messages_size] )
			box_messages_size++;

		send_answer(peer_sock, box_messages, box_messages_size, max_read_chars);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*USER_AUTHORIZED") == 0 )
	{
		putchar('\n');
		int i;
		for ( i = 1; i <= 28; i++ )
			putchar(' ');
		printf("\"%s\" joined to GLOBAL chat\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*USER_LEFT_CHAT") == 0 )
	{
		putchar('\n');
		int i;
		for ( i = 1; i <= 28; i++ )
			printf("%c", ' ');
		printf("\"%s\" left GLOBAL chat\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*CANNOT_CONNECT_DATABASE") == 0 )
	{
		clear_screen();
		fprintf(stderr, "Server cannot connect to database file. Try later\n");

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*CMD_ARG_OVERLIMIT_LENGTH") == 0 )
	{
		fprintf(stderr, "Some command or argument in your input is too long! Max length is %s\n", response_tokens[1]);

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*HELP_COMMAND_SUCCESS") == 0 )
	{
		printf("%s\n", "--- List of all valid commands ---");
		int k;
		for ( k = 1; k < response_tokens_size; k++ )
			printf("[%d]: %s\n", k, response_tokens[k]);
		printf("%s\n", "----------------------------------");

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*WHOIH_COMMAND_SUCCESS") == 0 )
	{
		int users_counter = response_tokens_size-1;

		if ( users_counter >= 1 )
		{
			printf("%s", "There ");
			(users_counter > 1) ? printf("are %d users online:\n", users_counter) : printf("is %d user online:\n", users_counter);
			
			int i;
			for ( i = 1; i <= users_counter; i++ )
				printf("%s\n", response_tokens[i]);
			printf("%s\n", "------------------------");
		}
		else
			printf("%s\n", "There is nobody online :(");

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*CHGPWD_COMMAND_SUCCESS") == 0 )
	{
		printf("%s\n", "Your password has been successfully changed");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*CHGPWD_COMMAND_INCORRECT_PASS") == 0 )
	{
		printf("%s\n",
			   "Incorrect password. Try to follow next rules:\n"
			   "Password must be not less than 4 symbols\n"
			   "Password must be not more than 20 symbols\n"
			   "Password has to consist correct symbols(a-zA-Z0-9 and special chars)"
			  );

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*DEOP_COMMAND_SUCCESS") == 0 )
	{
		printf("%s\n", "User has been removed from Admin group");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*DEOP_COMMAND_USER_ALREADY_USER") == 0 )
	{
		printf("%s\n", "This user is not an administrator\n"
				       "or has been removed from admin's group earlier");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*OP_COMMAND_SUCCESS") == 0 )
	{
		printf("%s\n", "User has been added to Admin's group");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*OP_COMMAND_USER_ALREADY_ADMIN") == 0 )
	{
		printf("%s\n", "User is already in Admin's group");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*STATUS_COMMAND_INCORRECT_STATUS") == 0 )
	{
		printf("%s\n", "Incorrect status! Type \"/status list\" to see list of valid statuses");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*STATUS_COMMAND_ALREADY_SET") == 0 )
	{
		printf("%s\n", "You have already set this status!");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*STATUS_COMMAND_SUCCESS") == 0 )
	{
		if ( response_tokens_size == 1 )
			printf("%s\n", "Your status has been successfully changed");
		else if ( response_tokens_size == 2 )
			printf("Your current status: %s\n", response_tokens[1]);
		else if ( response_tokens_size > 2 )
		{
			printf("%s\n", "List of all valid statuses:");
			int i;
			for ( i = 1; i < response_tokens_size; i++ )
				printf("- %s\n", response_tokens[i]);
			printf("%s\n", "----------------------");
		}

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*RECORD_COMMAND_SUCCESS") == 0 )
	{
		if ( strcmp(response_tokens[1], "debug") == 0 )
		{
			if ( !view_record_success_result(response_tokens, DEBUG_RECORD_FIELDS_NUM, 1) )
				printf("%s\n", "[ERROR]: An error has occured while allocating memory to string buffers!");
		}
		else if ( strcmp(response_tokens[1], "record") == 0 )
		{
			if ( !view_record_success_result(response_tokens, USER_RECORD_FIELDS_NUM, 0) )
				printf("%s\n", "[ERROR]: An error has occured while allocating memory to string buffers!");
		}

		return 1;
	}
	else if ( strcmp(response_tokens[0], "*MUTE_COMMAND_USER_ALREADY_MUTED") == 0 )
	{
		printf("%s\n", "Unable to execute a command. User has already muted!");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*MUTE_COMMAND_SUCCESS") == 0 )
	{
		printf("User \"%s\" has been muted for %s seconds.\n", response_tokens[1], response_tokens[2]);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*MUTE_COMMAND_YOU_MUTED") == 0 )
	{
		printf("You have been muted for %s seconds.\n", response_tokens[1]);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*UNMUTE_COMMAND_USER_NOT_MUTED") == 0 )
	{
		printf("%s\n", "Unable to execute a command. User is already unmuted!");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*UNMUTE_COMMAND_SUCCESS") == 0 )
	{
		printf("User \"%s\" has been successfully unmuted.\n", response_tokens[1]);
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*UNMUTE_COMMAND_YOU_UNMUTED") == 0 )
	{
		printf("%s\n", "You have been unmuted.");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*KICK_COMMAND_SUCCESS") == 0 )
	{
		if ( strcmp(response_tokens[1], "SENDER") == 0 )
		{
			printf("User \"%s\" has been kicked from the chat.\n", response_tokens[2]);
		}
		else if ( strcmp(response_tokens[1], "VICTIM") == 0 )
		{
			clear_screen();
			printf("%s\n", "You have been kicked from the chat.");
		}
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*TABLE_COMMAND_SUCCESS") == 0 )
	{
		/*
		int z;
		printf("%s\n", "DEBUG----------------------------------------DEBUG");
		for (z = 0; z < response_tokens_size; z++)
		{
			printf("\t\t%s\n", response_tokens[z]);
		}
		printf("%s\n", "DEBUG----------------------------------------DEBUG");
		*/

		if ( strcmp(response_tokens[1], "LIST") == 0 )
		{
			printf("%s\n", "List of available tables:");
			int i;
			for ( i = 2; i < response_tokens_size; i++ )
				printf("[%d]: %s\n", i-1, response_tokens[i]);
			printf("%s\n", "-------------------------");
			return 1;
		}
		
		if ( strcmp(response_tokens[1], "DATA") == 0 )
		{
			int non_empty_records_size = atoi(response_tokens[3]);

			if ( strcmp(response_tokens[4], "USERINFO") == 0 )
			{
				printf(
					   "\n%s\n"
					   "%s\n"
					   "| %-4s | %-16s | %-20s | %-1s |\n"
					   "%s\n",
					   "File \"usersdata.dat\":",
					   "------------------------------------------------------",
					   "ID", "Username", "Password", "R",
					   "------------------------------------------------------"
					  );
				
				int i;
				for ( i = 5; i < ( 5 * non_empty_records_size); i += 4 )
				{
					printf("| %-4s | %-16s | %-20s | %-1s |\n", response_tokens[i], response_tokens[i+1], response_tokens[i+2], response_tokens[i+3]);
					printf("%s\n", "------------------------------------------------------");
				}
			}
			else if ( strcmp(response_tokens[4], "XUSERINFO") == 0 )
			{
				printf(
						"\n%s\n" 
					    "%s\n"
						"| %-4s | %-25s | %-25s | %-25s | %-25s |\n"
						"%s\n",
						"File \"users_sessions_info.dat\":",
						"------------------------------------------------------------------------------------------------------------------------",
						"ID", "Reg. Date", "Last In Date", "Last Out Date", "Last IP",
						"------------------------------------------------------------------------------------------------------------------------"
					  );
				
				int i;
				for ( i = 5; i <= (5 * non_empty_records_size); i += 5 )
				{
					printf("| %-4s | %-25s | %-25s | %-25s | %-25s |\n", response_tokens[i], response_tokens[i+1], response_tokens[i+2], response_tokens[i+3], response_tokens[i+4]);
					printf("%s\n", "------------------------------------------------------------------------------------------------------------------------");
				}

			}

			return 1;
		}
		printf("%s\n", "[ERROR]: An internal error has occured while executing this command. Contact with admin.");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*COMMAND_INVALID_PARAMS") == 0 )
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

			if ( strcmp(response_tokens[2], "TOO_MUCH_ARGS" ) == 0 )
			{
				printf("%s\n", "Number of arguments too much or few than command needs.");
				return 1;
			}
			if ( strcmp(response_tokens[2], "SELF_USE") == 0 )
			{
				printf("%s\n", "You can not apply this command to yourself!");
				return 1;
			}
			if ( strcmp(response_tokens[2], "INCORRECT_USERNAME") == 0 )
			{
				printf("%s\n", "Check property of username");
				return 1;
			}
			if ( strcmp(response_tokens[2], "USER_NOT_FOUND") == 0 )
			{
				printf("%s\n", "User not found in database file. Is it registered?");
				return 1;
			}
			if ( strcmp(response_tokens[2], "USER_OFFLINE") == 0 )
			{
				printf("%s\n", "Unable to execute a command. User is offline.");
				return 1;
			}
			if ( strcmp(response_tokens[2], "INCORRECT_TIME_VALUE") == 0 )
			{
				printf("%s\n", "Time argument is not a number!");
				return 1;
			}
			if ( strcmp(response_tokens[2], "INCORRECT_TIME_RANGE") == 0 )
			{
				printf("%s\n", "Time value in invalid range.");
				return 1;
			}
			if ( strcmp(response_tokens[2], "INCORRECT_STRING_VALUE") == 0)
			{
				printf("%s\n", "Your parameter has an incorrect value!");
				return 1;
			}
		}
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*COMMAND_PARAMS_NO_NEED") == 0 )
	{
		printf("%s\n", "This command should be executed without params");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*COMMAND_NO_PERMS") == 0 )
	{
		printf("%s\n", "You don't have permission to execute this command");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*UNKNOWN_COMMAND") == 0 )
	{
		printf("%s\n", "Unknown command. Type /help to see commands list");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*NO_PERM_TO_CREATE_FILE") == 0 )
	{
		printf("%s\n", "Unable to create file\n"
				       "Do you have permission to create files in this folder?");
		return 1;
	}
	else if ( strcmp(response_tokens[0], "*SUCCESSFULLY_AUTHORIZED") == 0 )
	{
		clear_screen();
		printf("%s\n%s%s%s\n%s\n", "Welcome to GLOBAL chat room!",
				                   "You authorized here as \"", response_tokens[1],
								   "\"\nTo start chatting, just type text with ending ENTER",
								   "Type /help to show valid commands for your group"
			  );

		*authorized = 1;
		return 1;
	}

	return 0;
}

#endif

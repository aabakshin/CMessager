#ifndef COMMONS_C_SENTRY
#define COMMONS_C_SENTRY

#include "Commons.h"
#include "DateTime.h"

enum
{
	SEC_MILLISEC_MULTIPLIER			=			1000,
	NSEC_MILLISEC_DELIMETER			=		 1000000,
	VALID_SYMBOLS_NUM				=			  62,
	MAX_TOKENS_NUM					=			 341,	/* если BUFSIZE = 1024 */
	PORT_BUF_SIZE					=			   6,
	IP_PART_SIZE					=			   4,
	MAX_ADDRESS_STRING_SIZE			=			  22,
	SHIFT_VALUE						=			  24,	/* Величина битового сдвига для 32-битный целых */
	BYTE							=			   8
};

const char* server_codes_list[SERVER_CODES_COUNT] = {
									"*CANNOT_CONNECT_DATABASE",
									"*CHGPWD_COMMAND_SUCCESS",
									"*CHGPWD_COMMAND_INCORRECT_PASS",
									"*CLIENT_HAS_ACCOUNT",
									"*CMD_ARG_OVERLIMIT_LENGTH",
									"*COMMAND_INVALID_PARAMS",
									"*COMMAND_NO_PERMS",
									"*COMMAND_PARAMS_NO_NEED",
									"*DEOP_COMMAND_SUCCESS",
									"*DEOP_COMMAND_USER_ALREADY_USER",
									"*HELP_COMMAND_SUCCESS",
									"*KICK_COMMAND_SUCCESS",
									"*LOGIN_WAIT_LOGIN",
									"*LOGIN_ALREADY_AUTHORIZED",
									"*LOGIN_ALREADY_USED",
									"*LOGIN_INCORRECT",
									"*LOGIN_NOT_EXIST",
									"*LOGIN_WAIT_PASS",
									"*MUTE_COMMAND_USER_ALREADY_MUTED",
									"*MUTE_COMMAND_SUCCESS",
									"*MUTE_COMMAND_YOU_MUTED",
									"*NEW_PASS_INCORRECT",
									"*NO_PERM_TO_CREATE_FILE",
									"*OP_COMMAND_SUCCESS",
									"*OP_COMMAND_USER_ALREADY_ADMIN",
									"*PASS_NOT_MATCH",
									"*RECORD_COMMAND_SUCCESS",
									"*SIGNUP_WAIT_LOGIN",
									"*SIGNUP_WAIT_PASS",
									"*STATUS_COMMAND_INCORRECT_STATUS",
									"*STATUS_COMMAND_ALREADY_SET",
									"*STATUS_COMMAND_SUCCESS",
									"*SUCCESSFULLY_AUTHORIZED",
									"*TABLE_COMMAND_SUCCESS",
									"*UNKNOWN_COMMAND",
									"*UNMUTE_COMMAND_USER_NOT_MUTED",
									"*UNMUTE_COMMAND_SUCCESS",
									"*UNMUTE_COMMAND_YOU_UNMUTED",
									"*USER_LEFT_CHAT",
									"*USER_AUTHORIZED",
									"*WHOIH_COMMAND_SUCCESS"
								};

/* Записывает в буфер первые max_target_size-2 символа */
int get_string(char* target_buf, int max_target_size)
{
	int ch;
	int i = 0;

	do
	{
		ch = fgetc(stdin);

		if ( (ch == '\n') || (ch == EOF) )
			break;
		
		if ( i < max_target_size-2 )
		{
			target_buf[i] = ch;
			i++;
		}
		else
		{
			break;
		}
	}
	while ( (ch != '\n') && (ch != EOF) );

	if ( i > 0 )
	{
		target_buf[i] = '\n';
		i++;

		target_buf[i] = '\0';

		return i;
	}

	return 0;
}

int sendall(int s, const char* buf, int* buf_size)
{
	int total = 0;
	int bytesleft = *buf_size;
	int n;

	while ( total < *buf_size )
	{
		n = send(s, buf+total, bytesleft, 0);
		if ( n == -1 ) 
			break;
		total += n;
		bytesleft -= n;
	}
	*buf_size = total;

	return n == -1 ? -1 : 0;
}

char* get_code(void)
{
	const char* symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	
	char* buf = malloc(CAPTCHA_CODE_LENGTH+1);
	if ( !buf )
	{
		fprintf(stderr, "%s", "\n[Commons]: In function \"get_code\" memory error\n");
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
	/* Удаляет пробела из начала сообщения */
	///////////////////////////////////////////////////////////
	int i = 0, c = 0;

	while ( read[i++] == ' ' ) 
		c++;

	for ( i = c; i < read_size; i++ ) 
		read[i-c] = read[i];
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
				
	int k = 0;
	int j;
	for ( i = 0; i < size; i++ )
	{
		for ( j = 0; j < strlen(message_tokens[i]); j++ )
			read[k++] = message_tokens[i][j];
		read[k++] = ' ';
	}
	read[k-1] = '\0';
}

unsigned long long get_tick_unix(void)
{
	struct timespec cur_time;
	clock_gettime(CLOCK_REALTIME, &cur_time);

	return (cur_time.tv_sec*SEC_MILLISEC_MULTIPLIER + cur_time.tv_nsec/NSEC_MILLISEC_DELIMETER);
}

void clear_screen(void)
{
	int i;
	for (i = 1; i <= 10; i++)
		printf("%s", "\n\n\n\n\n\n\n\n\n\n");
}

void clear_stdin(void)
{
	int c;
	do
	{
		c = getchar();
	}
	while ( (c != EOF) && (c != '\n') );
}

void print_record(char** args, int args_size, int debug_mode)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( debug_mode )
	{
		if ( args_size != DEBUG_RECORD_FIELDS_NUM )
		{
			fprintf(stderr, "[%s] %s Unexpected behaviour has occured while printing record\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return;
		}

		printf("%s"
				"%40s%-34s|\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %1d %-50s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-52s |\n"
				"%s\n"
				"| %17s | %-52s |\n"
				"%s\n"
				"| %17s | %-52s |\n"
				"%s\n"
				"| %17s | %-52s |\n"
				"%s\n"
				"| %17s | %-52s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-4d %-47s |\n"
				"%s\n"
				"| %17s | %-15d %-36s |\n"
				"\\%74s/\n",
							"/--------------------------------------------------------------------------\\\n|",
															args[0], " record",
							"|--------------------------------------------------------------------------|",
															"ID", atoi(args[1]), " ",
							"|--------------------------------------------------------------------------|",
															"Authorized", atoi(args[2]), " ",
							"|--------------------------------------------------------------------------|",
															"Buffer used", atoi(args[3]), " ",
							"|--------------------------------------------------------------------------|",
															"Last Date In", args[4],
							"|--------------------------------------------------------------------------|",
															"Last IP", args[5],
							"|--------------------------------------------------------------------------|",
															"Reg. Date", args[6],
							"|--------------------------------------------------------------------------|",
															"Login", args[7],
							"|--------------------------------------------------------------------------|",
															"Pass", args[8],
							"|--------------------------------------------------------------------------|",
															"Rank", atoi(args[9]), " ",
							"|--------------------------------------------------------------------------|",
															"Socket number", atoi(args[10]), " ",
							"|--------------------------------------------------------------------------|",
															"State", atoi(args[11]), " ",
							"|--------------------------------------------------------------------------|",
															"Status", atoi(args[12]), " ",
							"|--------------------------------------------------------------------------|",
															"Is muted", atoi(args[13]), " ",
							"|--------------------------------------------------------------------------|",
															"Total mute time", atoi(args[14]), " ",
							"|--------------------------------------------------------------------------|",
															"Mute time left", atoi(args[15])," ",
							"|--------------------------------------------------------------------------|",
															"Start mute time", atoi(args[16]), " ",
							 "--------------------------------------------------------------------------"
			  );
		return;
	}
	

	if ( args_size != USER_RECORD_FIELDS_NUM )
	{
		fprintf(stderr, "[%s] [ERROR]: Unexpected behaviour has occured while printing record\n", get_time_str(cur_time, MAX_TIME_STR_SIZE));
		return;
	}

	printf("%s"
		   "%40s%-34s|\n"
		   "%s\n"
		   "| %17s | %-52s |\n"
		   "%s\n"
		   "| %17s | %-52s |\n"
		   "%s\n"
		   "| %17s | %-52s |\n"
		   "%s\n"
		   "| %17s | %-52s |\n"
		   "%s\n"
		   "| %17s | %-52s |\n"
		   "%s\n"
		   "| %-72s |\n"
		   "\\%74s/\n",
						"/--------------------------------------------------------------------------\\\n|",
														args[0], " record",
						"|--------------------------------------------------------------------------|",
														"Status", args[1],
						"|--------------------------------------------------------------------------|",
														"Rank", args[2],
						"|--------------------------------------------------------------------------|",
														"Real Name", args[3],
						"|--------------------------------------------------------------------------|",
														"Age", args[4],
						"|--------------------------------------------------------------------------|",
														"Registration Date", args[5],
						"|--------------------------------------------------------------------------|",
																args[6],
						 "--------------------------------------------------------------------------"
		  );
}

static void reverse(char* s)
{
	int i = 0;
	int j = strlen(s)-1;

	for ( ; i < j; i++, j-- )
	{
		char c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static int num_digit_cnt(int number)
{
	int counter = 0;
	
	if ( number == 0 )
		return 1;

	if ( number < 0 )
		number *= -1;

	while ( number > 0 )
	{
		counter++;
		number /= 10;
	}

	return counter;
}

void itoa(int number, char* num_buf, int max_buf_len)
{
	if ( number == 0 )
	{
		num_buf[0] = '0';
		num_buf[1] = '\0';
		return;
	}

	int cnt = num_digit_cnt(number);

	if ( cnt > (max_buf_len-1) )
		cnt = max_buf_len-1;
	
	int flag = 0;
	if ( number < 0 )
	{
		number *= -1;
		flag = 1;
	}
	
	int i = 0;
	while ( (number > 0) && (i < cnt) )
	{
		num_buf[i] = (number % 10) + '0';
		number /= 10;
		i++;
	}

	if ( flag )
	{
		num_buf[i] = '-';
		i++;
	}
	num_buf[i] = '\0';

	reverse(num_buf);
}

char* concat_addr_port(unsigned long ip, unsigned long port)
{
	char* result = malloc(MAX_ADDRESS_STRING_SIZE);
	if ( !result )
		return NULL;

	int i = 0;
	int cur_pos = 0;
	int shift = SHIFT_VALUE;

	while ( shift >= 0 )
	{
		int j = 0;
		char ip_buf[IP_PART_SIZE];
		int ip_part = (ip >> shift & 0xFF);
		shift -= BYTE;
		itoa(ip_part, ip_buf, IP_PART_SIZE);
		int ip_buf_len = strlen(ip_buf);
		for ( ; i < cur_pos+ip_buf_len; i++ )
		{
			result[i] = ip_buf[j];
			j++;
		}
		result[i] = '.';
		i++;
		cur_pos = i;
	}
	result[cur_pos-1] = ':'; /* Перезаписываем символ . на : */

	char port_buf[PORT_BUF_SIZE];
	itoa(port, port_buf, PORT_BUF_SIZE);
	int port_buf_len = strlen(port_buf);

	int j = 0;
	for ( ; i < cur_pos+port_buf_len; i++ )
	{
		result[i] = port_buf[j];
		j++;
	}
	result[i] = '\0';

	return result;
}
int check_client_answer(const char *answer)
{
	const char* check_for[] = {
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

	int i;
	for (i = 0; check_for[i]; i++)
		if (strcmp(answer, check_for[i]) == 0)
			return 1;

	return 0;
}

#endif

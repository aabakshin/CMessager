#include "../../src/Commons.h"
#include "../../src/clientCore.h"
#include "../../src/Input.h"
#include "../../src/CommandsHistoryList.h"


/* Буфер предыдущих отправленных команд/сообщений */
CommandsHistoryList* chl_list = NULL;

/* Флаг выхода из программы */
static int exit_flag = 0;


void exit_handler(int signo)
{
	int save_errno = errno;
	signal(SIGINT, exit_handler);

	exit_flag = 1;

	errno = save_errno;
}


int main(int argc, char** argv)
{
	signal(SIGINT, exit_handler);

	int authorized = 0;
	int messages_counter = 0;
	int start_signal = 0;
	time_t start_time = 0;
	time_t total_time = ANTISPAM_MODULE_TOTAL_TIME_MS;


	printf("\033c");     /* work on VT100 terminal series only */

	if ( argc != 3 )
	{
		fprintf(stderr, "%s", "\nIncorrect arguments!\nUsage: <program_name> <ip/hostname> <port>\n");
		return 1;
	}

	int peer_sock = client_init(argv[1], argv[2]);
	if ( peer_sock == -1 )
	{
		fprintf(stderr, "%s", "\nAn error has occured while executing initial procedure\n");
		return 1;
	}


	srand(time(0));


	while ( 1 )
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(peer_sock, &readfds);
		FD_SET(0, &readfds);

		struct timeval timeout;
		timeout.tv_sec = SELECT_TIMER_SEC;
		timeout.tv_usec = SELECT_TIMER_MSEC * 1000;

		int res = -1;
		res = select(peer_sock+1, &readfds, 0, 0, &timeout);
		if ( res == -1 )
		{
			if ( errno == EINTR )
			{
				if ( exit_flag )
					client_close_connection(peer_sock);

				//fprintf(stderr, "%s", "\nGot some signal.\n");
				continue;
			}

			fprintf(stderr, "\nselect() failed. {%d}\n", errno);
			continue;
		}


		if ( FD_ISSET(peer_sock, &readfds) )
		{
			char* response_tokens[MAX_RESPONSE_ARGS_NUM];

			for ( int i = 0; i < MAX_RESPONSE_ARGS_NUM; i++ )
				response_tokens[i] = NULL;

			char read_buf[BUFSIZE] = { 0 };
			int bytes_received = recv(peer_sock, read_buf, BUFSIZE, 0);
			printf("\nReceived %d bytes\n", bytes_received);

			if ( bytes_received <= 1 )
			{
				fprintf(stderr, "\nConnection closed by server.\n");
				client_close_connection(peer_sock);
			}

			for ( int i = 0; i < bytes_received; i++ )
			{
				if ( read_buf[i] == '\n' )
				{
					read_buf[i] = '\0';
					break;
				}
			}

			restrict_message_length(read_buf, 1000);

			char* istr = strtok(read_buf, "|");
			int i = 0;
			while ( istr )
			{
				response_tokens[i] = istr;
				i++;
				istr = strtok(NULL, "|");
			}
			int response_tokens_size = i;

			if ( !check_server_response(peer_sock, response_tokens, response_tokens_size, &authorized) )
			{
				client_close_connection(peer_sock);
			}
		}


		if ( authorized && FD_ISSET(0, &readfds) )
		{
			char send_buf[BUFSIZE] = { 0 };
			do
			{
				int str_len = input(send_buf, BUFSIZE);
				printf("\nstr_len = %d\n", str_len);
				if ( str_len < 1 )
				{
					if ( str_len == EXIT_CODE )
					{
						exit_flag = 1;
						break;
					}
					continue;
				}

				if ( str_len > 1 )
					delete_extra_spaces(send_buf, str_len+1);
			}
			while ( (send_buf[0] == '\n') || (send_buf[0] == '\0') );

			if ( exit_flag )
				client_close_connection(peer_sock);

			int sent_bytes = restrict_message_length(send_buf, MAX_MESSAGE_LENGTH);



			/*								анти-спам модуль								*/

			long long timestamps[2] = { 0 };
			long long interval;
			int x = 0;

			if ( !start_signal )
			{
				start_time = get_tick_unix();
				start_signal = 1;
			}

			timestamps[(x%2)] = get_tick_unix();
			if ( x < 1 )
				x++;
			else
				x = 0;

			interval = timestamps[1] - timestamps[0];
			if ( interval < 0 )
				interval *= -1;


			sendall(peer_sock, send_buf, &sent_bytes);
			printf("Sent %d bytes\n\n", sent_bytes);
			++messages_counter;


			int len = strlen(send_buf);
			if ( send_buf[len-1] == '\n' )
				send_buf[len-1] = '\0';

			chl_insert(&chl_list, send_buf, len+1);
			int chl_size = chl_get_size(chl_list);

			if ( chl_size > HISTORY_COMMANDS_LIST_SIZE )
				chl_delete(&chl_list, chl_list->number);


			if ( messages_counter >= ANTISPAM_MODULE_MSG_CNT )
			{
				messages_counter = 0;
				start_signal = 0;
				total_time = get_tick_unix() - start_time;
			}

			if ( (total_time < ANTISPAM_MODULE_TOTAL_TIME_MS) || (interval < ANTISPAM_MODULE_MESSAGES_INTERVAL) )
			{
				messages_counter = 0;
				start_signal = 0;
				total_time = ANTISPAM_MODULE_TOTAL_TIME_MS;

				spam_module_process(peer_sock);
			}
		}
	}

	return 0;
}

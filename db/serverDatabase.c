/* Реализация модуля serverDatabase */

#ifndef SERVERDATABASE_C_SENTRY
#define SERVERDATABASE_C_SENTRY

#include "../Commons.h"
#include "../DateTime.h"
#include "../DatabaseStructures.h"
#include "SessionList.h"
#include "serverDatabase.h"
#include "DatabaseMsgHandlers.h"
#include <signal.h>

static int exit_flag = 0;
static int sig_number = 0;

enum
{ 
		QUEUE_SOCK_LEN					=			 16,
		MAX_TOKENS_IN_MESSAGE			=			100,
		WRITE_LINE_TOKENS				=			 15,
		MSG_SYN_TIMER_MS				=			200,
		MSG_SYN_TRIES_CNT				=			  3
};

/* DATABASE COMMANDS */
enum
{
		INIT_TABLES,
		DB_READLINE,
		DB_WRITELINE
};

const char* db_commands_names[] = 
{
			"INIT_TABLES",
			"DB_READLINE",
			"DB_WRITELINE",
			NULL
};

static int db_session_do_read(Server* serv_ptr, Session* sess);
static void db_session_message_handler(Server* serv_ptr, Session* sess, const char* client_line);
static void db_session_check_lf(Server* serv_ptr, Session* sess);

static void db_server_close_session(Server* serv_ptr, int sock);
static int db_server_accept_client(Server* serv_ptr);
static Session* db_get_session_by_fd(Server* serv_ptr, int fd);


void stop_handler(int signo)
{
	int save_errno = errno;
	signal(SIGINT, stop_handler);
	
	sig_number = signo;
	exit_flag = 1;

	errno = save_errno;
}


static int db_session_do_read(Server* serv_ptr, Session* sess)
{
	char cur_time[CURRENT_TIME_SIZE];
	
	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_do_read\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_do_read\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		return 0;
	}

	int rc = read(sess->data->fd, sess->data->buf, BUFSIZE);
	printf("[%s] %s Received %d bytes from %s => ", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, rc, sess->data->addr);
	
	if ( rc < 1 )
	{
		printf("%s\n", "()");
		return 0;
	}

	sess->data->buf_used += rc;

	if ( sess->data->buf_used >= BUFSIZE )
	{
		fprintf(stderr, "[%s] %s Client's(%s) line is too long!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sess->data->addr);
		return 0;
	}
	
	db_session_check_lf(serv_ptr, sess);

	return 1;
}

static void db_session_check_lf(Server* serv_ptr, Session* sess)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_check_lf\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_check_lf\" params \"sess\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	int pos = -1;

	int i;
	for ( i = 0; i < sess->data->buf_used; i++ )
	{
		if ( sess->data->buf[i] == '\n' )
		{
			pos = i;
			break;
		}
	}

	if ( pos < 1 )
		return;

	char* client_line = malloc(pos+1);
	if ( !client_line )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_check_lf\" unable to allocate memory to \"client_line\"\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	memcpy(client_line, sess->data->buf, pos);
	client_line[pos] = '\0';
	if ( client_line[pos-1] == '\r' )
		client_line[pos-1] = '\0';

	sess->data->buf_used -= (pos + 1);
	memmove(sess->data->buf, sess->data->buf+pos+1, sess->data->buf_used);

	printf("[%s] %s Client Line: ( %s )\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, client_line); /* debug */

	db_session_message_handler(serv_ptr, sess, client_line);
}

static void db_session_message_handler(Server* serv_ptr, Session* sess, const char* client_line)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_message_handler\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( sess == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_message_handler\" \"sess\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( client_line == NULL )
	{	
		fprintf(stderr, "[%s] %s In function \"db_session_message_handler\" \"client_line\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( client_line[0] == '\0' )
	{
		fprintf(stderr, "[%s] %s In function \"db_session_message_handler\" \"client_line\" is empty!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	char buffer[BUFFER_SIZE];
	int len = strlen(client_line);
	memcpy(buffer, client_line, (len < BUFFER_SIZE) ? len : BUFFER_SIZE-1);

	if ( len < BUFFER_SIZE )
		buffer[len] = '\0';
	else
		buffer[BUFFER_SIZE-1] = '\0';

	int i = 0;
	char* mes_tokens[MAX_TOKENS_IN_MESSAGE] = { NULL };
	char* istr = strtok(buffer, "|");
	while ( istr )
	{
		mes_tokens[i] = istr;
		i++;
		if ( i >= MAX_TOKENS_IN_MESSAGE )
			break;
		istr = strtok(NULL, "|");
	}
	int tokens_num = i;

	if ( strcmp(mes_tokens[0], db_commands_names[INIT_TABLES]) == 0 )
	{
		char records_num_buf[100];
		int len = strlen(mes_tokens[1]);
		
		if ( len >= 100 )
			len = 99;

		memcpy(records_num_buf, mes_tokens[1], len);
		records_num_buf[len] = '\0';
		int records_num = atoi(records_num_buf);
		

		char usersinfo_name[100];
		len = strlen(mes_tokens[2]);

		if ( len >= 100 )
			len = 99;

		memcpy(usersinfo_name, mes_tokens[2], len);
		usersinfo_name[len] = '\0';


		char usersessions_name[100];
		len = strlen(mes_tokens[3]);

		if ( len >= 100 )
			len = 99;

		memcpy(usersessions_name, mes_tokens[3], len);
		usersessions_name[len] = '\0';
		

		int ret_value1 = db_create_userinfo_table(records_num, usersinfo_name);
		int ret_value2 = db_create_usersessions_table(records_num, usersessions_name);
		
		const char* msg_to_send = NULL;
		if ( ret_value1 && ret_value2 )
		{
			msg_to_send = "DB_INITIALIZATION_SUCCESS\n";
			memcpy(serv_ptr->userinfo_table_name, usersinfo_name, strlen(usersinfo_name)+1);
			memcpy(serv_ptr->usersessions_table_name, usersessions_name, strlen(usersessions_name)+1);
			serv_ptr->db_records_num = records_num;
		}
		else
			msg_to_send = "DB_INITIALIZATION_ERROR\n";
		
		len = strlen(msg_to_send);
		int wc = write(sess->data->fd, msg_to_send, len);
		printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);
	}
	else if ( strcmp(mes_tokens[0], db_commands_names[DB_READLINE]) == 0 )
	{
		char username[100];
		int len = strlen(mes_tokens[1]);
		
		if ( len >= 100 )
			len = 99;

		memcpy(username, mes_tokens[1], len);
		username[len] = '\0';

		const char* msg_to_send = NULL;
		char* readline = db_readline_from_tables(username, serv_ptr->userinfo_table_name, serv_ptr->usersessions_table_name);
		if ( readline == NULL )
		{
			msg_to_send = "DB_LINE_NOT_FOUND\n";
			len = strlen(msg_to_send);
			int wc = write(sess->data->fd, msg_to_send, len);
			printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

			return;
		}
		
		msg_to_send = "DB_LINE_EXIST|";
		char send_buf[BUFFER_SIZE] = { 0 };
		len = strlen(msg_to_send);
		memcpy(send_buf, msg_to_send, len);
		send_buf[len] = '\0';

		int len_readline = strlen(readline);
		strncat(send_buf, readline, len_readline);
		len += len_readline;

		send_buf[len] = '\0';

		int wc = write(sess->data->fd, send_buf, len);
		printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

		if ( readline )
			free(readline);
	}
	else if ( strcmp(mes_tokens[0], db_commands_names[DB_WRITELINE]) == 0 )
	{
		/* DB_WRITE_LINE|username|pass|rank|realname|age|quote|muted|start_mute_time|mute_time|mute_time_left|last_ip|last_date_in|last_date_out|registration_date */
		if ( tokens_num != WRITE_LINE_TOKENS )
		{
			const char* msg_to_send = "DB_LINE_FORMAT_ERROR\n";
			int len = strlen(msg_to_send);
			int wc = write(sess->data->fd, msg_to_send, len);
			printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

			return;
		}		

		int index = db_get_record_index_by_name(mes_tokens[1], serv_ptr->userinfo_table_name);
		if ( index < 0 )
		{
			if ( db_userinfo_table_is_full(serv_ptr->userinfo_table_name) )
			{
				int ret_value1 = db_create_userinfo_table(serv_ptr->db_records_num*2, serv_ptr->userinfo_table_name);
				int ret_value2 = db_create_usersessions_table(serv_ptr->db_records_num*2, serv_ptr->usersessions_table_name);
				if ( ret_value1 && ret_value2 )
				{
					index = serv_ptr->db_records_num;
					serv_ptr->db_records_num *= 2;
					
					const char* msg_to_send = "DB_UPDATE_TABLE_SUCCESS|";
					char send_buf[BUFFER_SIZE] = { 0 };
					int pos = strlen(msg_to_send);
					memcpy(send_buf, msg_to_send, pos);
					send_buf[pos] = '\0';

					char new_size[10];
					itoa(serv_ptr->db_records_num, new_size, 9);

					strcat(send_buf, new_size);
					pos += strlen(new_size);
					send_buf[pos] = '\n';
					send_buf[pos+1] = '\0';

					int wc = write(sess->data->fd, send_buf, pos+1);
					printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, pos+1, sess->data->addr);

					/* механизм синхронизации для нормальной передачи двух сообщений: DB_UPDATE_TABLE_SUCCESS и DB_LINE_WRITE_SUCCESS/DB_LINE_WRITE_ERROR */
					int synchro_success = 0;
					struct timeval tv;
					tv.tv_sec = 0;
					tv.tv_usec = MSG_SYN_TIMER_MS;
					
					int tries = MSG_SYN_TRIES_CNT;
					while ( tries > 0 ) 
					{
						fd_set r;
						FD_ZERO(&r);
						FD_SET(sess->data->fd, &r);

						int ret_val = select(sess->data->fd+1, &r, NULL, NULL, &tv);
						if ( ret_val < 1 )
						{
							fprintf(stderr, "[%s] %s Unable to get synchronize message from app server [%s]\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, (ret_val == 0) ? "Timeout error" : "select() error");
							if ( ret_val == -1 )
								fprintf(stderr, "[%s] %s errno value = %d\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, errno);
							
							printf("[%s] %s Tries number: %d\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, tries);
							
							int wc = write(sess->data->fd, send_buf, pos+1);
							printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, pos+1, sess->data->addr);
							
							tries--;

							continue;
						}
						
						if ( FD_ISSET(sess->data->fd, &r) )
						{
							char buffer[100];
							int rc = read(sess->data->fd, buffer, 99);
							buffer[rc] = '\0';

							if ( strcmp(buffer, "SYN\n") != 0 )
							{
								fprintf(stderr, "[%s] %s Invalid synchronize message!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
								printf("[%s] %s Tries number: %d\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, tries);
							
								int wc = write(sess->data->fd, send_buf, pos+1);
								printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, pos+1, sess->data->addr);
								
								tries--;

								continue;
							}
							synchro_success = 1;
							break;
						}
					}
					
					if ( !synchro_success )
					{
						fprintf(stderr, "[%s] %s Failed synchronize with peer [%s].\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, sess->data->addr);
						
						const char* msg_to_send = "DB_SYNCHRONIZE_ERROR\n";
						int len = strlen(msg_to_send);
						int wc = write(sess->data->fd, msg_to_send, len);
						printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);
						db_server_close_session(serv_ptr, sess->data->fd);
						return;
					}
					/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
				}
				else
				{
					const char* msg_to_send = "DB_UPDATE_TABLE_ERROR\n";
					int len = strlen(msg_to_send);
					int wc = write(sess->data->fd, msg_to_send, len);
					printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);
					
					return;
				}
			}
			else
			{
				index = db_get_new_record_index(serv_ptr->userinfo_table_name);
			}
		}
		
		char write_line[BUFFER_SIZE] = { 0 };

		int pos = 0;
		char index_str[10];
		itoa(index, index_str, 9);
		int len = strlen(index_str);
		memcpy(write_line + pos, index_str, len);
		pos += len;
		write_line[pos] = '|';
		pos++;
		
		int i;
		for ( i = 1; i < WRITE_LINE_TOKENS; i++ )
		{
			len = strlen(mes_tokens[i]);
			memcpy(write_line + pos, mes_tokens[i], len);
			pos += len;
			write_line[pos] = '|';
			pos++;
		}
		write_line[pos-1] = '\0';

		if ( db_writeline_to_tables(write_line, serv_ptr->userinfo_table_name, serv_ptr->usersessions_table_name) )
		{
			const char* msg_to_send = "DB_LINE_WRITE_SUCCESS\n";
			int len = strlen(msg_to_send);
			int wc = write(sess->data->fd, msg_to_send, len);
			printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

			return;
		}

		const char* msg_to_send = "DB_LINE_WRITE_ERROR\n";
		len = strlen(msg_to_send);
		int wc = write(sess->data->fd, msg_to_send, len);
		printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);
	}
	else
	{
		fprintf(stderr, "[%s] %s In function \"db_session_message_handler\" \"client_line\" has invalid command value\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		
		const char* msg_to_send = "DB_UNKNOWN_COMMAND\n";
		int len = strlen(msg_to_send);
		int wc = write(sess->data->fd, msg_to_send, len);
		printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);
	}
}

static Session* db_get_session_by_fd(Server* serv_ptr, int fd)
{
	char cur_time[CURRENT_TIME_SIZE];
	
	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_session_by_fd\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	
	if ( fd < 0 )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_session_by_fd\" incorrect \"fd\" value!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	Session* s_list = serv_ptr->sess_list;
	while ( s_list )
	{
		if ( (s_list->data != NULL) && (s_list->data->fd == fd) )
			return s_list;

		s_list = s_list->next;
	}
		
	return NULL;
}

int db_server_init(int port)
{
	struct sockaddr_in addr;
	char cur_time[CURRENT_TIME_SIZE];


	printf("[%s] %s Creating socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( listen_sock < 0 )
	{
		fprintf(stderr, "[%s] %s socket() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return -1;
	}
	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); /* Предотвращение "залипания" TCP порта */
	
	
	printf("[%s] %s Binding socket to local address..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(listen_sock, (struct sockaddr*) &addr, sizeof(addr)) < 0 )
	{
		fprintf(stderr, "[%s] %s Failed to bind() a port. May be it has already used?\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}


	printf("[%s] %s Listening..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	if ( listen(listen_sock, QUEUE_SOCK_LEN) < 0 )
	{
		fprintf(stderr, "[%s] %s listen() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return -1;
	}
	

	printf("[%s] %s Waiting for connections..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);


	return listen_sock;
}

static int db_server_accept_client(Server* serv_ptr)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	char cur_time[CURRENT_TIME_SIZE];


	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_server_accept_client\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int client_sock = accept(serv_ptr->ls, (struct sockaddr*) &addr, &len);
	if ( client_sock == -1 )
	{
		fprintf(stderr, "[%s] %s accept() failed. {%d}\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, errno);
		return -1;
	}
	
	int ip =  ntohl(addr.sin_addr.s_addr);
	int port = ntohs(addr.sin_port);

	char* buf_ip = concat_addr_port(ip, port);
	if ( !buf_ip )
	{
		fprintf(stderr, "[%s] %s function \"concat_addr_port\" didn't make correct address!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}
	
	ClientData* data = malloc(sizeof(ClientData));
	if ( !data )
	{
		if ( buf_ip )
			free(buf_ip);

		fprintf(stderr, "[%s] %s In function \"db_server_accept_client\" unable to allocate memory to \"data\" pointer!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);

		return -1;
	}
	data->fd = client_sock;
	
	int i;
	for ( i = 0; buf_ip[i]; i++ )
		data->addr[i] = buf_ip[i];
	data->addr[i] = '\0';
	
	memset(data->buf, 0, sizeof(data->buf));

	data->buf_used = 0;

	sess_insert(&serv_ptr->sess_list, data);

	printf("[%s] %s New connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, buf_ip);
	
	if ( buf_ip )
		free(buf_ip);

	return client_sock;
}

static void db_server_close_session(Server* serv_ptr, int sock)
{
	char cur_time[CURRENT_TIME_SIZE];
	
	
	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_server_close_session\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}

	if ( sock < 0 )
	{
		fprintf(stderr, "[%s] %s In function \"db_server_close_session\" \"sock_num\" is less than 0\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}
	

	Session* sess = db_get_session_by_fd(serv_ptr, sock);

	if ( sess == NULL ) 
	{
		fprintf(stderr, "[%s] %s In function \"db_server_close_session\" unable find \"sock\" value in sessions list!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return;
	}
	close(sock);

	
	printf("[%s] %s Lost connection from %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, sess->data->addr);

	const ClientData* data = sess_remove(&serv_ptr->sess_list, sess->data);

	if ( data )
		free((void*) data);
}

int db_server_running(Server* serv_ptr)
{
	signal(SIGINT, stop_handler);

	fd_set readfds;
	int res, max_d;
	char cur_time[CURRENT_TIME_SIZE];

	if ( serv_ptr == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_server_close_session\" \"serv_ptr\" is NULL\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 1;
	}

	while ( 1 )
	{
		FD_ZERO(&readfds);
		FD_SET(serv_ptr->ls, &readfds);
		max_d = serv_ptr->ls;
		
		Session* sess = serv_ptr->sess_list;
		while ( sess )
		{
			if ( (sess->data != NULL) && (sess->data->fd > -1) )
			{
				FD_SET(sess->data->fd, &readfds);
				if ( sess->data->fd > max_d )
					max_d = sess->data->fd;
			}

			sess = sess->next;
		}

		res = select(max_d+1, &readfds, NULL, NULL, NULL);
		if ( res == -1 )
		{
			if ( errno == EINTR )
			{
				if ( sig_number == SIGINT )
				{
					Session* sess = serv_ptr->sess_list;
					while ( sess )
					{
						if ( (sess->data != NULL) && (sess->data->fd > -1) )
							db_server_close_session(serv_ptr, sess->data->fd);

						sess = sess->next;
					}
				
					close(serv_ptr->ls);

					if ( sess_clear(&serv_ptr->sess_list, 1) )
						serv_ptr->sess_list = NULL;

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

		if ( FD_ISSET(serv_ptr->ls, &readfds) )
			db_server_accept_client(serv_ptr);
		
		sess = serv_ptr->sess_list;
		while ( sess != NULL )
		{
			if ( (sess->data != NULL) && FD_ISSET(sess->data->fd, &readfds) )
				if ( !db_session_do_read(serv_ptr, sess) )
					db_server_close_session(serv_ptr, sess->data->fd);

			sess = sess->next;
		}
	}

	printf("\n[%s] %s Closing socket..\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 0;
}


#endif

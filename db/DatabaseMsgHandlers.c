#ifndef DATABASE_MSG_HANDLERS_C_SENTRY
#define DATABASE_MSG_HANDLERS_C_SENTRY

#include "DatabaseMsgHandlers.h"
#include "../Commons.h"
#include "../DateTime.h"
#include "../DatabaseStructures.h"
#include <ctype.h>

static const char* empty_field = "undefined";

enum
{
			MAX_TOKENS_IN_MESSAGE			=			100,
			WRITE_LINE_TOKENS				=			 15
};

int db_get_new_record_index(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	int index = 0;
	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_new_record_index\" unable to allocate memory to \"record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	fseek(fd, 0, SEEK_SET);

	while ( !feof(fd) )
	{
		fread(record, sizeof(DBUsersInformation), 1, fd);
		if ( record->ID == -1 )
		{
			free(record);
			fseek(fd, 0, SEEK_SET);
			return index;
		}

		index++;
	}

	if ( record )
		free(record);

	fseek(fd, 0, SEEK_SET);

	return -1;
}

int db_userinfo_table_get_size(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_userinfo_table_get_size\" unable to allocate memory to \"record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int records_num = 0;

	fseek(fd, 0, SEEK_SET);

	while ( !feof(fd) )
	{
		fread(record, sizeof(DBUsersInformation), 1, fd);
		records_num++;
	}

	if ( record )
		free(record);

	fseek(fd, 0, SEEK_SET);

	return records_num;
}

int db_userinfo_table_is_empty(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	int records_num = db_userinfo_table_get_size(fd);

	if ( records_num > 0 )
	{
		DBUsersInformation* first_record = malloc(sizeof(DBUsersInformation));
		if ( !first_record )
		{
			fprintf(stderr, "[%s] %s In function \"db_userinfo_table_is_empty\" unable to allocate memory to \"first_record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}

		int non_empty_record = 0;

		fseek(fd, 0, SEEK_SET);

		while ( !feof(fd) )
		{
			fread(first_record, sizeof(DBUsersInformation), 1, fd);
			if ( first_record->ID > -1 )
			{
				non_empty_record = 1;
				break;
			}
		}

		if ( first_record )
			free(first_record);

		fseek(fd, 0, SEEK_SET);

		if ( non_empty_record )
			return 0;

		return 1;
	}

	fprintf(stderr, "[%s] %s In function \"db_userinfo_table_is_empty\" \"records_num\" less than 1!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
	return 0;
}

int db_userinfo_table_is_full(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	int records_num = db_userinfo_table_get_size(fd);

	if ( records_num > 0 )
	{
		DBUsersInformation* first_record = malloc(sizeof(DBUsersInformation));
		if ( !first_record )
		{
			fprintf(stderr, "[%s] %s In function \"db_userinfo_table_is_full\" unable to allocate memory to \"first_record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}
		int empty_record = 0;

		fseek(fd, 0, SEEK_SET);

		while ( !feof(fd) )
		{
			fread(first_record, sizeof(DBUsersInformation), 1, fd);
			if ( first_record->ID == -1 )
			{
				empty_record = 1;
				break;
			}
		}

		if ( first_record )
			free(first_record);

		fseek(fd, 0, SEEK_SET);

		if ( empty_record )
			return 0;

		return 1;
	}

	fprintf(stderr, "[%s] %s In function \"db_userinfo_table_is_full\" \"records_num\" less than 1!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
	return 0;
}

FILE* db_create_userinfo_table(int records_num, const char* table_name)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( records_num < 1 )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" unable to initialize \"%s\" database file. Incorrect records num value!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, table_name);
		return NULL;
	}

	FILE* fd;
	if ( (fd = fopen(table_name, "rb+")) == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" file \"%s\" does not exist! Creating new one!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE, table_name);
		if ( (fd = fopen(table_name, "wb+")) == NULL )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" you don't have permission to create file in this directory.\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return NULL;
		}
	}
	else
	{
		int read_size = db_userinfo_table_get_size(fd);
		if ( read_size < 0 )
		{
			return NULL;
		}

		if ( read_size >= records_num )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" table \"%s\" is already initialized!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE, table_name);
			return fd;
		}

		fclose(fd);
		if ( (fd = fopen(table_name, "ab+")) == NULL )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" you don't have permission to create file in this directory.\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}
		
		records_num -= read_size;
	}

	DBUsersInformation* new_record = malloc( sizeof(DBUsersInformation) );
	if ( !new_record )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_userinfo_table\" memory error\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int i;
	for ( i = 1; i <= records_num; i++ )
	{
		memset(new_record, 0, sizeof(DBUsersInformation));

		new_record->ID = -1;

		memcpy(new_record->username, empty_field, strlen(empty_field)+1);
		memcpy(new_record->pass, empty_field, strlen(empty_field)+1);

		new_record->rank[0] = 'u';
		new_record->rank[1] = '\0';

		new_record->age = -1;
		memcpy(new_record->realname, empty_field, strlen(empty_field)+1);
		memcpy(new_record->quote, empty_field, strlen(empty_field)+1);

		fwrite(new_record, sizeof(DBUsersInformation), 1, fd);
	}

	if ( new_record )
		free(new_record);

	return fd;
}

int db_usersessions_table_get_size(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	DBXUsersInformation* record = malloc(sizeof(DBXUsersInformation));
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_usersessions_table_get_size\" unable to allocate memory to \"record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}
	int records_num = 0;

	fseek(fd, 0, SEEK_SET);

	while ( !feof(fd) )
	{
		fread(record, sizeof(DBXUsersInformation), 1, fd);
		records_num++;
	}

	if ( record )
		free(record);

	fseek(fd, 0, SEEK_SET);

	return records_num;
}

int db_usersessions_table_is_empty(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	int records_num = db_usersessions_table_get_size(fd);

	if ( records_num > 0 )
	{
		DBXUsersInformation* first_record = malloc(sizeof(DBXUsersInformation));
		if ( !first_record )
		{
			fprintf(stderr, "[%s] %s In function \"db_usersessions_table_is_empty\" unable to allocate memory to \"first_record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}
		int non_empty_record = 0;

		fseek(fd, 0, SEEK_SET);

		while ( !feof(fd) )
		{
			fread(first_record, sizeof(DBXUsersInformation), 1, fd);
			if ( first_record->ID > -1 )
			{
				non_empty_record = 1;
				break;
			}
		}

		if ( first_record )
			free(first_record);

		fseek(fd, 0, SEEK_SET);

		if ( non_empty_record )
			return 0;

		return 1;
	}

	fprintf(stderr, "[%s] %s In function \"db_usersessions_table_is_empty\" \"records_num\" less than 1!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
	return 0;
}

int db_usersessions_table_is_full(FILE* fd)
{
	char cur_time[MAX_TIME_STR_SIZE];

	int records_num = db_usersessions_table_get_size(fd);

	if ( records_num > 0 )
	{
		DBXUsersInformation* first_record = malloc(sizeof(DBXUsersInformation));
		if ( !first_record )
		{
			fprintf(stderr, "[%s] %s In function \"db_usersessions_table_is_full\" unable to allocate memory to \"first_record\" pointer!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}
		int empty_record = 0;

		fseek(fd, 0, SEEK_SET);

		while ( !feof(fd) )
		{
			fread(first_record, sizeof(DBXUsersInformation), 1, fd);
			if ( first_record->ID == -1 )
			{
				empty_record = 1;
				break;
			}
		}

		if ( first_record )
			free(first_record);

		fseek(fd, 0, SEEK_SET);

		if ( empty_record )
			return 0;

		return 1;
	}

	fprintf(stderr, "[%s] %s In function \"db_usersessions_table_is_full\" \"records_num\" less than 1!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
	return 0;
}

FILE* db_create_usersessions_table(int records_num, const char* table_name)
{
	char cur_time[MAX_TIME_STR_SIZE] = { 0 };

	if ( records_num < 1 )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" unable to initialize \"%s\" database file. Incorrect records num value!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, table_name);
		return NULL;
	}

	FILE* fd;
	if ( (fd = fopen(table_name, "rb+")) == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" file \"%s\" does not exist! Creating new one!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE, table_name);
		if ( (fd = fopen(table_name, "wb+")) == NULL )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" you don't have permission to create file in this directory.\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return NULL;
		}
	}
	else
	{
		int read_size = db_usersessions_table_get_size(fd);
		if ( read_size < 0 )
		{
			return NULL;
		}

		if ( read_size == records_num )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" table \"%s\" is already initialized!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE, table_name);
			return fd;
		}

		fclose(fd);
		if ( (fd = fopen(table_name, "ab+")) == NULL )
		{
			fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" you don't have permission to create file in this directory.\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
			return NULL;
		}

		if ( read_size > 0 )
			if ( records_num > read_size )
				records_num -= read_size;
	}


	DBXUsersInformation* new_record = malloc(sizeof(DBXUsersInformation));
	if ( !new_record )
	{
		fprintf(stderr, "[%s] %s In function \"db_create_usersessions_table\" memory error\n", get_time_str(cur_time,MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int i;
	for ( i = 1; i <= records_num; i++ )
	{
		memset(new_record, 0, sizeof(DBXUsersInformation));

		new_record->ID = -1;
		new_record->muted = 0;
		new_record->mute_time = 0;
		new_record->start_mute_time = 0;
		new_record->mute_time_left = 0;

		memcpy(new_record->last_ip, empty_field, strlen(empty_field)+1);
		memcpy(new_record->last_date_in, empty_field, strlen(empty_field)+1);
		memcpy(new_record->registration_date, empty_field, strlen(empty_field)+1);
		memcpy(new_record->last_date_out, empty_field, strlen(empty_field)+1);

		fwrite(new_record, sizeof(DBXUsersInformation), 1, fd);
	}

	if ( new_record )
		free( new_record );

	return fd;
}

int db_get_record_index(FILE* fd, const char* search_key)
{
	char cur_time[MAX_TIME_STR_SIZE] = { 0 };

	if ( search_key == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_record_index_by_name\" an internal error has occured. \"nickname\" param is NULL\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int records_size = db_userinfo_table_get_size(fd);
	if ( records_size < 1 )
	{
		fprintf(stderr, "[%s] %s An internal error occured in \"get_record_index_by_name\" while attempting to get \"record_size\" param. It's 0\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	DBUsersInformation* record = malloc( sizeof(DBUsersInformation) );
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"get_record_index_by_name\" memory error with \"record\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int is_number = 0;
	int i;
	for ( i = 0; search_key[i]; i++ )
	{
		if ( !isdigit(search_key[i]) )
		{
			is_number = 0;
			break;
		}
		is_number = 1;
	}

	for ( i = 0; i < records_size; i++ )
	{
		memset(record, 0, sizeof(DBUsersInformation));
		fseek(fd, i * sizeof(DBUsersInformation), SEEK_SET);
		fread(record, sizeof(DBUsersInformation), 1, fd);

		if ( !is_number )
		{
			if ( strcmp(record->username, search_key) == 0 )
				break;
		}
		else
		{
			int key = atoi(search_key);
			if ( record->ID == key )
				break;
		}
	}

	if ( record )
		free(record);

	fseek(fd, 0, SEEK_SET);

	if ( i == records_size )
	{
		fprintf(stderr, "[%s] %s In function \"get_record_index_by_name\" unable to find record with search key \"%s\" in the table!\n",
				get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, search_key);
		return -1;
	}

	return i;
}

char* db_readline_from_tables(FILE* usr_fd, FILE* sess_fd, const char* search_key)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( search_key == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" \"name\" is NULL!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int index = db_get_record_index(usr_fd, search_key);
	if ( index < 0 )
	{
		return NULL;
	}

	DBUsersInformation* record = malloc( sizeof(DBUsersInformation) );
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"record\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	memset(record, 0, sizeof(DBUsersInformation));

	fseek(usr_fd, index * sizeof(DBUsersInformation), SEEK_SET);
	fread(record, sizeof(DBUsersInformation), 1, usr_fd);

	char buffer[100] = { 0 };
	itoa(record->ID, buffer, 99);


	char* result = malloc(sizeof(char) * BUFFER_SIZE);
	if ( !result )
	{
		free(record);
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"result\"!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int pos = strlen(buffer);
	memcpy(result, buffer, pos);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));

	pos += strlen(record->username);
	strcat(result, record->username);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(record->pass);
	strcat(result, record->pass);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(record->rank);
	strcat(result, record->rank);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(record->realname);
	strcat(result, record->realname);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	itoa(record->age, buffer, 99);
	pos += strlen(buffer);
	strcat(result, buffer);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));


	pos += strlen(record->quote);
	strcat(result, record->quote);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	if ( record )
		free(record);


	DBXUsersInformation* xrecord = malloc( sizeof(DBXUsersInformation) );
	if ( !xrecord )
	{
		free(result);
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"xrecord\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	memset(xrecord, 0, sizeof(DBXUsersInformation));
	fseek(sess_fd, index * sizeof(DBXUsersInformation), SEEK_SET);
	fread(xrecord, sizeof(DBXUsersInformation), 1, sess_fd);


	itoa(xrecord->muted, buffer, 99);

	pos += strlen(buffer);
	strcat(result, buffer);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));


	itoa(xrecord->start_mute_time, buffer, 99);

	pos += strlen(buffer);
	strcat(result, buffer);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));


	itoa(xrecord->mute_time, buffer, 99);

	pos += strlen(buffer);
	strcat(result, buffer);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));


	itoa(xrecord->mute_time_left, buffer, 99);

	pos += strlen(buffer);
	strcat(result, buffer);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';

	memset(buffer, 0, sizeof(buffer));


	pos += strlen(xrecord->last_ip);
	strcat(result, xrecord->last_ip);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(xrecord->last_date_in);
	strcat(result, xrecord->last_date_in);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(xrecord->last_date_out);
	strcat(result, xrecord->last_date_out);
	result[pos] = '|';
	pos++;
	result[pos] = '\0';


	pos += strlen(xrecord->registration_date);
	strcat(result, xrecord->registration_date);
	result[pos] = '\n';
	pos++;
	result[pos] = '\0';

	if ( xrecord )
		free(xrecord);

	return result;
}

int db_writeline_to_tables(FILE* usr_fd, FILE* sess_fd, char* writeline)
{
	/* writeline string format	=>	id|username|pass|rank|realname|age|quote|muted|start_mutetime|mute_time|mute_time_left|last_ip|last_date_in|last_date_out|registration_date */
	/* если в поле значение undefined или в поле rank значение u, то обновлять это поле не нужно */

	char cur_time[MAX_TIME_STR_SIZE];

	if ( writeline == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" \"name\" is NULL!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int i = 0;
	char* mes_tokens[MAX_TOKENS_IN_MESSAGE] = { NULL };
	char* istr = strtok(writeline, "|");

	while ( istr )
	{
		mes_tokens[i] = istr;
		i++;
		if ( i >= MAX_TOKENS_IN_MESSAGE )
			break;
		istr = strtok(NULL, "|");
	}
	int tokens_num = i;

	if ( tokens_num != WRITE_LINE_TOKENS )
	{
		fprintf(stderr, "[%s] %s In function \"db_writeline_to_tables\" an internal error has occured. \"tokens_num\" is not correct!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}


	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_writeline_to_tables\" an internal error has occured. Memory error in \"record\"!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	DBXUsersInformation* xrecord = malloc(sizeof(DBXUsersInformation));
	if ( !record )
	{
		free(record);
		fprintf(stderr, "[%s] %s In function \"db_writeline_to_tables\" an internal error has occured. Memory error in \"record\"!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int index = atoi(mes_tokens[0]);
	fseek(usr_fd, index * sizeof(DBUsersInformation), SEEK_SET);
	fread(record, sizeof(DBUsersInformation), 1, usr_fd);

	record->ID = index;

	if ( strcmp(mes_tokens[1], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[1]);
		memcpy(record->username, mes_tokens[1], len);
		record->username[len] = '\0';
	}

	if ( strcmp(mes_tokens[2], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[2]);
		memcpy(record->pass, mes_tokens[2], len);
		record->pass[len] = '\0';
	}

	if ( strcmp(mes_tokens[3], "u") != 0 )
	{
		int len = strlen(mes_tokens[3]);
		memcpy(record->rank, mes_tokens[3], len);
		record->rank[len] = '\0';
	}

	if ( strcmp(mes_tokens[4], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[4]);
		memcpy(record->realname, mes_tokens[4], len);
		record->realname[len] = '\0';
	}

	if ( strcmp(mes_tokens[5], empty_field) != 0 )
		record->age = atoi(mes_tokens[5]);

	if ( strcmp(mes_tokens[6], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[6]);
		memcpy(record->quote, mes_tokens[6], len);
		record->quote[len] = '\0';
	}

	fseek(usr_fd, index * sizeof(DBUsersInformation), SEEK_SET);
	fwrite(record, sizeof(DBUsersInformation), 1, usr_fd);
	free(record);


	fseek(sess_fd, index * sizeof(DBXUsersInformation), SEEK_SET);
	fread(xrecord, sizeof(DBXUsersInformation), 1, sess_fd);

	xrecord->ID = index;

	if ( strcmp(mes_tokens[7], empty_field) != 0 )
		xrecord->muted = atoi(mes_tokens[7]);
	if ( strcmp(mes_tokens[8], empty_field) != 0 )
		xrecord->start_mute_time = atoi(mes_tokens[8]);
	if ( strcmp(mes_tokens[9], empty_field) != 0 )
		xrecord->mute_time = atoi(mes_tokens[9]);
	if ( strcmp(mes_tokens[10], empty_field) != 0 )
		xrecord->mute_time_left = atoi(mes_tokens[10]);

	if ( strcmp(mes_tokens[11], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[11]);
		memcpy(xrecord->last_ip, mes_tokens[11], len);
		xrecord->last_ip[len] = '\0';
	}

	if ( strcmp(mes_tokens[12], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[12]);
		memcpy(xrecord->last_date_in, mes_tokens[12], len);
		xrecord->last_date_in[len] = '\0';
	}

	if ( strcmp(mes_tokens[13], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[13]);
		memcpy(xrecord->last_date_out, mes_tokens[13], len);
		xrecord->last_date_out[len] = '\0';
	}

	if ( strcmp(mes_tokens[14], empty_field) != 0 )
	{
		int len = strlen(mes_tokens[14]);
		memcpy(xrecord->registration_date, mes_tokens[14], len);
		xrecord->registration_date[len] = '\0';
	}

	fseek(sess_fd, index * sizeof(DBXUsersInformation), SEEK_SET);
	fwrite(xrecord, sizeof(DBXUsersInformation), 1, sess_fd);
	free(xrecord);

	return 1;
}

#endif

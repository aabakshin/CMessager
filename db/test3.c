#include "../Commons.h"
#include "../DateTime.h"
#include "../DatabaseStructures.h"

static int db_userinfo_table_get_size(const char* table_name)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( (table_name == NULL) || (table_name[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s In function \"db_userinfo_table_get_size\" incorrect \"table_name\" argument!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}
	
	FILE* fd;
	if ( (fd = fopen(table_name, "rb")) == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_userinfo_table_get_size\" file \"%s\" does not exist!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, table_name);
		return -1;
	}

	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	int records_num = 0;

	while ( !feof(fd) )
	{
		fread(record, sizeof(DBUsersInformation), 1, fd);
		records_num++;		
	}
	
	if ( record )
		free(record);
	
	fclose(fd);

	return records_num;
}

static int db_get_record_index_by_name(const char* nickname, const char* userinfo_table_name)
{
	char cur_time[MAX_TIME_STR_SIZE] = { 0 };
	
	if ( nickname == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_record_index_by_name\" an internal error has occured. \"nickname\" param is NULL\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}
	
	if ( userinfo_table_name == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_get_record_index_by_name\" an internal error has occured. \"userinfo_table_name\" is NULL\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int records_size = db_userinfo_table_get_size(userinfo_table_name);
	if ( records_size < 1 )
	{
		fprintf(stderr, "[%s] %s An internal error occured in \"get_record_index_by_name\" while attempting to get \"record_size\" param. It's 0\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}
	
	FILE* fd = NULL;
	if ( !(fd = fopen(userinfo_table_name, "rb")) )
	{
		fprintf(stderr, "[%s] %s In function \"get_record_index_by_name\" unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, userinfo_table_name);
		return -1;
	}
	
	DBUsersInformation* record = malloc( sizeof(DBUsersInformation) );
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"get_record_index_by_name\" memory error with \"record\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return -1;
	}

	int i;
	for (i = 0; i < records_size; i++)
	{
		memset(record, 0, sizeof(DBUsersInformation));
		fseek(fd, i * sizeof(DBUsersInformation), SEEK_SET);
		fread(record, sizeof(DBUsersInformation), 1, fd);

		if ( strcmp(record->username, nickname) == 0 )
			break;
	}
	
	if ( record )
		free(record);

	if ( fd )
		fclose(fd);

	if ( i == records_size )
	{
		fprintf(stderr, "[%s] %s In function \"get_record_index_by_name\" unable to find record with name \"%s\" in \"%s\" table!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, nickname, userinfo_table_name);
		return -1;
	}

	return i;
}

static char* db_readline_from_tables(const char* name, const char* userinfo_table_name, const char* usersessions_table_name)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( name == NULL )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" \"name\" is NULL!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int index = db_get_record_index_by_name(name, userinfo_table_name);
	
	if ( index < 0 )
	{
		return NULL;
	}

	char* result = malloc(sizeof(char) * BUFFER_SIZE);
	if ( !result )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"result\"!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	FILE* fd = NULL;
	if ( !(fd = fopen(userinfo_table_name, "rb")) )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, userinfo_table_name);
		return NULL;
	}

	DBUsersInformation* record = malloc( sizeof(DBUsersInformation) );
	if ( !record )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"record\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	memset(record, 0, sizeof(DBUsersInformation));
	fseek(fd, index * sizeof(DBUsersInformation), SEEK_SET);
	fread(record, sizeof(DBUsersInformation), 1, fd);

	char buffer[100] = { 0 };
	itoa(record->ID, buffer, 99);

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

	if ( fd )
		fclose(fd);

	
	fd = NULL;
	if ( !(fd = fopen(usersessions_table_name, "rb")) )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, userinfo_table_name);
		return NULL;
	}

	DBXUsersInformation* xrecord = malloc( sizeof(DBXUsersInformation) );
	if ( !xrecord )
	{
		fprintf(stderr, "[%s] %s In function \"db_readline_from_tables\" memory error with \"xrecord\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}
	memset(xrecord, 0, sizeof(DBXUsersInformation));
	fseek(fd, index * sizeof(DBXUsersInformation), SEEK_SET);
	fread(xrecord, sizeof(DBXUsersInformation), 1, fd);

	
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

	if ( fd )
		fclose(fd);

	return result;
}


int main(int argc, char** argv)
{
	//char cur_time[MAX_TIME_STR_SIZE];

	const char* msg_to_send = NULL;
	char* readline = db_readline_from_tables("Alex", "users_data.dat", "users_sessions_info.dat");	// SELECT "Alex" FROM "users_data.dat", "users_sessions_info.dat";
	if ( readline == NULL )
	{
		msg_to_send = "DB_LINE_NOT_FOUND\n";
		printf("\"msg_to_send\" = %s\n", msg_to_send);
		//printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

		return 0;
	}
	
	msg_to_send = "DB_LINE_EXIST|";
	char send_buf[BUFFER_SIZE] = { 0 };
	int len = strlen(msg_to_send);
	memcpy(send_buf, msg_to_send, len);
	send_buf[len] = '\0';

	int len_readline = strlen(readline);
	strncat(send_buf, readline, len_readline);
	len += len_readline;

	send_buf[len] = '\0';

	printf("\"send_buf\" = %s\n", send_buf);
	//int wc = write(sess->data->fd, send_buf, len);
	//printf("[%s] %s Sent %d\\%d bytes to %s\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE, wc, len, sess->data->addr);

	if ( readline )
		free(readline);



	return 0;
}

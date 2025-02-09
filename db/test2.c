#include "../Commons.h"
#include "../DateTime.h"
#include "../DatabaseStructures.h"

static int db_userinfo_table_get_size(const char* table_name)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( (table_name == NULL) || (table_name[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s In function \"db_table_is_empty\" incorrect \"table_name\" argument!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}
	
	FILE* fd;
	if ( (fd = fopen(table_name, "rb")) == NULL )
	{
		fprintf(stderr, "[%s] %s File \"%s\" does not exist!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, table_name);
		return 0;
	}

	DBUsersInformation* record = malloc(sizeof(DBUsersInformation));
	int records_num = 0;

	while ( !feof(fd) )
	{
		fread(record, sizeof(DBUsersInformation), 1, fd);
		printf("%s\n", record->username);
		records_num++;
	}
	
	if ( record )
		free(record);
	
	fclose(fd);

	return records_num;
}

static int db_userinfo_table_is_empty(const char* table_name)
{
	char cur_time[MAX_TIME_STR_SIZE];

	if ( (table_name == NULL) || (table_name[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s In function \"db_table_is_empty\" incorrect \"table_name\" argument!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	int records_num = db_userinfo_table_get_size(table_name);

	if ( records_num > 0 )
	{
		FILE* fd;
		if ( (fd = fopen(table_name, "rb")) == NULL )
		{
			fprintf(stderr, "[%s] %s File \"%s\" does not exist!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, table_name);
			return 0;
		}

		DBUsersInformation* first_record = malloc(sizeof(DBUsersInformation));
		int non_empty_record = 0;

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
		
		fclose(fd);
		
		if ( non_empty_record )
			return 0;
		
		return 1;
	}
	
	fprintf(stderr, "[%s] %s In function \"db_table_is_empty\" \"records_num\" less than 1!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
	return 0;
}



int main(int argc, char** argv)
{
	int size = db_userinfo_table_get_size("users_data.dat");

	printf("size = %d\n", size);
	
	printf("Is table an empty? - %s\n", db_userinfo_table_is_empty("users_data.dat") ? "Yes" : "No");
	
	return 0;
}


#include "serverConfigs.h"
#include "DateTime.h"
#include "Commons.h"

enum
{
		BUFFER_SIZE				=			2048
};

int main(void)
{
	char parsed_options[3][2][100] =
	{
		{"curDbSize", "10"},
		{"usersinfoDbName", "users_info.dat"},
		{"usersessionsDbName", "users_sessions.dat"}
	};

	char send_buf[BUFFER_SIZE];
	const char* init_db_msg = "INIT_DB_RECORDS_NUM|";
	int len = strlen(init_db_msg);
	memcpy(send_buf, init_db_msg, len);

	int i = len;
	int k;
	for ( k = 0; k < CONFIG_STRINGS_NUM; k++ )
	{
		int j = 0;
		for ( ; parsed_options[k][1][j]; i++, j++ )
			send_buf[i] = parsed_options[k][1][j];
		send_buf[i] = '|';
		i++;
	}
	send_buf[i-1] = '\n';
	send_buf[i] = '\0';

	printf("send_buf = %s\n", send_buf);	


	return 1;
}

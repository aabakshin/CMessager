#include "../Commons.h"
#include "../DatabaseStructures.h"
#include "../DateTime.h"

enum
{
	WRITE_LINE_TOKENS		=		15
};

const char* mes_tokens[WRITE_LINE_TOKENS+1] =
{
	"DB_WRITELINE",
	"Alex",
	"Password",
	"A",
	"Alexandr",
	"24",
	"YA NIXUYA NE SMOG",
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

static int db_writeline_to_tables(const char* write_line, const char* userinfo_table_name, const char* usersessions_table_name)
{
	return 0;
}

int main(int argc, char** argv)
{

	int index = 1;
	char cur_time[MAX_TIME_STR_SIZE];

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

	printf("write_line = %s\n", write_line);

	if ( db_writeline_to_tables(write_line, "users_data.dat", "users_sessions_info.dat") )
	{
		const char* msg_to_send = "DB_LINE_WRITE_SUCCESS\n";
		int len = strlen(msg_to_send);
		printf("msg_to_send = %s\n", msg_to_send);

		return 1;
	}

	const char* msg_to_send = "DB_LINE_WRITE_ERROR\n";
	len = strlen(msg_to_send);
	printf("msg_to_send = %s\n", msg_to_send);

	return 0;
}

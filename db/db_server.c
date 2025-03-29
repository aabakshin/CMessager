#include "SessionList.h"
#include "serverDatabase.h"
#include "../Commons.h"
#include "../DateTime.h"

int main(int argc, char** argv)
{
	char cur_time[CURRENT_TIME_SIZE];

	Server serv;
	serv.ls = -1;
	serv.sess_list = NULL;
	serv.db_records_num = -1;

	clear_screen();

	if ( argc != 2 )
	{
		fprintf(stderr, "[%s] %s Usage: <program_name> <port>\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 1;
	}

	int port_number = atoi(argv[1]);
	if ( port_number < 1024 )
	{
		fprintf(stderr, "[%s] %s Incorrect port number\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 2;
	}

	int listen_sock = -1;
	if ( (listen_sock = db_server_init(port_number)) == -1 )
	{
		fprintf(stderr, "[%s] %s Unable to initialize server\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 3;
	}
	serv.ls = listen_sock;

	int ret_value = db_server_running(&serv);
	printf("[%s] %s Finished\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return ret_value;
}

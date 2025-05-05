#include "../includes/serverCore.h"
#include "../includes/Commons.h"
#include "../includes/DateTime.h"

int main(int argc, char** argv)
{
	char cur_time[CURRENT_TIME_SIZE];

	printf("\033c"); /* clear the screen for VT100 terminal */

	if ( argc != 2 )
	{
		fprintf(stderr, "[%s] %s Usage: <program_name> <port>\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 1;
	}

	int port_number = atoi(argv[1]);
	if ( port_number <= 1024 )
	{
		fprintf(stderr, "[%s] %s Incorrect port number\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 2;
	}

	Server serv;
	memset(&serv, 0, sizeof(Server));
	if ( !server_init(port_number, &serv) )
	{
		fprintf(stderr, "[%s] %s Unable to initialize server\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 3;
	}

	int ret_value = server_running(&serv);
	printf("[%s] %s Finished\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return ret_value;
}

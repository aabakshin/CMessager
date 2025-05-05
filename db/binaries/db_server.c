#include "../includes/Config.h"
#include "../includes/serverDatabase.h"
#include "../../includes/Commons.h"
#include "../../includes/DateTime.h"

int main(int argc, char** argv)
{
	char cur_time[CURRENT_TIME_SIZE];

	printf("\033c");
	if ( argc != 2 )
	{
		fprintf(stderr, "[%s] %s Usage: <program_name> <port>\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
		return 1;
	}

	int port_number = atoi(argv[1]);
	if ( port_number <= 1024 )
	{
		fprintf(stderr, "[%s] %s Incorrect port number\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 2;
	}

	InitDbServData srv_data;
	memset(&srv_data, 0, sizeof(InitDbServData));

	ConfigFields cfg;
	memset(&cfg, 0, sizeof(ConfigFields));

	if ( db_server_init(port_number, &srv_data, &cfg) == -1 )
	{
		fprintf(stderr, "[%s] %s Unable to initialize server\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 3;
	}

	Server serv;
	memset(&serv, 0, sizeof(Server));
	serv.server_data = &srv_data;
	serv.sess_list = NULL;

	int ret_value = db_server_running(&serv);
	printf("[%s] %s Finished\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return ret_value;
}

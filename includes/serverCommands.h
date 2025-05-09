#ifndef SERVERCOMMANDS_H_SENTRY
#define SERVERCOMMANDS_H_SENTRY

#include "serverCore.h"
#include "DatabaseStructures.h"

enum
{
	CMD_ARGS_MAX_LENGTH				=			200,
	MIN_AGE_VALUE					=			  1,
	MAX_AGE_VALUE					=			150,
	MIN_MUTE_TIME_SEC				=			 10,
	MAX_MUTE_TIME_SEC				=		   3600
};


enum
{
	STATUS_STR_SIZE					=			 10,
	AGE_STR_SIZE					=			 10
};

typedef struct
{
	char username[LOGIN_SIZE];
	char status[STATUS_STR_SIZE];
	char rank[RANK_SIZE];
	char regdate[REG_DATE_SIZE];
	char age[AGE_STR_SIZE];
	char realname[REALNAME_SIZE];
	char quote[QUOTE_SIZE];
} ResponseRecord;


enum
{
	AUTH_STR_SIZE					=			 10,
	USED_STR_SIZE					=			 10,
	SOCK_STR_SIZE					=			 10,
	STATE_STR_SIZE					=			 10
};

typedef struct
{
	char username[LOGIN_SIZE];
	char id[ID_SIZE];
	char auth[AUTH_STR_SIZE];
	char used[USED_STR_SIZE];
	char last_date_in[LAST_DATE_IN_SIZE];
	char last_ip[LAST_IP_SIZE];
	char regdate[REG_DATE_SIZE];
	char pass[PASS_SIZE];
	char rank[RANK_SIZE];
	char sock[SOCK_STR_SIZE];
	char state[STATE_STR_SIZE];
	char status[STATE_STR_SIZE];
	char muted[MUTED_SIZE];
	char mute_time[MUTE_TIME_SIZE];
	char mute_time_left[MUTE_TIME_LEFT_SIZE];
	char start_mute_time[START_MUTE_TIME_SIZE];
} ResponseDebugRecord;


int eval_rank_num(const char* last_date_in, const char* registration_date);
void set_user_rank(ClientSession *sess);
char get_user_rank(int rank);
void eval_mute_time_left(ClientSession *sess);
void view_data(const char* str, int str_size, char mode, int line_length);
int clear_cmd_args(char** cmd_args, int args_num);
void command_overlimit_length_handler(ClientSession* sess);
void help_command_handler(ClientSession *sess, char **cmd_args, int args_num);
void whoih_command_handler(ClientSession *sess, char **cmd_args, int args_num);
void chgpass_command_handler(ClientSession *sess, char **cmd_args, int args_num);
void op_command_handler(ClientSession *sess, char **cmd_args, int args_num);
void deop_command_handler(ClientSession *sess, char **cmd_args, int args_num);
void pm_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void status_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void record_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void mute_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void unmute_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void kick_command_handler(ClientSession* sess, char **cmd_args, int args_num);
void table_command_handler(ClientSession* sess, char** cmd_args, int args_num);
void ban_command_handler(ClientSession* sess, char** cmd_args, int args_num);
void unban_command_handler(ClientSession* sess, char** cmd_args, int args_num);
void text_message_handler(ClientSession* sess, const char* msg, int is_private, const char* adresat);
char** is_received_message_command(const char* msg, int* cmd_num, int* args_num);

#endif

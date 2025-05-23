#ifndef COMMONS_H_SENTRY
#define COMMONS_H_SENTRY


#define INFO_MESSAGE_TYPE "[INFO]: "
#define WARN_MESSAGE_TYPE "[WARN]: "
#define ERROR_MESSAGE_TYPE "[ERROR]: "

enum
{
	BUFSIZE							=		   1024,
	MIN_LOGIN_LENGTH				=			  3,
	MAX_LOGIN_LENGTH				=			 16,
	MIN_PASS_LENGTH					=			  4,
	MAX_PASS_LENGTH					=			 20,
	DEBUG_RECORD_FIELDS_NUM			=			 17,
	USER_RECORD_FIELDS_NUM			=			  7
};

enum { SERVER_CODES_COUNT			=			 42 };
enum { SUBCOMMANDS_CODES_COUNT		=			  8 };

enum
{
			CANNOT_CONNECT_DATABASE_CODE,
			CHGPWD_COMMAND_SUCCESS_CODE,
			CHGPWD_COMMAND_INCORRECT_PASS_CODE,
			CLIENT_HAS_ACCOUNT_CODE,
			CMD_ARG_OVERLIMIT_LENGTH_CODE,
			COMMAND_INVALID_PARAMS_CODE,
			COMMAND_NO_PERMS_CODE,
			COMMAND_PARAMS_NO_NEED_CODE,
			DEOP_COMMAND_SUCCESS_CODE,
			DEOP_COMMAND_USER_ADREADY_USER_CODE,
			HELP_COMMAND_SUCCESS_CODE,
			KICK_COMMAND_SUCCESS_CODE,
			LOGIN_WAIT_LOGIN_CODE,
			LOGIN_ALREADY_AUTHORIZED_CODE,
			LOGIN_ALREADY_USED_CODE,
			LOGIN_INCORRECT_CODE,
			LOGIN_NOT_EXIST_CODE,
			LOGIN_WAIT_PASS_CODE,
			MUTE_COMMAND_USER_ALREADY_MUTED_CODE,
			MUTE_COMMAND_SUCCESS_CODE,
			MUTE_COMMAND_YOU_MUTED_CODE,
			NEW_PASS_INCORRECT_CODE,
			NO_PERM_TO_CREATE_FILE_CODE,
			OP_COMMAND_SUCCESS_CODE,
			OP_COMMAND_USER_ALREADY_ADMIN_CODE,
			PASS_NOT_MATCH_CODE,
			RECORD_COMMAND_SUCCESS_CODE,
			SIGNUP_WAIT_LOGIN_CODE,
			SIGNUP_WAIT_PASS_CODE,
			STATUS_COMMAND_INCORRECT_STATUS_CODE,
			STATUS_COMMAND_ALREADY_SET_CODE,
			STATUS_COMMAND_SUCCESS_CODE,
			SUCCESSFULLY_AUTHORIZED_CODE,
			TABLE_COMMAND_SUCCESS_CODE,
			UNKNOWN_COMMAND_CODE,
			UNMUTE_COMMAND_USER_NOT_MUTED_CODE,
			UNMUTE_COMMAND_SUCCESS_CODE,
			UNMUTE_COMMAND_YOU_UNMUTED_CODE,
			USER_LEFT_CHAT_CODE,
			USER_AUTHORIZED_CODE,
			WHOIH_COMMAND_SUCCESS_CODE,
			INTERNAL_ERROR_CODE
};

enum
{
			TOO_MUCH_ARGS,
			SELF_USE,
			INCORRECT_USERNAME,
			USER_NOT_FOUND,
			USER_OFFLINE,
			INCORRECT_TIME_VALUE,
			INCORRECT_TIME_RANGE,
			INCORRECT_STRING_VALUE
};


int sendall(int s, const char* buf, int* buf_size);
void itoa(int number, char* num_buf, int max_buf_len);
char* concat_addr_port(unsigned long ip, unsigned long port);
int concat_request_strings(char* result, int result_size, const char** query_strings);
unsigned long long get_tick_unix(void);
void clear_stdin(void);
void print_record(char** args, int args_size, int debug_mode);

#endif

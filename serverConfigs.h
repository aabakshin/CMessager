#ifndef SERVERCONFIGS_H_SENTRY
#define SERVERCONFIGS_H_SENTRY

#define CONFIG_NAME											"server.properties"
#define OPS_NAME											"ops.txt"


#define CONFIG_SETTING_DEFAULT_DB_SIZE_NAME					"currentDbSize"
#define CONFIG_SETTING_DEFAULT_DB_SIZE_VALUE				"10"

#define CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME			"defaultUsersdataDbName"
#define CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME_VALUE		"usersdata.dat"

#define CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME		"defaultUsersSessionsDbName"
#define CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME_VALUE	"users_sessions.dat"

enum
{
	CONFIG_STRINGS_NUM							=			 3,
	CONFIG_STRING_SIZE							=			80
};


char** parse_ops_file(int* strings_count);
char** parse_configuration_file(int* strings_count);

#endif

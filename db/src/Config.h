#ifndef CONFIG_H_SENTRY
#define CONFIG_H_SENTRY

#define CONFIG_PATH											"../"
#define CONFIG_NAME											CONFIG_PATH"server.properties"

#define CONFIG_SETTING_DEFAULT_DB_SIZE_NAME					"currentDbSize"
#define CONFIG_SETTING_DEFAULT_DB_SIZE_VALUE				"10"

#define CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME			"defaultUsersdataDbName"
#define CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME_VALUE		"usersdata.dat"

#define CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME		"defaultUsersSessionsDbName"
#define CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME_VALUE	"users_sessions.dat"

#include <stdio.h>

enum
{
	CONFIG_STRINGS_NUM							=			 3,
	CONFIG_STRING_SIZE							=		   100
};

struct ConfigFields {
	int records_num;
	char userinfo_filename[CONFIG_STRING_SIZE/2];
	char usersessions_filename[CONFIG_STRING_SIZE/2];
};
typedef struct ConfigFields ConfigFields;


FILE* read_configuration_file(ConfigFields* cfg);
int write_configuration_file(const ConfigFields* cfg, FILE* cfg_fd);

#endif

#ifndef CONFIG_C_SENTRY
#define CONFIG_C_SENTRY

#include "../Commons.h"
#include "Config.h"
#include "../serverCore.h"
#include "../DateTime.h"


static const char* config_params_names[CONFIG_STRINGS_NUM] =
{
				CONFIG_SETTING_DEFAULT_DB_SIZE_NAME,
				CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME,
				CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME
};

static const char* config_params_values[CONFIG_STRINGS_NUM] =
{
				CONFIG_SETTING_DEFAULT_DB_SIZE_VALUE,
				CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME_VALUE,
				CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME_VALUE
};

static char** parse_configuration_file(int* strings_count, FILE* cfg_fd);


static char** parse_configuration_file(int* strings_count, FILE* cfg_fd)
{
	char cur_time[MAX_TIME_STR_SIZE];
	*strings_count = 0;


	/* пытаемся открыть конфиг-файл */
	int unable_read = 0;
	if ( !(cfg_fd = fopen(CONFIG_NAME, "r+")) )
	{
		unable_read = 1;
		/* если файл не существовал, пытаемся его создать и приводим настройки к дефолтным значениям */
		fprintf(stderr, "[%s] %s Unable to open file \"%s\". Creating new one..\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), WARN_MESSAGE_TYPE, CONFIG_NAME);

		if ( !(cfg_fd = fopen(CONFIG_NAME, "w+")) )
		{
			fprintf(stderr, "[%s] %s Unable to create \"%s\" file. Do you have permission to this?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
			return NULL;
		}
		fprintf(stderr, "[%s] %s Setting all configs to default values..\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE);

		int j = 0;
		while ( j < CONFIG_STRINGS_NUM )
		{
			char cfg_setting[CONFIG_STRING_SIZE] = { 0 };

			int len = strlen(config_params_names[j]);
			int pos = len;
			memcpy(cfg_setting, config_params_names[j], len);
			cfg_setting[pos] = '=';
			pos++;
			cfg_setting[pos] = '\0';

			len = strlen(config_params_values[j]);
			pos += len;
			strncat(cfg_setting, config_params_values[j], len);
			cfg_setting[pos] = '\0';

			fprintf(cfg_fd, "%s\n", cfg_setting);
			j++;
		}

		rewind(cfg_fd);
		*strings_count = j;
	}

	if ( !unable_read )
	{
		int ch;
		while ( (ch = fgetc(cfg_fd)) != EOF )
			if ( ch == '\n' )
				(*strings_count)++;

		rewind(cfg_fd);
	}

	if ( *strings_count != CONFIG_STRINGS_NUM )
	{
		fprintf(stderr, "[%s] %s Incorrect strings number in configuration file \"%s\"! Is it correctly edited?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
		return NULL;
	}

	char** config_strings = calloc(CONFIG_STRINGS_NUM, sizeof(char*));
	if ( !config_strings )
	{
		fprintf(stderr, "[%s] %s An error is occured while allocating memory for \"config_strings\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	int i;
	for ( i = 0; i < CONFIG_STRINGS_NUM; i++ )
	{
		config_strings[i] = calloc(CONFIG_STRING_SIZE, sizeof(char*));
		if ( !config_strings[i] )
		{
			fprintf(stderr, "[%s] %s An error is occured while allocating memory for \"config_strings[%d]\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, i);

			int j;
			for ( j = 0; j < i; j++ )
			{
				free(config_strings[j]);
				config_strings[j] = NULL;
			}
			free(config_strings);
			return NULL;
		}
	}

	/* чтение содержимого конфиг-файла в массив строк */
	i = 0;
	while ( 1 )
	{
		fgets(config_strings[i], CONFIG_STRING_SIZE, cfg_fd);
		if ( feof(cfg_fd) )
			break;

		int len = strlen(config_strings[i]);
		config_strings[i][len-1] = '\0';

		i++;
		if ( i >= CONFIG_STRINGS_NUM )
			break;
	}

	rewind(cfg_fd);

	return config_strings;
}

FILE* read_configuration_file(ConfigFields* cfg)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( cfg == NULL )
	{
		fprintf(stderr, "[%s] %s Unable to parse configuration file. \"cfg\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	printf("[%s] %s Parsing configuration file...\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	int strings_count = 0;

	FILE* cfg_fd = NULL;
	char** config_strings = parse_configuration_file(&strings_count, cfg_fd);

	if ( (config_strings == NULL) || (strings_count == 0) )
	{
		fprintf(stderr, "[%s] %s Unable to parse configuration file. Return value is null!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return NULL;
	}

	char parsed_options[CONFIG_STRINGS_NUM][2][CONFIG_STRING_SIZE/2];
	int i = 0;

	while ( i < CONFIG_STRINGS_NUM )
	{
		char* istr = strtok(config_strings[i], "=");
		int j = 0;
		while ( istr != NULL )
		{
			strcpy(parsed_options[i][j], istr);
			j++;
			if ( j > 1 )
				break;
			istr = strtok(NULL, "=");
		}
		i++;
	}

	for ( i = 0; i < strings_count; i++ )
		free(config_strings[i]);
	free(config_strings);


	if ( strcmp(parsed_options[0][0], CONFIG_SETTING_DEFAULT_DB_SIZE_NAME) != 0 )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[0][0]);
		return NULL;
	}

	if ( strcmp(parsed_options[1][0], CONFIG_SETTING_DEFAULT_USERSDATA_DB_NAME) != 0 )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[1][0]);
		return NULL;
	}

	if ( strcmp(parsed_options[2][0], CONFIG_SETTING_DEFAULT_USERSSESSIONS_DB_NAME) != 0 )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[2][0]);
		return NULL;
	}


	/////////////////////////////////////////////
	int db_records_num = atoi(parsed_options[0][1]);
	if ( db_records_num < 1 )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[0][1]);
		return NULL;
	}
	cfg->records_num = db_records_num;
	/////////////////////////////////////////////


	/////////////////////////////////////////////
	char db_userinfo_filename[CONFIG_STRING_SIZE/2];
	strcpy(db_userinfo_filename, parsed_options[1][1]);
	if ( (strcmp(db_userinfo_filename, "undefined") == 0) || (db_userinfo_filename[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[1][1]);
		return NULL;
	}
	strcpy(cfg->userinfo_filename, db_userinfo_filename);
	/////////////////////////////////////////////


	/////////////////////////////////////////////
	char db_usersessions_filename[CONFIG_STRING_SIZE/2];
	strcpy(db_usersessions_filename, parsed_options[2][1]);
	if ( (strcmp(db_userinfo_filename, "undefined") == 0) || (db_userinfo_filename[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[2][1]);
		return NULL;
	}
	strcpy(cfg->usersessions_filename, db_usersessions_filename);
	/////////////////////////////////////////////

	printf("[%s] %s Done!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return cfg_fd;
}

int write_configuration_file(const ConfigFields* cfg, FILE* cfg_fd)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( cfg == NULL )
	{
		fprintf(stderr, "[%s] %s Unable to parse configuration file. \"cfg\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	char cfg_records_num[100];
	itoa(cfg->records_num, cfg_records_num, 9);

	char* values[] =
	{
			cfg_records_num,
			(char*)cfg->userinfo_filename,
			(char*)cfg->usersessions_filename
	};

	if ( cfg_fd == NULL )
	{
		if ( !(cfg_fd = fopen(CONFIG_NAME, "r+")) )
		{
			fprintf(stderr, "[%s] %s Unable to open \"%s\" config file. Trying to create one for you.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
			if ( !(cfg_fd = fopen(CONFIG_NAME, "w+")) )
			{
				fprintf(stderr, "[%s] %s You don't have permission to create file in this directory.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
				return 0;
			}
		}
	}
	rewind(cfg_fd);

	int i = 0;
	while ( i < CONFIG_STRINGS_NUM )
	{
		char buffer[CONFIG_STRING_SIZE];
		memset(buffer, 0, sizeof(buffer));

		int pos = 0;
		int len = strlen(config_params_names[i]);
		strcpy(buffer, config_params_names[i]);
		pos += len;
		buffer[pos] = '=';
		pos++;
		buffer[pos] = '\0';
		strcat(buffer, values[i]);
		pos += strlen(values[i]);
		buffer[pos] = '\0';

		fprintf(cfg_fd, "%s\n", buffer);
		i++;
	}

	rewind(cfg_fd);

	return 1;
}

#endif

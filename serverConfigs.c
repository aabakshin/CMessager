#ifndef SERVERCONFIGS_C_SENTRY
#define SERVERCONFIGS_C_SENTRY

#include "Commons.h"
#include "serverConfigs.h"
#include "serverCore.h"
#include "DateTime.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


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


char** parse_ops_file(int* strings_count)
{
	char cur_time[MAX_TIME_STR_SIZE];
	*strings_count = 0;
	
	
	/* Подсчёт кол-ва строк в файле */
	FILE* dbops = NULL;
	if ( (dbops = fopen(OPS_NAME, "r")) != NULL )
	{
		int c;
		while ( (c = fgetc(dbops)) != EOF )
			if ( c == '\n' )
				(*strings_count)++;
	}
	else
	{
		fprintf(stderr, "[%s] %s Unable to open file \"%s\"! Is it exist?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, OPS_NAME);
		return NULL;
	}

	if ( *strings_count == 0 )
	{	
		if ( dbops )
			fclose(dbops);
		
		fprintf(stderr, "[%s] %s File \"%s\" is empty!\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), WARN_MESSAGE_TYPE, OPS_NAME);
		return NULL;
	}
	/*------------------------------*/

	fseek(dbops, 0, SEEK_SET);
	
	/* Подсчёт длин выявленных строк и сохранение длин в массив */
	int old_size = *strings_count;
	int* strings_length = malloc( old_size * sizeof(int) );
	int i;
	for ( i = 0; i < *strings_count; i++ )
		strings_length[i] = 0;

	i = 0;
	int c;
	while ( (c = fgetc(dbops)) != EOF )
	{
		if ( (c != '\n') && (c != ' ') )
				strings_length[i] += 1;
		
		if ( c == '\n' )
			i++;
	}

	for ( i = 0; i < old_size; i++ )
	{
		if ( strings_length[i] == 0 )
			(*strings_count)--;
	}	
	/*---------------------------------------------------------*/

	fseek(dbops, 0, SEEK_SET);

	/* запись строк из файла в результирующий массив строк */
	char** ops_strings = malloc(sizeof(char*) * (*strings_count));
	int j = 0;
	for ( i = 0; i < old_size; i++ )
	{
		if ( strings_length[i] > 0 )
		{
			ops_strings[j] = malloc(sizeof(char)*strings_length[i]+1);
			j++;
		}
	}

	j = 0;
	i = 0;
	int k = 0;

	while ( (c = fgetc(dbops)) != EOF )
	{
		if ( strings_length[k] < 1 )
		{
			if (c == '\n')
				k++;

			continue;
		}
		
		if ( c != '\n' )
		{
			if (c == ' ')
				continue;

			ops_strings[i][j] = c;
			j++;
		}
		else
		{
			ops_strings[i][j] = '\0';
			j = 0;
			i++;
			k++;
		}
	}
	/*------------------------------------------------------*/

	if ( strings_length )
		free(strings_length);

	if ( dbops )
		fclose(dbops);

	return ops_strings;
}


char** parse_configuration_file(int* strings_count)
{
	char cur_time[MAX_TIME_STR_SIZE];
	*strings_count = 0;


	/* пытаемся открыть конфиг-файл */
	int f_closed = 0;
	FILE* cfg_ptr = NULL;
	if ( !(cfg_ptr = fopen(CONFIG_NAME, "r")) )
	{
		/* если файл не существовал, пытаемся его создать и приводим настройки к дефолтным значениям */
		fprintf(stderr, "[%s] %s Unable to open file \"%s\". Creating new one..\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), WARN_MESSAGE_TYPE, CONFIG_NAME);

		if ( !(cfg_ptr = fopen(CONFIG_NAME, "w")) )
		{
			fprintf(stderr, "[%s] %s Unable to create \"%s\" file. Do you have permission to this?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
			return NULL;
		}
		fprintf(stderr, "[%s] %s Setting all configs to default values..\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), INFO_MESSAGE_TYPE);
	
		int j = 0;
		while ( j < CONFIG_STRINGS_NUM )
		{
			char cfg_setting[100] = { 0 };
			
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

			fprintf(cfg_ptr, "%s\n", cfg_setting);

			j++;
		}
		
		if ( cfg_ptr )
			fclose(cfg_ptr);
		
		f_closed = 1;

		*strings_count = j;
	}

	if ( f_closed )
	{
		if ( !(cfg_ptr = fopen(CONFIG_NAME, "r")) )
		{
			fprintf(stderr, "[%s] %s Unable to open file \"%s\". Is it exist?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
			return NULL;
		}
	}
	else
	{
		int ch;
		while ( (ch = fgetc(cfg_ptr)) != EOF )
			if ( ch == '\n' )
				(*strings_count)++;
	
		rewind(cfg_ptr);
	}

	if ( *strings_count != CONFIG_STRINGS_NUM )
	{
		fprintf(stderr, "[%s] %s Incorrect strings number in configuration file \"%s\"! Is it correctly edited?\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
		
		if ( cfg_ptr )
			fclose(cfg_ptr);

		return NULL;
	}

	char** config_strings = calloc(CONFIG_STRINGS_NUM, sizeof(char*));
	if ( !config_strings )
	{
		fprintf(stderr, "[%s] %s An error is occured while allocating memory for \"config_strings\"\n", get_time_str(cur_time, MAX_TIME_STR_SIZE), ERROR_MESSAGE_TYPE);
		
		if ( cfg_ptr )
			fclose(cfg_ptr);

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
			
			if ( cfg_ptr )
				fclose(cfg_ptr);

			return NULL;
		}
	}

	/* чтение содержимого конфиг-файла в массив строк */
	i = 0;
	while ( !feof(cfg_ptr) )
	{
		fgets(config_strings[i], CONFIG_STRING_SIZE, cfg_ptr);
		i++;
		if ( i >= CONFIG_STRINGS_NUM )
			break;
	}
	if ( cfg_ptr )
		fclose(cfg_ptr);

	/*	printf("*config_strings = %p\n", config_strings);	*/
	
	return config_strings;
}

int read_configuration_file(ConfigFields* cfg)
{
	char cur_time[CURRENT_TIME_SIZE];

	if ( cfg == NULL )
	{
		fprintf(stderr, "[%s] %s Unable to parse configuration file. \"cfg\" is NULL!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	printf("[%s] %s Parsing configuration file...\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);
	int strings_count = 0;
	char** config_strings = parse_configuration_file(&strings_count);

	if ( (config_strings == NULL) || (strings_count == 0) )
	{
		fprintf(stderr, "[%s] %s Unable to parse configuration file. Return value is null!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
		return 0;
	}

	char parsed_options[CONFIG_STRINGS_NUM][2][100];
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


	/////////////////////////////////////////////
	int db_records_num = atoi(parsed_options[0][1]);
	if ( db_records_num < 1 )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[0][1]);
		return 0;
	}
	cfg->records_num = db_records_num;
	/////////////////////////////////////////////

	
	/////////////////////////////////////////////
	char db_userinfo_filename[100];
	strcpy(db_userinfo_filename, parsed_options[1][1]);
	if ( (strcmp(db_userinfo_filename, "undefined") == 0) || (db_userinfo_filename[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[1][1]);
		return 0;
	}
	strcpy(cfg->userinfo_filename, db_userinfo_filename);
	/////////////////////////////////////////////

	
	/////////////////////////////////////////////
	char db_usersessions_filename[100];
	strcpy(db_usersessions_filename, parsed_options[2][1]);
	if ( (strcmp(db_userinfo_filename, "undefined") == 0) || (db_userinfo_filename[0] == '\0') )
	{
		fprintf(stderr, "[%s] %s Incorrect value in \"%s\" parameter!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, parsed_options[2][1]);
		return 0;
	}
	strcpy(cfg->usersessions_filename, db_usersessions_filename);
	/////////////////////////////////////////////

	printf("[%s] %s Done!\n", get_time_str(cur_time, CURRENT_TIME_SIZE), INFO_MESSAGE_TYPE);

	return 1;
}

int write_configuration_file(const ConfigFields* cfg)
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

	FILE* cfgPtr = NULL;
	if ( !(cfgPtr = fopen(CONFIG_NAME, "r+")) )
	{
		fprintf(stderr, "[%s] %s Unable to open \"%s\" config file. Trying to create one for you.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE, CONFIG_NAME);
		if ( !(cfgPtr = fopen(CONFIG_NAME, "w")) )
		{
			fprintf(stderr, "[%s] %s You don't have permission to create file in this directory.\n", get_time_str(cur_time, CURRENT_TIME_SIZE), ERROR_MESSAGE_TYPE);
			return 0;
		}
	}

	int i = 0;
	while ( i < CONFIG_STRINGS_NUM )
	{
		char buffer[1024];
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

		fprintf(cfgPtr, "%s\n", buffer);
	
		i++;
	}

	fclose(cfgPtr);

	return 1;
}

#endif

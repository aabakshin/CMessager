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
		*strings_count = 0;

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


#endif

#ifndef SERVERCONFIGS_C_SENTRY
#define SERVERCONFIGS_C_SENTRY

#include "Commons.h"
#include "serverConfigs.h"
#include "serverCore.h"
#include "DateTime.h"


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

#endif

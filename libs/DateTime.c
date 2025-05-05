#ifndef DATETIME_C_SENTRY
#define DATETIME_C_SENTRY

#define _XOPEN_SOURCE

#include "../includes/DateTime.h"
#include <time.h>
#include <string.h>


char* get_time_str(char* current_time, int time_size)
{
	if ( current_time == NULL )
		return NULL;

	if ( time_size < MAX_TIME_STR_SIZE )
		time_size = MAX_TIME_STR_SIZE;

	time_t cur_time_in_secs = time(0);
	struct tm* cur_time = NULL;
	cur_time = localtime(&cur_time_in_secs);
	strftime( current_time, time_size, "%H:%M:%S", cur_time );

	return current_time;
}

char* get_date_str(char* current_date, int date_size)
{
	if ( current_date == NULL )
		return NULL;

	if (date_size < MAX_DATE_STR_SIZE)
		date_size = MAX_DATE_STR_SIZE;

	time_t cur_time_in_secs = time(0);
	struct tm* cur_time = NULL;
	cur_time = localtime(&cur_time_in_secs);
	strftime(current_date, date_size, "%a %d %b %Y %H:%M:%S", cur_time);

	return current_date;
}

unsigned long long get_date_num(const char *date)
{
	if ( date == NULL )
		return 0;

	struct tm time;
	memset(&time, 0, sizeof(time));

	strptime(date, "%a %d %b %Y %H:%M:%S", &time);

	return mktime(&time);
}

#endif

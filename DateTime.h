#ifndef DATETIME_H_SENTRY
#define DATETIME_H_SENTRY

enum
{
	MAX_TIME_STR_SIZE			=			20,
	MAX_DATE_STR_SIZE			=			40,
	CURRENT_TIME_SIZE			=		   100,
	CURRENT_DATE_BUF_SIZE		=		   100
};

char* get_time_str(char* current_time, int time_size);
char* get_date_str(char* current_date, int date_size);
unsigned long long get_date_num(const char* date);

#endif

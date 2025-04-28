#ifndef DATABASE_MSG_HANDLERS_H_SENTRY
#define DATABASE_MSG_HANDLERS_H_SENTRY

#include <stdio.h>

int db_get_new_record_index(FILE* fd);
int db_userinfo_table_get_size(FILE* fd);
int db_userinfo_table_is_empty(FILE* fd);
int db_userinfo_table_is_full(FILE* fd);
int db_usersessions_table_get_size(FILE* fd);
int db_usersessions_table_is_empty(FILE* fd);
int db_usersessions_table_is_full(FILE* fd);

int db_get_record_index(FILE* fd, const char* search_key);

FILE* db_create_userinfo_table(int records_size, const char* table_name);
FILE* db_create_usersessions_table(int records_size, const char* table_name);
char* db_readline_from_tables(FILE* usr_fd, FILE* sess_fd, const char* search_key);
int db_writeline_to_tables(FILE* usr_fd, FILE* sess_fd, char* writeline);


#endif

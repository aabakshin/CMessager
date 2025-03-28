/* Операции совершаемые с базой данных */

#ifndef DATABASE_MSG_HANDLERS_H_SENTRY
#define DATABASE_MSG_HANDLERS_H_SENTRY


int db_get_new_record_index(const char* table_name);
int db_userinfo_table_get_size(const char* table_name);
int db_userinfo_table_is_empty(const char* table_name);
int db_userinfo_table_is_full(const char* table_name);
int db_usersessions_table_get_size(const char* table_name);
int db_usersessions_table_is_empty(const char* table_name);
int db_usersessions_table_is_full(const char* table_name);

int db_get_record_index(const char* search_key, const char* table_name);

int db_create_userinfo_table(int records_size, const char* table_name);
int db_create_usersessions_table(int records_size, const char* table_name);
char* db_readline_from_tables(const char* search_key, const char* userinfo_table_name, const char* usersessions_table_name);
int db_writeline_to_tables(char* writeline, const char* userinfo_table_name, const char* usersessions_table_name);


#endif

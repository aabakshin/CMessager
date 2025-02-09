#include <stdio.h>
#include <string.h>

enum 
{
	BUFFER_SIZE					=		2048,
	MAX_TOKENS_IN_MESSAGE		=		 100
};

int main(int argc, char** argv)
{
	const char* client_line = "INIT_DB_RECORDS_NUM|10\n";
	char buffer[BUFFER_SIZE];
	int len = strlen(client_line);
	memcpy(buffer, client_line, (len < BUFFER_SIZE) ? len : BUFFER_SIZE-1);

	if ( len < BUFFER_SIZE )
		buffer[len] = '\0';
	else
		buffer[BUFFER_SIZE-1] = '\0';

	int i = 0;
	char* mes_tokens[MAX_TOKENS_IN_MESSAGE] = { NULL };
	char* istr = strtok(buffer, "|");
	while ( istr )
	{
		mes_tokens[i] = istr;
		i++;
		if ( i >= MAX_TOKENS_IN_MESSAGE )
			break;
		istr = strtok(NULL, "|");
	}
	
	for ( i = 0; i < 2; i++ )
		printf("%s\n", mes_tokens[i]);
	putchar('\n');

	return 0;
}

#ifndef SESSION_LIST_H_SENTRY
#define SESSION_LIST_H_SENTRY

enum
{
			ADDRESS_SIZE			=			50,
			BUF_SIZE				=		  2048
};

typedef struct
{
	int fd;
	int buf_used;
	char addr[ADDRESS_SIZE];
	char buf[BUF_SIZE];
} ClientData;

struct Session
{
	ClientData* data;
	struct Session* next;
	struct Session* prev;
};
typedef struct Session Session;


int sess_get_size(Session* list);
void sess_insert(Session** list_ptr, const ClientData* data);
const ClientData* sess_remove(Session** list_ptr, const ClientData* data);
int sess_clear(Session** list_ptr, int clear);
void sess_print(Session* list);

#endif

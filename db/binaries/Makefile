CC = gcc
LIBS_PATH = ../libs
SRCMODULES = $(LIBS_PATH)/Config.c $(LIBS_PATH)/serverDatabase.c ../$(LIBS_PATH)/Commons.c ../$(LIBS_PATH)/DateTime.c $(LIBS_PATH)/SessionList.c $(LIBS_PATH)/DatabaseMsgHandlers.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = db_server
PORT = 7778

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

db_server: db_server.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)

run: db_server
	./$^ $(PORT)

debug: db_server
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(PORT)

clean:
	rm -rf $(OBJMODULES) $(OUTPUT_NAME)

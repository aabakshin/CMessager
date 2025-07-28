CC = gcc
SRC_PATH = ../src
SRCMODULES = $(SRC_PATH)/Config.c $(SRC_PATH)/serverDatabase.c ../$(SRC_PATH)/Commons.c ../$(SRC_PATH)/DateTime.c $(SRC_PATH)/SessionList.c $(SRC_PATH)/DatabaseMsgHandlers.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = db_server
PORT = 7778

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

db_server: db_server.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)
	mv ../src/*.o ../lib/

run: db_server
	./$^ $(PORT)

debug: db_server
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(PORT)

clean:
	rm -rf $(OBJMODULES) $(OUTPUT_NAME)

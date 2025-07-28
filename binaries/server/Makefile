CC = gcc
SRC_PATH = ../../src
LIB_PATH = ../../lib
SRCMODULES = $(SRC_PATH)/serverCore.c $(SRC_PATH)/Commons.c $(SRC_PATH)/DateTime.c $(SRC_PATH)/StringList.c $(SRC_PATH)/serverCommands.c $(SRC_PATH)/serverConfigs.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = server
PORT = 7777

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

server: server.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)
	mv ../../src/*.o ../../lib/

run: server
	./$^ $(PORT)

debug: server
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(PORT)

clean:
	rm -rf $(LIB_PATH)/serverCore.o $(LIB_PATH)/Commons.o $(LIB_PATH)/DateTime.o $(LIB_PATH)/StringList.o $(LIB_PATH)/serverCommands.o $(LIB_PATH)/serverConfigs.o $(OUTPUT_NAME)

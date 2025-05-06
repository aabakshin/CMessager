CC = gcc
LIBS_PATH = ../../libs
SRCMODULES = $(LIBS_PATH)/serverCore.c $(LIBS_PATH)/Commons.c $(LIBS_PATH)/DateTime.c $(LIBS_PATH)/StringList.c $(LIBS_PATH)/serverCommands.c $(LIBS_PATH)/serverConfigs.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = server
PORT = 7777

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

server: server.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)

run: server
	./$^ $(PORT)

debug: server
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(PORT)

clean:
	rm -rf $(OBJMODULES) $(OUTPUT_NAME)

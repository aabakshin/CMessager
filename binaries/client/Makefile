CC = gcc
LIBS_PATH = ../../libs
SRCMODULES = $(LIBS_PATH)/Commons.c $(LIBS_PATH)/clientCore.c $(LIBS_PATH)/DateTime.c $(LIBS_PATH)/Input.c $(LIBS_PATH)/CommandsHistoryList.c $(LIBS_PATH)/Graphics.c $(LIBS_PATH)/DebugUtils.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = client
ADDRESS = 192.168.88.128
PORT = 7777

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

client: client.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)

run: client
	./$^ $(ADDRESS) $(PORT)

debug: client
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(ADDRESS) $(PORT)

clean:
	rm -rf $(OBJMODULES) $(OUTPUT_NAME)

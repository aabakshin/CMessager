CC = gcc
SRC_PATH = ../../src
SRCMODULES = $(SRC_PATH)/Commons.c $(SRC_PATH)/clientCore.c $(SRC_PATH)/DateTime.c $(SRC_PATH)/Input.c $(SRC_PATH)/CommandsHistoryList.c $(SRC_PATH)/Graphics.c $(SRC_PATH)/DebugUtils.c
OBJMODULES = $(SRCMODULES:.c=.o)
CFLAGS = -Wall -g
OUTPUT_NAME = client
ADDRESS = 192.168.88.129
PORT = 7777

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

client: client.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $(OUTPUT_NAME)
	mv ../../src/*.o ../../lib/

run: client
	./$^ $(ADDRESS) $(PORT)

debug: client
	valgrind --leak-check=full -s --track-origins=yes --show-leak-kinds=all ./$^ $(ADDRESS) $(PORT)

clean:
	rm -rf $(OBJMODULES) $(OUTPUT_NAME)

CFLAGS=-Wall -Wextra -std=gnu11 -pedantic -ggdb

heap: main.c
	$(CC) $(CFLAGS) -o heap main.c

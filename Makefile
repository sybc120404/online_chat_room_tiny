CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread
INCLUDES := -Iinc

main : src/server.c
	$(CC) $(CFLAGS) $(INCLUDES) -o server src/server.c $(LDFLAGS)

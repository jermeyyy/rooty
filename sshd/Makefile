
CC ?= gcc
CFLAGS ?= -Wall -Wextra

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o sshd sshd.c -Wl,-Bstatic -lssh -Wl,-Bdynamic -lz -lcrypto -lrt -lutil -lpthread

clean:
	rm -f sshd
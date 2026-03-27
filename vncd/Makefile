
CC ?= gcc
CFLAGS ?= -Wall -Wextra

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o vncd vnc-server.c -Wl,-Bstatic -lvncserver -Wl,-Bdynamic -ljpeg -lz -lpthread -lX11

clean:
	rm -f vncd

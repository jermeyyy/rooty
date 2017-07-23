

all:
	gcc -o sshd sshd.c -Wl,-Bstatic -lssh -Wl,-Bdynamic -lz -lcrypto -lrt -lutil

clean:
	rm sshd
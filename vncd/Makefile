

all:
	gcc -o vncd vnc-server.c -Wl,-Bstatic -lvncserver -Wl,-Bdynamic -ljpeg -lz -lpthread -lX11

clean:
	rm vncd

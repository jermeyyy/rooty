obj-m += rooty.o

all: module tools

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

tools:
	$(MAKE) -C ioctl
	$(MAKE) -C sshd
	$(MAKE) -C vncd

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(MAKE) -C ioctl clean
	$(MAKE) -C sshd clean
	$(MAKE) -C vncd clean

.PHONY: all module tools clean
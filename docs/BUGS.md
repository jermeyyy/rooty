# Bugs and Defects Report

Comprehensive catalog of bugs found across the rooty codebase, organized by severity.

---

## Critical Severity

### 1. x86-64 Incompatible Trampoline (`syscall_table.h`)

`HIJACK_SIZE = 6` uses a `push imm32; ret` trampoline that can only encode 32-bit addresses. On x86-64, kernel addresses are above `0xFFFFFFFF80000000`, so the address is truncated — the hook jumps to an invalid location and causes a kernel panic.

**Fix:** Use a 14-byte `mov rax, imm64; jmp rax` trampoline on x86-64, or use ftrace-based hooking.

### 2. x86-32 Only Syscall Table Discovery (`syscall_table.h`)

`find_sys_call_table()` uses the IDT to find the syscall handler via interrupt `0x80`. The IDT descriptor struct uses 16-bit `off1`/`off2` fields which is x86-32 layout. On x86-64, IDT entries are 16 bytes with a 64-bit offset. This will return garbage on x86-64 systems.

**Fix:** Use `kallsyms_lookup_name("sys_call_table")` (for kernels < 5.7) or kprobes-based lookup for modern kernels.

### 3. `filp_open` Error Check Uses `== NULL` Instead of `IS_ERR()` (`proc_fs_hide.h`, `socket_hide.h`)

`filp_open()` returns `ERR_PTR(-errno)` on failure, not `NULL`. The current check `== NULL` will miss all errors, and the returned error pointer will be dereferenced — causing a kernel oops.

**Affected functions:**
- `get_vfs_iterate()` in `proc_fs_hide.h`
- `get_tcp_seq_show()` in `socket_hide.h`
- `get_udp_seq_show()` in `socket_hide.h`

**Fix:** Replace `== NULL` with `IS_ERR(filep)` and use `PTR_ERR(filep)` to get the error code.

### 4. Buffer Overflow in Keylogger (`keylogger.h`)

`ksym_std()` indexes `ascii[128]` with `param->value & 0xff`, which yields values 0–255. Indices 128–255 are out of bounds. Similarly, `ksym_loc()`, `ksym_num()`, `ksym_arw()`, `ksym_mod()` index 16-element arrays with `val & 0xff` — values ≥ 16 are OOB reads.

**Fix:** Mask the index to the array bounds: `ascii[param->value & 0x7f]`, and `array[val & 0x0f]` for the 16-element arrays.

### 5. `seq->count` Underflow in Socket Hiding (`socket_hide.h`)

`seq->count - TMPSZ` is an unsigned subtraction. If `seq->count < TMPSZ`, the result wraps to a very large positive value, causing `strnstr` to read out of bounds in kernel memory.

**Fix:** Add a bounds check: `if (seq->count < TMPSZ) return ret;`

### 6. SSH Magic Packet Size Mismatch (`ssh.h`)

`payload_size != 4` check should be `!= sizeof(struct magic_icmp)` (which is 10 bytes: 4 magic + 4 IP + 2 port). Since the struct is 10 bytes, the condition `payload_size != 4` is always true for valid packets — the magic ICMP trigger **never fires**.

**Fix:** Change to `payload_size < sizeof(struct magic_icmp)`.

### 7. Unconditional Printk Dereference in `watch_icmp` (`ssh.h`)

The debug `printk` at the top of `watch_icmp` dereferences `payload->magic`, `payload->ip`, and `payload->port` **before** checking if the packet is ICMP or if the payload size is valid. Non-ICMP packets or truncated packets will trigger a kernel oops.

**Fix:** Move the `printk` after all validation checks.

### 8. X11 Display and Image Leak (`vncd/vnc-server.c`)

`refresh()` calls `XOpenDisplay(":0.0")` every frame but never calls `XCloseDisplay()`. Additionally, `XGetImage()` allocates a new `XImage*` each call but `XDestroyImage()` is never called. This leaks both X11 connections and `width*height*4` bytes of memory per frame (~33 MB at 4K resolution per second).

**Fix:** Open the display once, reuse it, and call `XDestroyImage()` after copying pixel data.

### 9. Python 2 Only Script (`ping.py`)

Uses Python 2-only syntax: `print` statement, `0xffffffffL` long literal, `except socket.error, msg:` syntax. Completely broken on Python 3.

**Fix:** Port to Python 3 (use `print()`, remove `L` suffix, use `except socket.error as msg:`).

---

## High Severity

### 10. Memory Leak in `root_me()` (`syscall_table.h`)

`prepare_creds()` is called twice. The first allocation is leaked because `new_cred` is immediately overwritten by the second call.

**Fix:** Remove the first `prepare_creds()` call.

### 11. `hijack_start` Silent Failure (`syscall_table.h`)

If `kmalloc` fails after memory is already patched with the hook trampoline, the hook is active but untracked. `hijack_stop` will never be able to restore the original bytes.

**Fix:** Allocate the `sym_hook` struct *before* patching memory. If allocation fails, return without hooking.

### 12. PID Truncation (`proc_fs_hide.h`)

`hidden_proc.pid` is `unsigned short` (16-bit, max 65535). Linux PIDs can reach `PID_MAX_LIMIT = 4194304` on 64-bit systems. PIDs > 65535 will be silently truncated, hiding the wrong process or failing to hide the target.

**Fix:** Use `pid_t` (signed 32-bit) for the PID field.

### 13. Race Condition on `filldir` Function Pointers (`proc_fs_hide.h`)

`proc_filldir` and `root_filldir` are global static function pointers overwritten in `n_proc_iterate`/`n_root_iterate` without any locking. Concurrent readdir calls will clobber each other's callbacks.

**Fix:** Use a per-call approach (e.g., pass the original filldir via a wrapper struct) or protect with a spinlock.

### 14. Use-After-Free After `rcu_read_unlock()` (`ssh.h`, `vnc.h`)

`pid_task()` returns a pointer valid only under `rcu_read_lock()`. The code calls `rcu_read_unlock()` then uses the task pointer to send a signal — the task could be freed by then.

**Fix:** Perform `send_sig_info()` while still holding `rcu_read_lock()`, or take a reference on the task struct.

### 15. Keylogger Modifier Keys Logged as Arrow Keys (`keylogger.h`)

`ksym_mod()` indexes `arrows[]` instead of `mod[]` — a copy-paste bug. Modifier keys (Shift, Ctrl, etc.) will be logged as arrow key names.

**Fix:** Change `arrows[val & 0x0f]` to `mod[val & 0x0f]` in `ksym_mod()`.

### 16. Keylogger File Position Always Zero (`keylogger.h`)

In `flusher()`, `pos` is a local variable initialized to 0. Every flush overwrites the beginning of the file rather than appending. Historical keylog data is lost.

**Fix:** Make `pos` a persistent (static local or global) variable, or use `O_APPEND` mode when opening the file.

### 17. No `argc` Validation in ioctl Client (`ioctl/ioctl.c`)

Cases 1–12 access `argv[2]` without checking `argc >= 3`. Running `./ioctl 1` (without a PID argument) causes a segfault.

**Fix:** Validate `argc` before accessing `argv[2]` in each case.

### 18. `copy_from_user` Return Value Mishandled (`ioctl.h`)

`copy_from_user` returns the number of bytes NOT copied. The code treats any nonzero return as an error and returns 0 (success) to userspace. Partial copies are silently accepted, and errors report success.

**Fix:** Return `-EFAULT` when `copy_from_user` returns nonzero.

### 19. Netfilter Hook API Change (`ssh.h`)

`nf_hookfn` signature changed in kernel 4.1+ and `nf_register_hook` was removed in kernel 4.15. The current code won't compile on kernels ≥ 4.15.

**Fix:** Use `nf_register_net_hook()` with the updated callback signature.

### 20. `set_fs`/`get_fs`/`get_ds` Removed (`keylogger.h`)

These functions were removed in kernel 5.10+. The keylogger's `vfs_write` approach from kernel space requires `kernel_write()` instead.

**Fix:** Replace `set_fs(get_ds()); vfs_write(...); set_fs(old_fs);` with `kernel_write(...)`.

### 21. No `file_args.namelen` Validation (`ioctl.h`)

The `namelen` field comes from userspace and is passed directly to `kmalloc`. A malicious value can cause excessive kernel memory allocation (denial of service).

**Fix:** Add an upper bound check (e.g., `namelen > PATH_MAX`).

---

## Medium Severity

### 22. Global Linked Lists Without Locking

All linked lists (`hooked_syms`, `hidden_procs`, `hidden_files`, `hidden_tcp4_ports`, etc.) are accessed without any spinlock or mutex. Concurrent operations corrupt the lists.

### 23. `write_cr0` Pinned in Kernel ≥ 5.3 (`syscall_table.h`)

Kernel 5.3+ pins CR0.WP via `native_write_cr0` checks. The `disable_wp`/`restore_wp` functions no longer work.

### 24. `kallsyms_lookup_name` Unexported in Kernel ≥ 5.7 (`proc_fs_hide.h`)

Several functions rely on `kallsyms_lookup_name()` to find internal kernel symbols. This was unexported in kernel 5.7.

### 25. `to_flush` / `logidx` Race in Keylogger (`keylogger.h`)

`to_flush` is `volatile` but not atomic. `logidx` is shared between keyboard notifier (interrupt context) and flusher kthread with no locking.

### 26. `simple_strtol` Deprecated (`proc_fs_hide.h`)

`simple_strtol` has been deprecated since kernel 2.6.39. Use `kstrtol` or `kstrtoint` instead.

### 27. Incorrect Help Text in ioctl Client (`ioctl/ioctl.c`)

Commands 5–10 have swapped descriptions (UDP/TCP v4/v6 are mixed up).

### 28. `isVNCDrunning` Never Reset (`vnc.h`)

`stop_vnc()` does not set `isVNCDrunning = 0`, so the state variable remains stale.

---

## Low Severity

### 29. Socket File Descriptor Leaks (`ioctl/ioctl.c`, `sshd/sshd.c`, `vncd/vnc-server.c`)

All three userspace tools open a socket for ioctl communication and never close it.

### 30. `#include <stdio.h>` Duplicated (`ioctl/ioctl.c`)

stdio.h is included twice.

### 31. `AUTH_TOKEN` Defined in Multiple Headers (`ioctl.h`, `ssh.h`)

Macro defined in two places — risk of divergence.

### 32. Keymap Error: `KEY_LESS` at Position 61 (`keymap.h`)

Position 61 in `ascii[]` should be `KEY_EQUL` (equals sign), not `KEY_LESS`.

### 33. `execl` Failure Not Handled (`ioctl/ioctl.c`)

If `execl("/bin/sh", ...)` fails, execution falls through silently.

### 34. No `XOpenDisplay` NULL Check (`vncd/vnc-server.c`)

`XOpenDisplay(":0.0")` can return NULL on failure, leading to crashes.

# Kernel API Compatibility Report

Analysis of deprecated, removed, and version-specific kernel APIs used in the rooty codebase, with recommended alternatives.

---

## Compatibility Matrix

| API / Feature | Used In | Removed/Changed In | Status |
|---|---|---|---|
| `write_cr0()` WP bit toggle | `syscall_table.h` | Pinned in 5.3+ | **Broken** |
| `kallsyms_lookup_name()` | `proc_fs_hide.h` | Unexported in 5.7+ | **Broken** |
| `set_fs()` / `get_fs()` / `get_ds()` | `keylogger.h` | Removed in 5.10+ | **Broken** |
| `nf_register_hook()` | `ssh.h` | Removed in 4.15 | **Broken** |
| `nf_hookfn` 5-arg signature | `ssh.h` | Changed in 4.1 | **Broken** (≥ 4.1) |
| `readdir` / `filldir_t` VFS API | `proc_fs_hide.h` | Changed to `iterate` in 3.11 | **Broken** (≥ 3.11) |
| `kernel_thread()` (exported) | `proc_fs_hide.h` | Unexported in ~3.x | **Broken** |
| `populate_rootfs_wait()` | `proc_fs_hide.h` | Removed in 3.9 | **Broken** |
| `PDE()` macro | `socket_hide.h` | Removed in 3.10 | **Broken** |
| `filep->f_dentry` | `socket_hide.h` | Removed in 3.19 | **Broken** |
| `simple_strtol()` | `proc_fs_hide.h` | Deprecated since 2.6.39 | Compiles, deprecated |
| `tcp_seq_afinfo` struct layout | `socket_hide.h` | Changed multiple times | **Fragile** |
| IDT-based syscall table lookup | `syscall_table.h` | x86-32 only | **Broken** on x86-64 |
| `kuid_t` comparison without `.val` | `syscall_table.h` | Struct-based since 3.14 | **Broken** (≥ 3.14) |
| DSA SSH keys | `sshd/sshd.c` | OpenSSH deprecated DSA | **Weak/broken** |

---

## Estimated Working Kernel Range

Based on the APIs used, the rootkit is designed for **kernel 3.0–3.10** (approximately) with x86-32 architecture. Most functionality breaks on kernels ≥ 3.11 due to the VFS API change, and all functionality breaks on ≥ 5.3 due to CR0 pinning.

---

## Detailed Remediation Guide

### 1. CR0 Write-Protect Bypass (Kernel 5.3+)

**Problem:** `write_cr0(read_cr0() & ~CR0_WP)` no longer works because the kernel validates CR0 writes and refuses to clear the WP bit.

**Solutions (most to least robust):**
1. **Use ftrace-based hooking** — avoids needing to modify code pages entirely
2. **Use `set_memory_rw()` / `set_memory_ro()`** — change page permissions for specific pages
3. **Patch `update_mapping_prot()`** via kprobes — fragile but works on ARM64 and x86-64

### 2. Syscall Table Discovery (Kernel 5.7+)

**Problem:** `kallsyms_lookup_name` is unexported. IDT approach is x86-32 only.

**Solutions:**
1. **Kprobes trick:** Register a kprobe on `kallsyms_lookup_name`, extract the address from the kprobe struct, then unregister
2. **Scan kernel memory** for the syscall table signature (fragile)
3. **Use livepatch or ftrace** instead of syscall table hooking

```c
// Kprobes-based kallsyms_lookup_name retrieval
static struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
register_kprobe(&kp);
kallsyms_lookup_name_t fn = (kallsyms_lookup_name_t)kp.addr;
unregister_kprobe(&kp);
```

### 3. VFS Directory Iteration (Kernel 3.11+)

**Problem:** `readdir`/`filldir_t` API replaced by `iterate`/`iterate_shared` using `struct dir_context`.

**Solution:** Hook `iterate_shared` and provide a custom `dir_context` with a filtering `actor` callback:

```c
struct dir_context *ctx = /* from iterate_shared args */;
filldir_t real_actor = ctx->actor;
ctx->actor = my_filter;
ret = real_iterate_shared(file, ctx);
ctx->actor = real_actor;
```

### 4. Netfilter Hook Registration (Kernel 4.15+)

**Problem:** `nf_register_hook()` removed. Hook function signature changed.

**Solution:** Use `nf_register_net_hook()`:

```c
// Old (broken):
nf_register_hook(&nfho);

// New:
nf_register_net_hook(&init_net, &nfho);

// New hook signature (4.4+):
unsigned int hook_fn(void *priv,
                     struct sk_buff *skb,
                     const struct nf_hook_state *state);
```

### 5. Kernel File I/O (Kernel 5.10+)

**Problem:** `set_fs()`/`get_fs()`/`get_ds()` removed. `vfs_write` from kernel context no longer works with the address space trick.

**Solution:** Use `kernel_write()` / `kernel_read()`:

```c
// Old (broken):
mm_segment_t old_fs = get_fs();
set_fs(get_ds());
vfs_write(file, buf, len, &pos);
set_fs(old_fs);

// New:
kernel_write(file, buf, len, &pos);
```

### 6. `f_dentry` Removal (Kernel 3.19+)

**Problem:** `filep->f_dentry` member removed.

**Solution:** Use `filep->f_path.dentry` instead.

### 7. `PDE()` Removal (Kernel 3.10+)

**Problem:** `PDE()` macro removed from `proc_fs.h`.

**Solution:** Use `PDE_DATA()` (available 3.10–5.16), or `pde_data()` (5.17+).

### 8. UID Comparison (Kernel 3.14+)

**Problem:** `current->cred->uid == 0` doesn't compile because `uid` is `kuid_t` (a struct).

**Solution:**
```c
uid_eq(current_uid(), GLOBAL_ROOT_UID)
```

### 9. `simple_strtol` Deprecation

**Problem:** Deprecated; does not report conversion errors.

**Solution:** Use `kstrtol()` or `kstrtoint()`.

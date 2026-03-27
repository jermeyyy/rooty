# Rooty Codebase Review — Summary

Comprehensive code review of the rooty Linux rootkit (LKM + userspace tools), created as an attachment to a Bachelor Engineering Thesis.

---

## Documents

| Document | Description |
|---|---|
| [BUGS.md](BUGS.md) | 34 bugs cataloged by severity (9 critical, 12 high, 7 medium, 6 low) |
| [KERNEL_COMPATIBILITY.md](KERNEL_COMPATIBILITY.md) | 15 deprecated/removed kernel APIs with remediation guide |
| [CODING_STANDARDS.md](CODING_STANDARDS.md) | Linux kernel coding style compliance and C project best practices |
| [BUILD_SYSTEM.md](BUILD_SYSTEM.md) | Makefile improvements with complete recommended replacements |
| [SECURITY_ANALYSIS.md](SECURITY_ANALYSIS.md) | Authentication, anti-detection, and memory safety analysis |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Structural refactoring recommendations and priority roadmap |

---

## Key Findings

### Critical Bugs (Immediate Attention)
1. **x86-64 incompatible** — trampoline and syscall table discovery are x86-32 only
2. **`filp_open` dereference of error pointers** — will cause kernel oops on any file operation failure
3. **Buffer overflow in keylogger** — OOB read from 128-element and 16-element arrays
4. **ICMP trigger is non-functional** — size check `!= 4` instead of `sizeof(struct magic_icmp) = 10`
5. **Massive resource leak in VNC** — X11 display and image leak every frame

### Systemic Issues
- **No locking anywhere** — all shared data structures (6 linked lists, 4 function pointers, 5+ shared flags) accessed without synchronization
- **No include guards** — none of the 8 header files have `#ifndef` guards
- **All logic in headers** — functions defined in `.h` files instead of `.c` files
- **No error handling** — `kmalloc`, `filp_open`, `kthread_create`, `copy_from_user` returns uniformly unchecked
- **Code duplication** — port hiding logic copied 4x, daemon management copied 2x, shared structs copied 4x

### Kernel Compatibility
- Estimated working range: **kernel 3.0–3.10, x86-32 only**
- 15 APIs used are deprecated or removed in modern kernels
- Most critical breakages at kernel 3.11 (VFS), 4.15 (netfilter), 5.3 (CR0), 5.7 (kallsyms), 5.10 (set_fs)

### Build System
- Root Makefile has dead variables including a hardcoded personal path
- No unified build for userspace tools
- No compiler warning flags in any Makefile
- No `.gitignore` — built artifacts committed to repository

---

## Statistics

| Metric | Count |
|---|---|
| Total bugs found | 34 |
| Critical severity | 9 |
| High severity | 12 |
| Deprecated kernel APIs | 15 |
| Header files without include guards | 8/8 |
| Files with function definitions in headers | 7/8 |
| Shared data structures without locking | 11+ |
| Duplicated code blocks | ~6 major duplications |
| Hardcoded credentials/secrets | 4 (token, SSH user, SSH pass, username) |

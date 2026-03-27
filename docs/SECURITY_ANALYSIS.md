# Security Analysis

Analysis of security-relevant design decisions in the rooty rootkit codebase from an educational/research perspective.

---

## 1. Authentication Weaknesses

### Hardcoded Authentication Token

`AUTH_TOKEN = 0xDEADC0DE` is used as the sole authentication mechanism for both the IOCTL control interface and the ICMP magic packet trigger. This token is:

- **Plaintext in source** — visible to anyone with source access
- **Plaintext in binaries** — extractable via `strings rooty.ko`, `strings ioctl`
- **Compile-time constant** — cannot be changed without recompilation
- **Identical across all instances** — no per-deployment uniqueness
- **No cryptographic properties** — a well-known test value (`0xDEADC0DE`)

**Impact for educational context:** Demonstrates the concept but trivially defeated by binary analysis.

**Improvement:** Use a configurable token set at load time via module parameter, or derive from a shared secret using a hash function.

### Hardcoded SSH Credentials

`sshd/sshd.c` defines `SSHD_USER "root"` and `SSHD_PASSWORD "test"`. These are:
- Printed to stdout on startup: `"You can login as the user %s with the password %s\n"`
- Logged for every authentication attempt in cleartext: `"User %s wants to auth with pass %s\n"`
- Extractable via `strings sshd`

**Improvement:** Accept credentials via command-line arguments or environment variables.

---

## 2. Privilege Escalation

### `root_me()` Implementation

The `root_me()` function uses `prepare_creds()` / `commit_creds()` to grant root privileges. This is the standard kernel API for credential manipulation, but:

- **No validation of caller identity** — any process that can reach the IOCTL interface gets root
- **No auditing** — no log entry when privileges are escalated
- The only gate is `AUTH_TOKEN`, which is known

**Note:** This is by design for a rootkit, but worth documenting for the academic paper.

---

## 3. ICMP Trigger Attack Surface

### Magic Packet Design

The ICMP trigger in `ssh.h` accepts a specially crafted ICMP echo request containing:
- `magic` (4 bytes) — authentication token
- `ip` (4 bytes) — IP for reverse shell
- `port` (2 bytes) — port for reverse shell

**Issues:**
1. **No encryption** — the magic token, IP, and port are sent in cleartext and visible to any network sniffer
2. **No replay protection** — captured packets can be replayed
3. **No integrity check** — no HMAC or signature
4. **Payload size check is wrong** — `!= 4` instead of `>= sizeof(struct magic_icmp)`, so the trigger is actually broken (see [BUGS.md](BUGS.md))
5. **ICMP is often monitored** — IDS/IPS systems specifically watch for anomalous ICMP payloads

**Improvement for educational demo:** Use a HMAC with a timestamp for replay protection, or use DNS/HTTP covert channels instead.

---

## 4. Information Disclosure

### Kernel Log Messages

The module prints detailed information via `printk`:
- `"rooty: Module loaded"` — reveals rootkit presence
- `"rooty: sys_call_table found at %p"` — leaks KASLR address
- SSH debug messages with credentials

**Improvement:** Remove all `printk` calls in production, or gate them behind a debug flag.

### VNC Without Authentication

`vncd/vnc-server.c` starts a VNC server with:
- No password
- `alwaysShared = true`
- Listening on port 5900 (standard VNC port)

Any host that can reach port 5900 gets full screen view of the target.

---

## 5. Anti-Detection Weaknesses

### Module Hiding

The module hides from `lsmod` via `list_del_init()` and from `sysfs` via `kobject_del()`. However:
- **Not hidden from `/proc/modules`** — only `list_del_init` is called, which removes from the module list, but the module's memory remains allocated and detectable via memory scanning
- **`/sys/module/rooty/`** may still exist briefly before `kobject_del` runs
- **Module taint flags** are still set
- The module's memory footprint is detectable via `/proc/kcore` or `/dev/mem` analysis

### File Hiding

Files are hidden by name only. Detection methods:
- Direct inode access bypasses readdir hooks
- `stat()` on the hidden filename still works (only readdir is hooked)
- Filesystem-level tools (debugfs, fsck) see all files
- `find -inum` can locate hidden files by inode

### Port Hiding

Ports are hidden by removing lines from `/proc/net/tcp*` and `/proc/net/udp*`. Detection:
- `ss` uses `NETLINK_SOCK_DIAG` (not `/proc/net/*`) and is not hooked
- `nmap` scanning from another host reveals open ports
- `netstat` may use `/proc/net/*` (hidden) or netlink (visible)

---

## 6. Memory Safety Concerns

From a security perspective, several bugs create **exploitable conditions** in the rootkit itself:

| Vulnerability | Type | Exploitability |
|---|---|---|
| Buffer overflow in keylogger (`ascii[128]` OOB) | Stack/heap read OOB | Info leak, potential code execution |
| `seq->count - TMPSZ` underflow | Kernel heap read OOB | Info leak |
| Use-after-free in `ssh.h`/`vnc.h` (RCU) | UAF | Potential code execution |
| No `namelen` validation in IOCTL | Kernel heap exhaustion | DoS |
| `filldir` race condition | Race | Kernel crash, potential escalation |

These bugs mean the rootkit itself could be exploited by another attacker to escalate privileges or crash the kernel.

---

## 7. Recommendations Summary

| Area | Current State | Recommended Improvement |
|---|---|---|
| Authentication | Hardcoded `0xDEADC0DE` | Configurable on load, or key-based |
| ICMP channel | Plaintext, no integrity | HMAC + timestamp |
| SSH credentials | Hardcoded `root`/`test` | Runtime-configurable |
| Kernel messages | Verbose, leaks info | Remove or conditional compile |
| VNC access | No authentication | Add VNC password |
| Anti-detection | Basic readdir hooks | Hook netlink, hook stat, rootkit memory encryption |
| Memory safety | Multiple OOB/UAF bugs | Fix all bugs documented in BUGS.md |

# Build System Improvements

Analysis and recommendations for the build infrastructure.

---

## Current State

The project has 4 independent Makefiles with no unified build process:

| Makefile | Builds | Issues |
|---|---|---|
| `Makefile` (root) | Kernel module `rooty.ko` | Dead variables, no userspace targets |
| `ioctl/Makefile` | `ioctl` CLI tool | No warnings, no flags |
| `sshd/Makefile` | `sshd` daemon | No warnings, static linking issues |
| `vncd/Makefile` | `vncd` daemon | No warnings, no pkg-config |

---

## Issues

### 1. Root Makefile Contains Dead Code

```makefile
CC = gcc                                        # Never used (kbuild uses its own)
INCLUDES = -I/home/newhall/include -I../include # Never used, hardcoded personal path
```

### 2. No Unified Build

Running `make` in the root directory only builds the kernel module. Userspace tools must be built manually in each subdirectory.

### 3. No Compiler Warning Flags

None of the userspace Makefiles enable any warning flags. Many of the bugs documented in [BUGS.md](BUGS.md) would be caught by `-Wall -Wextra`.

### 4. Hardcoded Toolchain

All Makefiles hardcode `gcc` instead of using `$(CC)`. No support for cross-compilation or alternative compilers (e.g., `clang`).

### 5. Fragile `clean` Targets

`rm ioctl`, `rm sshd`, `rm vncd` fail with an error if the binary doesn't exist. Should use `rm -f`.

### 6. No `.PHONY` Declarations

`all` and `clean` targets are not declared `.PHONY`, which can cause issues if files named `all` or `clean` exist.

### 7. No Dependency Tracking

Changes to header files don't trigger recompilation.

### 8. No Install/Uninstall Targets

No way to deploy the built artifacts to the target system.

---

## Recommended Root Makefile

```makefile
# Rooty - Linux Rootkit LKM + Userspace Tools
# Top-level Makefile

.PHONY: all clean module tools ioctl sshd vncd install

KDIR   ?= /lib/modules/$(shell uname -r)/build
CC     ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2

# Default: build everything
all: module tools

# Kernel module
module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

# All userspace tools
tools: ioctl sshd vncd

ioctl:
	$(MAKE) -C ioctl/ CC=$(CC) CFLAGS="$(CFLAGS)"

sshd:
	$(MAKE) -C sshd/ CC=$(CC) CFLAGS="$(CFLAGS)"

vncd:
	$(MAKE) -C vncd/ CC=$(CC) CFLAGS="$(CFLAGS)"

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	$(MAKE) -C ioctl/ clean
	$(MAKE) -C sshd/ clean
	$(MAKE) -C vncd/ clean

# Kbuild object
obj-m += rooty.o
```

## Recommended Userspace Makefile Template

Example for `ioctl/Makefile`:

```makefile
.PHONY: all clean

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=
TARGET   = ioctl
SOURCES  = ioctl.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
```

Example for `sshd/Makefile`:

```makefile
.PHONY: all clean

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2
LDFLAGS  = -Wl,-Bstatic -lssh -Wl,-Bdynamic -lz -lcrypto -lutil
TARGET   = sshd
SOURCES  = sshd.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
```

Example for `vncd/Makefile`:

```makefile
.PHONY: all clean

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 $(shell pkg-config --cflags x11 2>/dev/null)
LDFLAGS  = -Wl,-Bstatic -lvncserver -Wl,-Bdynamic \
           -ljpeg -lz -lpthread $(shell pkg-config --libs x11 2>/dev/null || echo -lX11)
TARGET   = vncd
SOURCES  = vnc-server.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
```

---

## Additional Build Improvements

### Add `.gitignore`

The repository contains built artifacts (`rooty.ko`, `ioctl/ioctl`, `sshd/sshd`, `vncd/vncd`, `*.o`, `*.mod.c`, `modules.order`, `Module.symvers`). Add a proper `.gitignore`:

```gitignore
# Kernel module build
*.o
*.ko
*.mod
*.mod.c
*.cmd
modules.order
Module.symvers
.tmp_versions/

# Userspace binaries
ioctl/ioctl
sshd/sshd
vncd/vncd
```

### Add Static Analysis Targets

```makefile
check:
	cppcheck --enable=all --std=c11 ioctl/ioctl.c sshd/sshd.c vncd/vnc-server.c
	sparse -Wno-decl rooty.c

format:
	clang-format -i ioctl/ioctl.c sshd/sshd.c vncd/vnc-server.c
```

### Kernel Version Detection

For educational purposes, add compile-time checks:

```c
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#error "This module requires kernel < 5.7 (kallsyms_lookup_name unexported)"
#endif
```

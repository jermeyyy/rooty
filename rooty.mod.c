#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xab465258, "module_layout" },
	{ 0x81a5e22a, "kobject_del" },
	{ 0xcdffd70a, "sock_release" },
	{ 0xef9e3355, "sock_create" },
	{ 0x806efb2e, "nf_unregister_hook" },
	{ 0x597a8f75, "nf_register_hook" },
	{ 0x5bfc03c3, "unregister_keyboard_notifier" },
	{ 0xaac60944, "kthread_stop" },
	{ 0x4d1cbe1e, "wake_up_process" },
	{ 0x91dd4461, "kthread_create_on_node" },
	{ 0x13b2a946, "register_keyboard_notifier" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x76584577, "call_usermodehelper_exec" },
	{ 0x145a9a89, "call_usermodehelper_setfns" },
	{ 0x4ff1c9bc, "populate_rootfs_wait" },
	{ 0x29720c94, "send_sig_info" },
	{ 0xc2a82694, "pid_task" },
	{ 0xb57b00dd, "find_vpid" },
	{ 0x548dd3f1, "filp_close" },
	{ 0xef38dc8f, "filp_open" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xb5fb870b, "commit_creds" },
	{ 0x941ed3f8, "prepare_creds" },
	{ 0x5152e605, "memcmp" },
	{ 0x37a0cba, "kfree" },
	{ 0x4c2ae700, "strnstr" },
	{ 0x91715312, "sprintf" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xbd6d4a8a, "kmem_cache_alloc_trace" },
	{ 0x4ec0494, "kmalloc_caches" },
	{ 0x53f2d94, "pv_cpu_ops" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0xe45f60d8, "__wake_up" },
	{ 0x50eedeb8, "printk" },
	{ 0x19a9e62b, "complete" },
	{ 0x35af86fa, "call_usermodehelper_freeinfo" },
	{ 0x7e9ebb05, "kernel_thread" },
	{ 0xe007de41, "kallsyms_lookup_name" },
	{ 0x42dc7959, "vfs_write" },
	{ 0x75bb675a, "finish_wait" },
	{ 0x622fa02a, "prepare_to_wait" },
	{ 0x4292364c, "schedule" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xd2965f6f, "kthread_should_stop" },
	{ 0x5359bcde, "current_task" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "360DBC2801F107CEFF8B9CD");

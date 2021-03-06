#include <stdio.h>
#include <fcntl.h>
#ifndef S_SPLINT_S
/*unistd.h breaks some version of SPLint, so exclude it when running lint*/
#include <unistd.h>
#endif
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../serial.h"
#include "../debug.h"

int isSerialPortPresent(const char* name)
{
    int fd;

    fd = open(name, O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        return 0;
    }
    else
    {
        close(fd);
        return 1;
    }
}

#define MAX_SERIAL_PORTS 10

int getSerialPorts(unsigned int* count, char*** portnames)
{
    char         portname[20];
    unsigned int i,j;
    unsigned int internal = 0;
    char*        tmp_names[MAX_SERIAL_PORTS];

    for(i = 1; i < MAX_SERIAL_PORTS; i++)
    {
        snprintf(portname, sizeof(portname), "/dev/ttyS%u", i);
        if(isSerialPortPresent(portname))
        {
            tmp_names[internal] = strdup(portname);
            if(!tmp_names[internal])
            {
                for(j = internal+1; j > 0; j--)
                {
                    free(tmp_names[j-1]);
                }
                return -1;
            }
            internal++;
        }
    }
    if(internal)
    {
        *portnames = calloc(internal, sizeof(char*));
        if(!(*portnames))
        {
            for(j = internal+1; j > 0; j--)
            {
                free(tmp_names[j-1]);
            }
            return -1;
        }
        memcpy((*portnames), tmp_names, sizeof(char*)*internal);
    }
    else
    {
        *portnames = NULL;
    }
    *count = internal;
    return 0;
}

int openport(const char* portname, void** com_port)
{
    int            fd;
    struct termios settings;

    fd = open(portname, O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        return -1;
    }

    bzero(&settings, sizeof(settings));
    settings.c_cflag     = B9600 | CS8 | CLOCAL | CREAD;
    settings.c_iflag     = IGNPAR;
    settings.c_oflag     = 0;
    settings.c_lflag     = 0;
    settings.c_cc[VTIME] = (cc_t)-1;
    settings.c_cc[VMIN]  = 1;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &settings);

    *com_port = (void*)(size_t)fd;
    return 0;
}

size_t write_serial(void* com_port, unsigned char* data, size_t datalen)
{
    int    fd;
    size_t len;

    fd = (int)(size_t)com_port;
    len = write(fd, data, datalen);
    if(len != datalen)
    {
        printf("Last error: %x\n", errno);
    }
    return len;
}

size_t read_serial(void* com_port, unsigned char* data, size_t datalen)
{
    int          fd;

    fd = (int)(size_t)com_port;
    return read(fd, data, datalen);
}

size_t write_verify_read(void* com_port, unsigned char* in, size_t inlen, unsigned char* out, size_t outlen)
{
    size_t        ret;
    unsigned char buf[4096];

    ret = write_serial(com_port, in, inlen);
    if(ret != inlen)
    {
        return (unsigned int)-1;
    }
    ret = read_serial(com_port, buf, inlen+outlen);
    if(ret < inlen || memcmp(in, buf, inlen))
    {
#ifdef __x86_64
        printf("%s: Read %lx bytes\n", __FUNCTION__, ret);
#else
        printf("%s: Read %x bytes\n", __FUNCTION__, ret);
#endif
        printbuffer(buf, ret); 
        return (unsigned int)-1;
    }
    memmove(out, buf+inlen, ret-inlen);
    return ret-inlen;
}

int closeport(void* com_port)
{
    return close((int)(size_t)com_port);
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

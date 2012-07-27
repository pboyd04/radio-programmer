#include <stdio.h>
#include <windows.h>
#include "../serial.h"
#include "../debug.h"

int isSerialPortPresent(const char* name)
{
    HANDLE h;
    
    h = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if(h == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    else
    {
        CloseHandle(h);
        return 1;
    }
}

int getSerialPorts(unsigned int* count, char*** portnames)
{
    char         portname[5];
    unsigned int i,j;

    unsigned int internal = 0;
    char**       int_names = NULL;
    char**       tmp;

    for(i = 1; i < 10; i++)
    {
        sprintf_s(portname, sizeof(portname), "COM%u", i);
        if(isSerialPortPresent(portname))
        {
            if(!int_names)
            {
                int_names = calloc(1, sizeof(char*));
                if(!int_names)
                {
                    return -1;
                }
                int_names[0] = _strdup(portname);
                if(!int_names[0])
                {
                    free(int_names);
                    return -1;
                }
            }
            else
            {
                tmp = calloc(internal+1, sizeof(char*));
                if(!tmp)
                {
                    for(j = internal+1; j > 0; j--)
                    {
                        free(int_names[j-1]);
                    }
                    free(int_names);
                    return -1;
                }
                memcpy(tmp, int_names, sizeof(char*)*internal);
                tmp[internal] = _strdup(portname);
                if(!tmp[internal])
                {
                    for(j = internal+1; j > 0; j--)
                    {
                        free(int_names[j-1]);
                    }
                    free(int_names);
                    free(tmp);
                    return -1;
                }
                free(int_names);
                int_names = tmp;
            }
            internal++;
        }
    }
    *count = internal;
    *portnames = int_names;
    return 0;
}

int openport(const char* portname, void** com_port)
{
    HANDLE       h;
    DCB          dcb;
    COMMTIMEOUTS comTimeOut;
    
    h = CreateFile(portname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 
                   FILE_ATTRIBUTE_NORMAL, 0);
    if(h == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    GetCommState(h, &dcb);
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(h, &dcb);
    //printf("Baud Rate: %x\n", dcb.BaudRate);
    //printf("Handle: %x\n", h);

    comTimeOut.ReadIntervalTimeout = 4294967295;
    comTimeOut.ReadTotalTimeoutMultiplier = 3;
    comTimeOut.ReadTotalTimeoutConstant = 300;
    comTimeOut.WriteTotalTimeoutMultiplier = 3;
    comTimeOut.WriteTotalTimeoutConstant = 2;
    SetCommTimeouts(h, &comTimeOut);

    *com_port = (void*)h;
    return 0;
}

unsigned int write_serial(void* com_port, unsigned char* data, unsigned int datalen)
{
    HANDLE       h;
    unsigned int len = datalen;

    h = (HANDLE)com_port;
    if(!WriteFile(h, data, datalen, (LPDWORD)&len, NULL))
    {
        printf("%s: h = %p, data = %p, datalen = %x\n", __FUNCTION__, h, data, datalen);
        printf("%s: Last error: %x\n", __FUNCTION__, GetLastError());
    }
    return len;
}

unsigned int read_serial(void* com_port, unsigned char* data, unsigned int datalen)
{
    HANDLE       h;
    unsigned int len = 0;

    h = (HANDLE)com_port;
    if(!ReadFile(h, data, datalen, (LPDWORD)&len, NULL))
    {
        printf("%s: h = %p, data = %p, datalen = %x\n", __FUNCTION__, h, data, datalen);
        printf("%s: Last error: %x\n", __FUNCTION__, GetLastError());
    }
    return len;
}

unsigned int write_verify_read(void* com_port, unsigned char* in, unsigned int inlen, unsigned char* out, unsigned int outlen)
{
    unsigned int ret;
    unsigned char buf[4096];

    ret = write_serial(com_port, in, inlen);
    if(ret != inlen)
    {
        return (unsigned int)-1;
    }
    ret = read_serial(com_port, buf, inlen+outlen);
    if(ret < inlen || memcmp(in, buf, inlen))
    {
        printf("%s: Read %x bytes\n", __FUNCTION__, ret);
        printbuffer(buf, ret); 
        return (unsigned int)-1;
    }
    //printbuffer(buf, ret); 
    memmove(out, buf+inlen, ret-inlen);
    return ret-inlen;
}

int closeport(void* com_port)
{
    return CloseHandle((HANDLE)com_port);
}



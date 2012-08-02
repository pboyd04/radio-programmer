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

#define MAX_SERIAL_PORTS 10

int getSerialPorts(_Out_ unsigned int* count, _Out_ char*** portnames)
{
    char         portname[5];
    unsigned int i,j;
    unsigned int internal = 0;
    char*        tmp_names[MAX_SERIAL_PORTS];

    if(!count || !portnames)
    {
        return -1;
    }
    *count = 0;
    *portnames = NULL;

    for(i = 1; i < MAX_SERIAL_PORTS; i++)
    {
        sprintf_s(portname, sizeof(portname), "COM%u", i);
        if(isSerialPortPresent(portname))
        {
            tmp_names[internal] = _strdup(portname);
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

size_t write_serial(void* com_port, unsigned char* data, size_t datalen)
{
    HANDLE h;
    DWORD  len = (DWORD)datalen;

    if(!com_port || !data || !datalen)
    {
        return (unsigned int)-1;
    }

    h = (HANDLE)com_port;
    if(!WriteFile(h, data, (DWORD)datalen, &len, NULL))
    {
        printf("%s: Last error: %x\n", __FUNCTION__, GetLastError());
    }
    return len;
}

size_t read_serial(void* com_port, unsigned char* data, size_t datalen)
{
    HANDLE h;
    DWORD  len = 0;

    if(!com_port || !data || !datalen)
    {
        return (unsigned int)-1;
    }

    h = (HANDLE)com_port;
    if(!ReadFile(h, data, (DWORD)datalen, &len, NULL))
    {
        printf("%s: Last error: %x\n", __FUNCTION__, GetLastError());
    }
    return len;
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

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

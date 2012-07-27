#include <stdio.h>
#include "debug.h"

void printbuffer(unsigned char* buffer, size_t len)
{
    size_t offset;
    size_t i;

    for(offset = 0; offset < len; offset+=8)
    {
        printf("%04X: ", (unsigned int)offset);
        for(i = 0; i < 8 && i+offset < len; i++)
        {
            printf("%02X ", buffer[i+offset]);
        }
        for(i = 0; i < 8 && i+offset < len; i++)
        {
            if(buffer[i+offset] < 0x20 || buffer[i+offset] > 0x7E)
            {
                printf(".");
            }
            else
            {
                printf("%c", buffer[i+offset]);
            }
        }
        printf("\n");
    }
    
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

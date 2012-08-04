#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "serial.h"
#include "motorola_serial.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

void motorola_calculate_checksum(unsigned char* cmd, size_t len)
{
    unsigned char byte = 0;
    size_t        i;

    for(i = 0; i < len; i++)
    {
        byte += cmd[i];
    }
    cmd[len] = (unsigned char)((unsigned char)0xFF - (unsigned char)byte);
}

unsigned int motorola_valid_checksum(unsigned char* res, size_t len)
{
    unsigned char byte = 0;
    size_t        i;

    for(i = 0; i < len; i++)
    {
        byte += res[i];
    }
    if(byte == 0xFF)
    {
        return 1;
    }
    else
    {
        printf("byte = %x\n", byte);
        return 0;
    }
}

ssize_t motorola_send_cmd(void* com_port, unsigned char* cmd, size_t cmdlen, /*@out@*/ unsigned char* res, size_t reslen)
{
    unsigned char full_cmd[256];
    unsigned char full_res[256];
    size_t        full_res_len = 0;
    unsigned int  front_strip  = 1;
    size_t        ret;
    unsigned char status[1];
    size_t        expected;

    if(!cmd || cmdlen == 0 || cmdlen > 255 || !res || reslen == 0)
    {
        if(res)
        {
            res[0] = 0;
        }
        printf("Null param\n");
        return -1;
    }
    memset(res, 0, reslen);

    memcpy(full_cmd, cmd, cmdlen);
    motorola_calculate_checksum(full_cmd, cmdlen);
    ret = write_verify_read(com_port, full_cmd, cmdlen+1, status, 1);
    if(ret != 1 || status[0] != 0x50) /*0x50 seems to be ok... 0x60 seems to be not ok*/
    {
        /*Read any data left on the serial port and start over...*/
        (void)read_serial(com_port, full_res, sizeof(full_res));
        ret = write_verify_read(com_port, full_cmd, cmdlen+1, status, 1);
        if(ret != 1 || status[0] != 0x50) /*0x50 seems to be ok... 0x60 seems to be not ok*/
        {
            printf("%s: Command failed (ret = %x, status[0] = %x)\n", __FUNCTION__, (unsigned int)ret, status[0]);
            printf("Command was: \n");
            printbuffer(full_cmd, cmdlen+1);
            return -1;
        }
    }
    /*The next bit seems to indicate how many bytes are returned...*/
    ret = read_serial(com_port, full_res, 1);
    if(ret != 1)
    {
        printf("%s: Failed to get response length (ret = %x)\n", __FUNCTION__, (unsigned int)ret);
        return -1;
    }
    full_res_len = 1;
    if(full_res[0] == 0xFF)
    {
        /*Special case, return more data in a second command*/
        expected = 3;
    }
    else
    {
        expected = (size_t)((0xF & full_res[0])+1);
    }
    if(reslen < expected)
    {
        printf("%s: Buffer overrun (reslen = %x, expected = %x))\n", __FUNCTION__, (unsigned int)reslen, (unsigned int)expected);
        return -1;
    }
    if(expected >= (sizeof(full_res)-full_res_len))
    {
        printf("%s: Buffer overrun (full_res_len = %x, expected = %x))\n", __FUNCTION__, (unsigned int)full_res_len, (unsigned int)expected);
        return -1;
    }
    ret = read_serial(com_port, full_res+full_res_len, expected);
    if(ret != expected)
    {
        printf("%s: Failed to get expected length (ret = %x)\n", __FUNCTION__, (unsigned int)ret);
        return -1;
    }
    full_res_len += ret;
    if(full_res[0] == 0xFF)
    {
        /* This reponse looks something like 0x8X00NN
         * I don't know what X means
           NN is the bytes in this response */
        expected = (size_t)(full_res[2+(full_res_len-ret)]);
        if(reslen < expected)
        {
            printf("%s: Buffer overrun (reslen = %x, expected = %x))\n", __FUNCTION__, (unsigned int)reslen, (unsigned int)expected);
            return -1;
        }
        ret = read_serial(com_port, full_res+full_res_len, expected);
        if(ret != expected)
        {
            printf("%s: Failed to get second length (ret = %x, expected = %x)\n", __FUNCTION__, (unsigned int)ret, (unsigned int)expected);
            printbuffer(full_res+full_res_len, expected);
            return -1;
        }
        full_res_len += ret;
        front_strip += 3;
    }
    if(motorola_valid_checksum(full_res, full_res_len) == 0)
    {
        printf("%s: Invalid Checksum\n", __FUNCTION__);
        printbuffer(full_res, full_res_len);
        return -1;
    }
    memcpy(res, full_res+front_strip, expected-1);
    return (ssize_t)(expected-1);
}

ssize_t motorola_read_data(void* com_port, size_t offset, unsigned char* res, size_t reslen)
{
    unsigned char cmd[6];
    ssize_t       ret;
    unsigned char bigres[256];

    cmd[0] = 0xF5;
    cmd[1] = 0x11;
    cmd[2] = (unsigned char)(offset >> 24);
    cmd[3] = (unsigned char)(offset >> 16);
    cmd[4] = (unsigned char)(offset >> 8);
    cmd[5] = (unsigned char)offset;
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), bigres, sizeof(bigres));
    if(ret == -1)
    {
        return ret;
    }
    if((size_t)(ret-3) > reslen)
    {
        return -1;
    }
    memcpy(res, bigres+3, (size_t)(ret-3));
    return ret-3;
}

ssize_t motorola_read_data2(void* com_port, size_t start_offset, unsigned int step, size_t length, unsigned char* res, size_t reslen)
{
    size_t  offset;
    size_t  my_offset = 0;
    ssize_t ret;

    for(offset = start_offset; offset < start_offset+length; offset+=step)
    {
        ret = motorola_read_data(com_port, offset, res+my_offset, reslen-my_offset);
        if(ret == -1)
        {
            return ret;
        }
        my_offset+=ret;
    }
    return (ssize_t)my_offset;
}

int motorola_read_fw_ver(void* com_port, /*@out@*/ char** fw_ver)
{
    unsigned char cmd[3];
    ssize_t       ret;
    unsigned char data[12];

    if(!fw_ver)
    {
        /*@-mustdefine@*/
        return -1;
        /*@+mustdefine@*/
    }
    *fw_ver = NULL;
    if(!com_port)
    {
        return -1;
    }

    cmd[0] = 0xF2;
    cmd[1] = 0x23;
    cmd[2] = 0x03;
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), data, sizeof(data));
    if(ret == -1)
    {
        return -1; 
    }
    data[11] = 0;
    /* Data[0] == 0x8B
     * Data[1] == 0x03 
     * I don't know what these mean */
    *fw_ver = strdup((const char*)data+2);
    return 0;
}

int motorola_compatible_check(void* com_port)
{
    unsigned char cmd[3];
    ssize_t       ret;
    unsigned char data[4];

    cmd[0] = 0xF2;
    cmd[1] = 0x23;
    cmd[2] = 0x05;
    /* I think this is some sort capabilities command... 
     * I will need to get other responses than 0x8B05007C to figure out 
     * what this really is...*/
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), data, sizeof(data));
    if(ret != 3)
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        return -1;
    }
    if(data[0] == 0x8B && data[1] == 0x05 && data[2] == 0x00)
    {
        return 0;
    }
    else
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        printbuffer(data, (size_t)ret);
        return -1;
    }
}

int motorola_get_frequency_range(void* com_port, unsigned int* min_freq, unsigned int* max_freq)
{
    unsigned char cmd[3];
    ssize_t       ret;
    unsigned char data[18];

    cmd[0] = 0xF2;
    cmd[1] = 0x23;
    cmd[2] = 0x0F;
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), data, sizeof(data));
    if(ret != 17)
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        return -1;
    }
    /*
     * I don't know how to interpret this yet... so here are my two known values
     */
    if(data[0]  == 0x0F && data[1]  == 0xE5 && data[2]  == 0x8E && data[3]  == 0xC1 &&
       data[4]  == 0x54 && data[5]  == 0x19 && data[6]  == 0x9D && data[7]  == 0x11 &&
       data[8]  == 0xD1 && data[9]  == 0x80 && data[10] == 0x18 && data[11] == 0x00 &&
       data[12] == 0x60 && data[13] == 0xB0 && data[14] == 0x3C && data[15] == 0x8A &&
       data[16] == 0xFF)
    {
        *min_freq = 403;
        *max_freq = 470;
        return 0;
    }
    else if(data[0]  == 0x0F && data[1]  == 0xBC && data[2]  == 0x91 && data[3]  == 0xAC &&
            data[4]  == 0x20 && data[5]  == 0x21 && data[6]  == 0x4D && data[7]  == 0x48 &&
            data[8]  == 0xDD && data[9]  == 0x94 && data[10] == 0xB5 && data[11] == 0x7D &&
            data[12] == 0xC2 && data[13] == 0x99 && data[14] == 0x5A && data[15] == 0x87 &&
            data[16] == 0xAB)
    {
        *min_freq = 450;
        *max_freq = 527;
        return 0;
    }
    else
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        printbuffer(data, (size_t)ret);
        return -1;
    }
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

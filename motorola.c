#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "serial.h"
#include "bytes.h"
#include "debug.h"

#ifdef _MSC_VER
#define strdup _strdup

#pragma comment(lib, "getopt.lib")
#endif

int verbose_flag = 1;

unsigned char first_response[3] = {0x8B, 0x05, 0x00};
unsigned char second_response[17] = {0x0F, 0xE5, 0x8E, 0xC1, 0x54, 0x19, 0x9D, 0x11, 
	                             0xD1, 0x80, 0x18, 0x00, 0x60, 0xB0, 0x3C, 0x8A, 
				     0xFF};

unsigned char big_buff[8192] = {0};

void motorola_calculate_checksum(unsigned char* cmd, unsigned int len)
{
    unsigned char byte = 0;
    unsigned int  i;

    for(i = 0; i < len; i++)
    {
        byte += cmd[i];
    }
    cmd[len] = 0xFF - byte;
}

unsigned int valid_checksum(unsigned char* res, unsigned int len)
{
    unsigned char byte = 0;
    unsigned int  i;
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

unsigned int motorola_send_cmd(void* com_port, unsigned char* cmd, unsigned int cmdlen, unsigned char* res, unsigned int reslen)
{
    unsigned char full_cmd[256];
    unsigned char full_res[256];
    unsigned int  full_res_len = 0;
    unsigned int  front_strip  = 1;
    unsigned int  ret;
    unsigned char status[1];
    unsigned int  expected;

    if(!cmd || cmdlen == 0 || cmdlen > 255 || !res || !reslen)
    {
        return (unsigned int)-1;
    }

    memcpy(full_cmd, cmd, cmdlen);
    motorola_calculate_checksum(full_cmd, cmdlen);
    ret = write_verify_read(com_port, full_cmd, cmdlen+1, status, 1);
    if(ret != 1 || status[0] != 0x50) /*0x50 seems to be ok... 0x60 seems to be not ok*/
    {
        printf("%s: Command failed (ret = %x, status[0] = %x)\n", __FUNCTION__, ret, status[0]);
        printf("Command was: \n");
        printbuffer(full_cmd, cmdlen+1);
        return (unsigned int)-1;
    }
    /*The next bit seems to indicate how many bytes are returned...*/
    ret = read_serial(com_port, full_res, 1);
    if(ret != 1)
    {
        printf("%s: Failed to get response length (ret = %x)\n", __FUNCTION__, ret);
        return (unsigned int)-1;
    }
    full_res_len = 1;
    if(full_res[0] == 0xFF)
    {
        /*Special case, return more data in a second command*/
        expected = 3;
    }
    else
    {
        expected = (0xF & full_res[0])+1;
    }
    if(reslen < expected)
    {
        return (unsigned int)-1;
    }
    if(expected >= (sizeof(full_res)-full_res_len))
    {
        return (unsigned int)-1;
    }
    ret = read_serial(com_port, full_res+full_res_len, expected);
    if(ret != expected)
    {
        printf("%s: Failed to get expected length (ret = %x)\n", __FUNCTION__, ret);
        return (unsigned int)-1;
    }
    full_res_len += ret;
    if(full_res[0] == 0xFF)
    {
        /* This reponse looks something like 0x8X00NN
         * I don't know what X means
           NN is the bytes in this response */
        expected = full_res[2+(full_res_len-ret)];
        if(reslen < expected)
        {
            return (unsigned int)-1;
        }
        ret = read_serial(com_port, full_res+full_res_len, expected);
        if(ret != expected)
        {
            printf("%s: Failed to get second length (ret = %x)\n", __FUNCTION__, ret);
            printbuffer(full_res+full_res_len, expected);
            return (unsigned int)-1;
        }
        full_res_len += ret;
        front_strip += 3;
    }
    if(!valid_checksum(full_res, full_res_len))
    {
        printf("%s: Invalid Checksum\n", __FUNCTION__);
        printbuffer(full_res, full_res_len);
        return (unsigned int)-1;
    }
    memcpy(res, full_res+front_strip, expected-1);
    return expected-1;
}

unsigned int motorola_read_data(void* com_port, unsigned int offset, unsigned char* res, unsigned int reslen)
{
    unsigned char cmd[6];
    unsigned int  ret;
    unsigned char bigres[256];

    cmd[0] = 0xF5;
    cmd[1] = 0x11;
    cmd[2] = (unsigned char)(offset >> 24);
    cmd[3] = (unsigned char)(offset >> 16);
    cmd[4] = (unsigned char)(offset >> 8);
    cmd[5] = (unsigned char)offset;
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), bigres, sizeof(bigres));
    if(ret == (unsigned int)-1)
    {
        return ret;
    }
    if(ret-3 > reslen)
    {
        return (unsigned int)-1;
    }
    memmove(res, bigres+3, ret-3);
    return ret-3;
}

unsigned int motorola_read_data2(void* com_port, unsigned int start_offset, unsigned int step, unsigned int length, unsigned char* res, unsigned int reslen)
{
    unsigned int offset;
    unsigned int my_offset = 0;
    unsigned int ret;

    for(offset = start_offset; offset < start_offset+length; offset+=step)
    {
        ret = motorola_read_data(com_port, offset, res+my_offset, reslen-my_offset);
	if(ret == (unsigned int)-1)
	{
            return ret;
	}
	my_offset+=ret;
    }
    return my_offset;
}

unsigned int motorola_read_fw_ver(void* com_port, char** fw_ver)
{
    unsigned char cmd[3];
    unsigned int  ret;
    unsigned char data[12];

    cmd[0] = 0xF2;
    cmd[1] = 0x23;
    cmd[2] = 0x03;
    ret = motorola_send_cmd(com_port, cmd, sizeof(cmd), data, sizeof(data));
    if(ret == (unsigned int)-1)
    {
        return (unsigned int)-1; 
    }
    data[11] = 0;
    *fw_ver = strdup((const char*)data+2);
    return 0;
}

static struct option long_options[] =
{
    {"verbose", no_argument,       &verbose_flag, 2},
    {"brief",   no_argument,       &verbose_flag, 0},
	{"port",    required_argument, 0,             'p'},
	{"help",    no_argument,       0,             '?'},
	{"version", no_argument,       0,             'V'},
	{0, 0, 0, 0}
};

#define VERSION "0.1"

void help(const char* progname)
{
    printf("Usage: %s [OPTIONS]...\n", progname);
    printf("Utility to read from and write data to Motorola Radios\n");
    printf("\n");
    printf("      --verbose          Increase the verbosity of the output\n");
    printf("      --brief            Decreate the verbosity of the output\n");
    printf("  -p, --port             Specify the COM port to use\n");
    printf("  -?, --help             Display this help message and exit\n");
    printf("  -V, --version          Display the software version\n");
    printf("\n");
}

void version()
{
    printf("Motorola Radio Programmer CLI v%s\n", VERSION);
    printf("Written by Patrick Boyd.\n");
}

int main(int argc, char** argv)
{
    char**        portnames;
    unsigned int  portcount = 0;
    unsigned int  i;
    char*         selected_port = NULL;
    void*         comm;
    unsigned char data[256];
    unsigned char output[256];
    size_t        offset = 0;
    unsigned short first_offset;
    unsigned short next_offset;
    char*          fw_ver;
    int            arg;
    int            opt_index = 0;

    if(argc < 2)
    {
        getSerialPorts(&portcount, &portnames);
        printf("Ports: ");
        for(i = 0; i < portcount; i++)
        {
            printf("%s ", portnames[i]);
        }
        printf("\n");
        if(portcount == 1)
        {
            selected_port = portnames[0];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        while((arg = getopt_long(argc, argv, "p?V", long_options, &opt_index)) != -1)
        {
            switch(arg)
            {
                case '?':
                    help(argv[0]);
                    return 0;
                case 'V':
                    version();
                    return 0;
            }
        }
    }

    openport(selected_port, &comm);
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x05;
    i = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    /*This is some sort capabilities command... I will need to get other responses than
     *0x8B05007C to figure out what this really is...*/
    if(i != sizeof(first_response) || memcmp(first_response, output, i))
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, i);
        if(i != (unsigned int)-1)
        {
            printbuffer(output, i);
        } 
        return -1;
    }
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x0F;
    i = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    if(i != sizeof(second_response) || memcmp(second_response, output, i))
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, i);
        if(i != (unsigned int)-1)
        {
            printbuffer(output, i);
        }
        return -1; 
    }
    i = motorola_read_data(comm, 0x02000000, output, sizeof(output)); 
    if(i == (unsigned int)-1)
    {
        return -1;
    }
    first_offset = be16toh_array(output);
    i = motorola_read_data(comm, 0x02000000+first_offset, output, sizeof(output)); 
    if(i == (unsigned int)-1)
    { 
        return -1;
    }
    next_offset = be16toh_array(output)+first_offset;
    i = motorola_read_data(comm, 0x02000000+next_offset, output, sizeof(output)); 
    if(i == (unsigned int)-1)
    {
        return -1;
    }
    else
    {
        memcpy(big_buff+offset, output, i);
        offset+=i;
    }
    i = motorola_read_data(comm, 0x02000000+next_offset+2, output, sizeof(output)); 
    if(i == (unsigned int)-1)
    { 
        return -1;
    }
    else
    {
        memcpy(big_buff+offset, output, i);
        offset+=i;
    }
    i = motorola_read_data2(comm, 0x20000280, 0x20, 0x1760, big_buff, sizeof(big_buff));
    if(i == (unsigned int)-1)
    {
        return -1;
    }
    else
    {
        offset+=i;
    }
    printbuffer(big_buff, offset);
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x0E;
    i = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, i);
    if(i != (unsigned int)-1)
    {
        printbuffer(output, i);
    }
    motorola_read_fw_ver(comm, &fw_ver);
    printf("fw_ver = %s\n", fw_ver);

    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

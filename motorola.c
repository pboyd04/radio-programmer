#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "serial.h"
#include "bytes.h"
#include "debug.h"
#include "motorola.h"

#ifdef _MSC_VER
#include <BaseTsd.h>
#define strdup _strdup

typedef SSIZE_T ssize_t;

#pragma comment(lib, "getopt.lib")
#endif

int verbose_flag = 1;

unsigned char first_response[3] = {0x8B, 0x05, 0x00};
unsigned char second_response[17] = {0x0F, 0xE5, 0x8E, 0xC1, 0x54, 0x19, 0x9D, 0x11, 
                                     0xD1, 0x80, 0x18, 0x00, 0x60, 0xB0, 0x3C, 0x8A, 
                                     0xFF};

unsigned char big_buff[8192];

motorola_ht1250 buff;

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

unsigned int valid_checksum(unsigned char* res, size_t len)
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
        return -1;
    }
    if(expected >= (sizeof(full_res)-full_res_len))
    {
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
    if(valid_checksum(full_res, full_res_len) == 0)
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

typedef struct
{
    const char*   str;
    unsigned char value;
} code_str_map;

/*@ignore@*/
static struct option long_options[] =
{
    {"verbose", no_argument,       &verbose_flag, 2},
    {"brief",   no_argument,       &verbose_flag, 0},
	{"port",    required_argument, 0,             'p'},
	{"help",    no_argument,       0,             '?'},
	{"version", no_argument,       0,             'V'},
	{0, 0, 0, 0}
};
/*@end@*/

/*@constant static char* VERSION;@*/
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
    char**        portnames = NULL;
    unsigned int  portcount = 0;
    ssize_t       ret;
    unsigned int  i, j;
    char*         selected_port = NULL;
    void*         comm;
    unsigned char data[256];
    unsigned char output[256];
    size_t        offset = 0;
    size_t        first_offset;
    size_t        next_offset;
    char*          fw_ver;
    int            arg;
    int            opt_index = 0;
    unsigned char* tmp_ptr;
    motorola_string_struct* string_struct;
    char*          string_ptr;
    char*          string_storage;
    motorola_frequency* freq;
    motorola_scan_list* scan;
    motorola_scan_list_members* scan_members;

    memset(big_buff, 0, sizeof(big_buff));

    if(argc < 2)
    {
        if((getSerialPorts(&portcount, &portnames) != 0) || portnames == NULL)
        {
            fprintf(stderr, "Error obtaining port names for system\n");
            return -1;
        }
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
            for(i = 0; i < portcount; i++)
            {
                free(portnames[i]);
            }
            free(portnames);
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
    if(selected_port == NULL)
    {
        fprintf(stderr, "No port selected!\n");
        if((getSerialPorts(&portcount, &portnames) != 0) || portnames == NULL)
        {
            fprintf(stderr, "Error obtaining port names for system\n");
            return -1;
        }
        fprintf(stderr, "Valid Ports: ");
        for(i = 0; i < portcount; i++)
        {
            fprintf(stderr, "%s ", portnames[i]);
        }
        fprintf(stderr, "\n");
        for(i = 0; i < portcount; i++)
        {
            free(portnames[i]);
        }
        free(portnames);
        return -1;
    }

    if(openport(selected_port, &comm) != 0)
    {
        fprintf(stderr, "Error opening serial port %s!\n", selected_port);
        return -1;
    }
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x05;
    ret = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    /*This is some sort capabilities command... I will need to get other responses than
     *0x8B05007C to figure out what this really is...*/
    if((size_t)ret != sizeof(first_response) || (memcmp(first_response, output, (size_t)ret) != 0))
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        if(ret != -1)
        {
            printbuffer(output, (size_t)ret);
        } 
        return -1;
    }
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x0F;
    ret = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    if((size_t)ret != sizeof(second_response) || (memcmp(second_response, output, (size_t)ret) != 0))
    {
        printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
        if(ret != -1)
        {
            printbuffer(output, (size_t)ret);
        }
        return -1; 
    }
    ret = motorola_read_data(comm, 0x02000000, output, sizeof(output)); 
    if(ret == -1)
    {
        return -1;
    }
    first_offset = (size_t)be16toh_array(output);
    ret = motorola_read_data(comm, 0x02000000+first_offset, output, sizeof(output)); 
    if(ret == -1)
    { 
        return -1;
    }
    next_offset = (size_t)be16toh_array(output)+first_offset;
    ret = motorola_read_data(comm, 0x02000000+next_offset, output, sizeof(output)); 
    if(ret == -1)
    {
        return -1;
    }
    else
    {
        memcpy(big_buff+offset, output, (size_t)ret);
        offset+=ret;
    }
    ret = motorola_read_data(comm, 0x02000000+next_offset+2, output, sizeof(output)); 
    if(ret == -1)
    { 
        return -1;
    }
    else
    {
        memcpy(big_buff+offset, output, (size_t)ret);
        offset+=ret;
    }
    ret = motorola_read_data2(comm, 0x20000280, 0x20, 0x1760, big_buff, sizeof(big_buff));
    if(ret == -1)
    {
        return -1;
    }
    else
    {
        offset+=(size_t)ret;
    }
    data[0] = 0xF2;
    data[1] = 0x23;
    data[2] = 0x0E;
    ret = motorola_send_cmd(comm, data, 3, output, sizeof(output));
    printf("%s(%d): Read %x bytes\n", __FUNCTION__, __LINE__, (unsigned int)ret);
    if(ret != -1)
    {
        printbuffer(output, (size_t)ret);
    }
    (void)motorola_read_fw_ver(comm, &fw_ver);
    printf("fw_ver = %s\n", fw_ver);
    memcpy(&buff, big_buff, sizeof(buff));
    printbuffer(buff.unknown, sizeof(buff.unknown));
    printf("Serial Number = %s\n", buff.serial_num);
    printf("Model Number = %s\n", buff.model_num);
    printf("Original Programing\n");
    printf("    Code Plug Major Version = %x\n", buff.original.code_plug_major);
    printf("    Code Plug Minor Version = %x\n", buff.original.code_plug_minor);
    printf("    Code Plug Source = %x\n", buff.original.source);
    printf("    Date/Time = %x/%x/%02x%02x %02x:%02x\n", buff.original.date_time.month, buff.original.date_time.day,
                                                     buff.original.date_time.year[0], buff.original.date_time.year[1],
                                                     buff.original.date_time.hour, buff.original.date_time.min);
    printbuffer(buff.unknown2, sizeof(buff.unknown2));
    printf("Latest Programing\n");
    printf("    Code Plug Major Version = %x\n", buff.last.code_plug_major);
    printf("    Code Plug Minor Version = %x\n", buff.last.code_plug_minor);
    printf("    Code Plug Source = %x\n", buff.last.source);
    printf("    Date/Time = %x/%x/%02x%02x %02x:%02x\n", buff.last.date_time.month, buff.last.date_time.day,
                                                     buff.last.date_time.year[0], buff.last.date_time.year[1],
                                                     buff.last.date_time.hour, buff.last.date_time.min);
    printbuffer(buff.unknown3, sizeof(buff.unknown3));
    printf("Personality Assignments\n");
    printf("    Number of Personality Assignments %d\n", buff.personality_assignments.assignment_count);
    printf("    Unknown %02x %02x\n", buff.personality_assignments.unknown[0], 
                                      buff.personality_assignments.unknown[1]); 
    for(i = 0; i < buff.personality_assignments.assignment_count; i++)
    {
        printf("    Personality[%u] = %x\n", i, buff.personality_assignments.entries[i].personality);
        printf("    Unknown1[%u] = %x\n", i, buff.personality_assignments.entries[i].unknown[0]);
        printf("    Unknown2[%u] = %x\n", i, buff.personality_assignments.entries[i].unknown[1]);
    }
    tmp_ptr = (unsigned char*)&(buff.personality_assignments.entries[i]);
    printbuffer(tmp_ptr, 2);
    tmp_ptr += 2;
    string_struct = (motorola_string_struct*)tmp_ptr;
    printf("Personality Assignment Strings\n");
    printf("    Size of Personality Assignment Strings %d\n", string_struct->string_size);
    printf("    Number of Personality Assignment Strings %d\n", string_struct->string_count);
    printf("    Unknown %02x %02x %02x\n", string_struct->unknown[0], string_struct->unknown[1], string_struct->unknown[2]);
    string_storage = calloc((size_t)(string_struct->string_size+1), sizeof(char));
    if(!string_storage)
    {
        fprintf(stderr, "Error allocating memory\n");
        abort();
    }
    string_ptr = string_struct->string;
    for(i = 0; i < string_struct->string_count; i++)
    {
        memcpy(string_storage, string_ptr, (size_t)string_struct->string_size);
        string_ptr+=string_struct->string_size;
        printf("String[%u] = %s\n", i, string_storage);
    }
    tmp_ptr = (unsigned char*)string_ptr;
    printbuffer(tmp_ptr, 0x27B);
    tmp_ptr += 0x27B;
    arg = *tmp_ptr;
    printf("Scan List Count = %d\n", *tmp_ptr);
    tmp_ptr++;
    for(i = 0; i < (unsigned int)arg; i++)
    {
        scan = (motorola_scan_list*)tmp_ptr;
        tmp_ptr+=sizeof(motorola_scan_list);
        printf("Scan List %u\n", i);
        printf("    PL Scan Lockout              = %x\n", scan->options.bitfield.pl_scan_lockout);
        printf("    Unknown1                     = %x\n", scan->options.bitfield.unknown1);
        printf("    PL Scan Type                 = %x\n", scan->options.bitfield.pl_scan_type);
        printf("    Signaling Type               = %x\n", scan->options.bitfield.signaling_type);
        printf("    User Programmable            = %x\n", scan->options.bitfield.user_programmable);
        printf("    Nuisance Delete              = %x\n", scan->options.bitfield.nuisance_delete);
        printf("    Sample Time                  = %x\n", scan->options.bitfield.sample_time);
        printf("    Priority 2                   = %x\n", scan->options.bitfield.priority2);
        printf("    Priority 1                   = %x\n", scan->options.bitfield.priority1);
        printf("    Unknown15                    = %x\n", scan->options.bitfield.unknown15);
        printf("    Signaling Hold Time          = %x\n", scan->options.bitfield.signaling_hold_time);
        printf("    Tx Personality Type          = %x\n", scan->options.bitfield.personality);
        printf("    Talkback                     = %x\n", scan->options.bitfield.talkback);
        printf("    Trunk Group                  = %x\n", scan->options.bitfield.trunk_group);
        printf("    Unknown31                    = %x\n", scan->options.bitfield.unknown31);
        printf("    Tx Conventional Personality  = %x\n", scan->tx_conventional_personality);
        printf("    LS Scan Activity Search Time = %x\n", scan->ls_scan_search_time);
        printf("    Unknown                      = %x\n", scan->unknown);
    }
    for(i = 0; i < (unsigned int)arg; i++)
    {
        scan_members = (motorola_scan_list_members*)tmp_ptr;
        tmp_ptr+=sizeof(motorola_scan_list_members);
        printf("Scan List %u Members\n", i);
        printbuffer(scan_members->unknown, sizeof(scan_members->unknown));
        printf("    Member Entry Count %u\n", scan_members->entry_count);
        for(j = 0; j < scan_members->entry_count; j++)
        {
            printf("    Member Personality %u Type = %x\n", j, scan_members->entires[j].personality_type);
            printf("    Member Personality %u ID   = %x\n", j, scan_members->entires[j].personality_id);
        }
        tmp_ptr+=sizeof(motorola_scan_list_member_entry)*(j-1);
    }
    offset = (size_t)(0xA7 - (0x07*arg));
    printbuffer(tmp_ptr, offset);
    tmp_ptr += offset;
    arg = *tmp_ptr;
    tmp_ptr++;
    for(i = 0; i < (unsigned int)arg; i++)
    {
        freq = (motorola_frequency*)tmp_ptr;
        printf("Frequency %u\n", i);
        printbuffer(freq->unknown, sizeof(freq->unknown));
        printf("    Unknown %x\n", freq->unknown2);
        printf("    Bandwidth = ");
        switch(freq->bandwidth)
        {
            case 0x5:
                printf("12.5 MHz\n");
                break;
            case 0xA:
                printf("20 MHz\n");
                break;
            case 0xF:
                printf("25 MHz\n");
                break;
            default:
                printf("Unknown %x\n", freq->bandwidth);
                break;
        }
        printf("    Tx Squlech Type = ");
        switch(freq->tx_squelch_type)
        {
            case 0x00:
                printf("CSQ\n");
                break;
            case 0x02:
                printf("TPL\n");
                break;
            case 0x08:
                printf("DPL\n");
                break;
            default:
                printf("Unknown %x\n", (*tmp_ptr));
                break;
        }
        printf("    Tx Squelch Code = %03o\n", freq->tx_squelch_code);
        printf("    Rx Squlech Type = ");
        switch(freq->rx_squelch_type)
        {
            case 0x00:
                printf("CSQ\n");
                break;
            case 0x02:
                printf("TPL\n");
                break;
            case 0x08:
                printf("DPL\n");
                break;
            default:
                printf("Unknown %x\n", (*tmp_ptr));
                break;
        }
        printf("    Rx Squelch Code = %03o\n", freq->rx_squelch_code);
        printf("    Additional Squelch Settings = %x\n", freq->additional_squelch.dword);
        printf("        Busy Channel Lockout       = %x\n", freq->additional_squelch.bitfield.busy_channel_lockout);
        printf("        Rx DPL Invert              = %x\n", freq->additional_squelch.bitfield.rx_dpl_invert);
        printf("        Tx DPL Invert              = %x\n", freq->additional_squelch.bitfield.tx_dpl_invert);
        printf("        Unknown4                   = %x\n", freq->additional_squelch.bitfield.unknown3);
        printf("        Unknown5                   = %x\n", freq->additional_squelch.bitfield.unknown4);
        printf("        Unknown6                   = %x\n", freq->additional_squelch.bitfield.unknown5);
        printf("        Unknown7                   = %x\n", freq->additional_squelch.bitfield.unknown6);
        printf("        Mute/Unmute Rule           = %x\n", freq->additional_squelch.bitfield.unmute_mute_rule);
        printf("        Squelch Setting            = %x\n", freq->additional_squelch.bitfield.squelch_setting);
        printf("        Tx DPL Turn-Off            = %x\n", freq->additional_squelch.bitfield.tx_turn_off_code);
        printf("        Rx Only                    = %x\n", freq->additional_squelch.bitfield.rx_only);
        printf("        Talkaround                 = %x\n", freq->additional_squelch.bitfield.talkaround);
        printf("        Auto Scan                  = %x\n", freq->additional_squelch.bitfield.auto_scan);
        printf("        Unknown15                  = %x\n", freq->additional_squelch.bitfield.unknown11);
        printf("        Timeout                    = %x\n", freq->additional_squelch.bitfield.timeout);
        printf("        Unknown19                  = %x\n", freq->additional_squelch.bitfield.unknown15);
        printf("        Unknown20                  = %x\n", freq->additional_squelch.bitfield.unknown16);
        printf("        Unknown21                  = %x\n", freq->additional_squelch.bitfield.unknown17);
        printf("        Tx Voice Op                = %x\n", freq->additional_squelch.bitfield.tx_voice_operated);
        printf("        Non-Standard Reverse Burst = %x\n", freq->additional_squelch.bitfield.non_standard_reverse_burst); 
        printf("        Tx High Power              = %x\n", freq->additional_squelch.bitfield.tx_high_power);
        printf("        Tx Auto Power              = %x\n", freq->additional_squelch.bitfield.tx_auto_power);
        printf("        Unknown26                  = %x\n", freq->additional_squelch.bitfield.unknown21);
        printf("        Unknown27                  = %x\n", freq->additional_squelch.bitfield.unknown22);
        printf("        Unknown28                  = %x\n", freq->additional_squelch.bitfield.unknown23);
        printf("        Unknown29                  = %x\n", freq->additional_squelch.bitfield.unknown24);
        printf("        Compression Type           = %x\n", freq->additional_squelch.bitfield.compression_type);
        printbuffer(freq->unknown3, sizeof(freq->unknown3));
        printf("\n");
        tmp_ptr+=sizeof(motorola_frequency);
    }
    printbuffer(tmp_ptr, sizeof(buff.unknown4)-(tmp_ptr - buff.unknown4));

    if(portnames)
    {
        for(i = 0; i < portcount; i++)
        {
            free(portnames[i]);
        }
        free(portnames);
    }

    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

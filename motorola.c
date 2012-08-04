#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "serial.h"
#include "bytes.h"
#include "debug.h"
#include "motorola.h"
#include "motorola_serial.h"

#ifdef _MSC_VER
#define strdup _strdup

#pragma comment(lib, "getopt.lib")
#endif

int verbose_flag = 1;

typedef struct
{
    char*               fw_ver;
    unsigned int        min_freq;
    unsigned int        max_freq;
    union
    {
        motorola_ht1250 ht1250;
        unsigned char   raw[8192];
    }                   buff;
} my_radio_struct;

int motorola_get_data(void* comm, void** radio_data_handle)
{
    ssize_t          ret;
    unsigned char    data[256];
    unsigned char    output[256];
    size_t           offset = 0;
    size_t           first_offset;
    size_t           next_offset;
    my_radio_struct* radio_data;

    if(motorola_compatible_check(comm) != 0)
    {
        fprintf(stderr, "Error compatibility check failed!\n");
        return -1;
    } 
    radio_data = calloc(sizeof(my_radio_struct), 1);
    if(!radio_data)
    {
        return -1;
    }
    if(motorola_get_frequency_range(comm, &(radio_data->min_freq), &(radio_data->max_freq)) != 0)
    {
        fprintf(stderr, "Error Unable to get frequqncy range!\n");
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
        memcpy(radio_data->buff.raw+offset, output, (size_t)ret);
        offset+=ret;
    }
    ret = motorola_read_data(comm, 0x02000000+next_offset+2, output, sizeof(output)); 
    if(ret == -1)
    { 
        return -1;
    }
    else
    {
        memcpy(radio_data->buff.raw+offset, output, (size_t)ret);
        offset+=ret;
    }
    ret = motorola_read_data2(comm, 0x20000280, 0x20, 0x1760, radio_data->buff.raw, sizeof(radio_data->buff.raw));
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
    (void)motorola_read_fw_ver(comm, &(radio_data->fw_ver));
    *radio_data_handle = radio_data;
    return 0;
}

int motorola_get_fw_ver(void* radio_data_handle, char** fw_ver)
{
    my_radio_struct* radio_data;

    if(!radio_data_handle || !fw_ver)
    {
        return -1;
    }
    radio_data = (my_radio_struct*)radio_data_handle;
    *fw_ver = strdup(radio_data->fw_ver);
    return 0;
}

int motorola_get_model_num(void* radio_data_handle, char** model_num)
{
    my_radio_struct* radio_data;

    if(!radio_data_handle || !model_num)
    {
        return -1;
    }
    radio_data = (my_radio_struct*)radio_data_handle;
    *model_num = strdup(radio_data->buff.ht1250.model_num);
    return 0;
}

int motorola_get_serial_num(void* radio_data_handle, char** serial_num)
{
    my_radio_struct* radio_data;

    if(!radio_data_handle || !serial_num)
    {
        return -1;
    }
    radio_data = (my_radio_struct*)radio_data_handle;
    *serial_num = strdup(radio_data->buff.ht1250.serial_num);
    return 0;
}

int motorola_get_cp_info(void* radio_data_handle, int index, motorola_program_info* info)
{
    my_radio_struct* radio_data;

    if(!radio_data_handle || !info)
    {
        return -1;
    }
    radio_data = (my_radio_struct*)radio_data_handle;
    if(index == 0)
    {
        memcpy(info, &(radio_data->buff.ht1250.original), sizeof(motorola_program_info));
    }
    else
    {
        memcpy(info, &(radio_data->buff.ht1250.last), sizeof(motorola_program_info));
    }
    return 0;
}

int motorola_get_freq_range(void* radio_data_handle, unsigned int* min, unsigned int* max)
{
    my_radio_struct* radio_data;

    if(!radio_data_handle || !min || !max)
    {
        return -1;
    }
    radio_data = (my_radio_struct*)radio_data_handle;
    *min = radio_data->min_freq;
    *max = radio_data->max_freq;
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
    unsigned int  i, j;
    char*         selected_port = NULL;
    void*         comm;
    int            arg;
    int            opt_index = 0;
    size_t         offset;
    unsigned char* tmp_ptr;
    motorola_string_struct* string_struct;
    char*          string_ptr;
    char*          string_storage;
    my_radio_struct*    data;
    motorola_frequency* freq;
    motorola_scan_list* scan;
    motorola_scan_list_members* scan_members;

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
    if(motorola_get_data(comm, &data) != 0)
    {
        fprintf(stderr, "Error obtaining data from serial port %s!\n", selected_port);
        closeport(comm);
        return -1;
    }

    motorola_get_freq_range((void*)data, &i, &j);
    
    printf("Maximum Frequency = %u\n", i);
    printf("Minimum Frequency = %u\n", j);
    printf("fw_ver = %s\n", data->fw_ver);
    printf("Serial Number = %s\n", data->buff.ht1250.serial_num);
    printf("Model Number = %s\n", data->buff.ht1250.model_num);
    printf("Original Programing\n");
    printf("    Code Plug Major Version = %x\n", data->buff.ht1250.original.code_plug_major);
    printf("    Code Plug Minor Version = %x\n", data->buff.ht1250.original.code_plug_minor);
    printf("    Code Plug Source = %x\n", data->buff.ht1250.original.source);
    printf("    Date/Time = %x/%x/%02x%02x %02x:%02x\n", data->buff.ht1250.original.date_time.month, data->buff.ht1250.original.date_time.day,
                                                     data->buff.ht1250.original.date_time.year[0], data->buff.ht1250.original.date_time.year[1],
                                                     data->buff.ht1250.original.date_time.hour, data->buff.ht1250.original.date_time.min);
    printbuffer(data->buff.ht1250.unknown2, sizeof(data->buff.ht1250.unknown2));
    printf("Latest Programing\n");
    printf("    Code Plug Major Version = %x\n", data->buff.ht1250.last.code_plug_major);
    printf("    Code Plug Minor Version = %x\n", data->buff.ht1250.last.code_plug_minor);
    printf("    Code Plug Source = %x\n", data->buff.ht1250.last.source);
    printf("    Date/Time = %x/%x/%02x%02x %02x:%02x\n", data->buff.ht1250.last.date_time.month, data->buff.ht1250.last.date_time.day,
                                                     data->buff.ht1250.last.date_time.year[0], data->buff.ht1250.last.date_time.year[1],
                                                     data->buff.ht1250.last.date_time.hour, data->buff.ht1250.last.date_time.min);
    printbuffer(data->buff.ht1250.unknown3, sizeof(data->buff.ht1250.unknown3));
    printf("Radio Configuration\n");
    printf("    Unknown0             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown0); 
    printf("    Unknown1             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown1); 
    printf("    Unknown2             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown2); 
    printf("    RF Test              = %x\n", data->buff.ht1250.radio_configuration.bitfield.rf_test); 
    printf("    FPA Test             = %x\n", data->buff.ht1250.radio_configuration.bitfield.fpa_test); 
    printf("    Unknown5             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown5); 
    printf("    Unknown6             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown6); 
    printf("    TX Inhibit           = %x\n", data->buff.ht1250.radio_configuration.bitfield.tx_override); 
    printf("    Unknown8             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown8); 
    printf("    Unknown9             = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown9); 
    printf("    Unknown10            = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown10); 
    printf("    Unknown11            = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown11); 
    printf("    Sticky Monitor Alert = %x\n", data->buff.ht1250.radio_configuration.bitfield.sticky_permanent_monitor_alert); 
    printf("    Unknown13         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown13); 
    printf("    Unknown14         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown14); 
    printf("    Unknown15         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown15); 
    printf("    Unknown16         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown16); 
    printf("    VOX Headset       = %x\n", data->buff.ht1250.radio_configuration.bitfield.vox_headset); 
    printf("    Cloning           = %x\n", data->buff.ht1250.radio_configuration.bitfield.cloning); 
    printf("    Ht Keypad         = %x\n", data->buff.ht1250.radio_configuration.bitfield.hot_keypad); 
    printf("    Unknown20         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown20);
    printf("    Unknown21         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown21);
    printf("    Power Up Tone     = %x\n", data->buff.ht1250.radio_configuration.bitfield.power_up_tone_type);
    printf("    Unknown24         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown24);
    printf("    Auto Power Mode   = %x\n", data->buff.ht1250.radio_configuration.bitfield.auto_power_mode);
    printf("    Tx Low Batt       = %x\n", data->buff.ht1250.radio_configuration.bitfield.tx_low_batt);
    printf("    Recall Last Menu  = %x\n", data->buff.ht1250.radio_configuration.bitfield.recall_last_menu);
    printf("    Auto Backlight    = %x\n", data->buff.ht1250.radio_configuration.bitfield.auto_backlight);
    printf("    Power Up LED      = %x\n", data->buff.ht1250.radio_configuration.bitfield.power_up_led);
    printf("    Low Batt LED      = %x\n", data->buff.ht1250.radio_configuration.bitfield.low_batt_led);
    printf("    Busy LED          = %x\n", data->buff.ht1250.radio_configuration.bitfield.busy_led);
    printf("    Low Batt Interval = %x\n", data->buff.ht1250.radio_configuration.bitfield.rx_low_batt_interval);
    printf("    Unknown39         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown39);
    printf("    Unknown40         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown40);
    printf("    Unknown41         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown41);
    printf("    Unknown42         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown42);
    printf("    Unknown43         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown43);
    printf("    Unknown44         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown44);
    printf("    Unknown45         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown45);
    printf("    Unknown46         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown46);
    printf("    Unknown47         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown47);
    printf("    Unknown48         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown48);
    printf("    Unknown49         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown49);
    printf("    Unknown50         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown50);
    printf("    Unknown51         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown51);
    printf("    Unknown52         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown52);
    printf("    Unknown53         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown53);
    printf("    Unknown54         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown54);
    printf("    Long Press Time   = %x\n", data->buff.ht1250.radio_configuration.bitfield.long_press_time);
    printf("    Unknown63         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown63);
    printf("    Unknown64         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown64);
    printf("    Unknown65         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown65);
    printf("    Unknown66         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown66);
    printf("    Unknown67         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown67);
    printf("    Unknown68         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown68);
    printf("    Unknown69         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown69);
    printf("    Unknown70         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown70);
    printf("    Unknown71         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown71);
    printf("    Unknown72         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown72);
    printf("    Unknown73         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown73);
    printf("    Unknown74         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown74);
    printf("    Unknown75         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown75);
    printf("    Unknown76         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown76);
    printf("    Unknown77         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown77);
    printf("    Unknown78         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown78);
    printf("    Unknown79         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown79);
    printf("    Option Board Type = %x\n", data->buff.ht1250.radio_configuration.bitfield.option_board_type);
    printf("    Unknown83         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown83);
    printf("    Unknown84         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown84);
    printf("    Unknown85         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown85);
    printf("    Unknown86         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown86);
    printf("    Unknown87         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown87);
    printf("    Unknown88         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown88);
    printf("    Unknown89         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown89);
    printf("    Unknown90         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown90);
    printf("    Unknown91         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown91);
    printf("    Unknown92         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown92);
    printf("    APF               = %x\n", data->buff.ht1250.radio_configuration.bitfield.audio_processing_filter);
    printf("    Alert Tone Constant Boost = %x\n", data->buff.ht1250.radio_configuration.bitfield.alert_tone_constant_boost);
    printf("    Wrap Around Alert = %x\n", data->buff.ht1250.radio_configuration.bitfield.wrap_around_alert);
    printf("    Unknown96         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown96);
    printf("    Priority Scan     = %x\n", data->buff.ht1250.radio_configuration.bitfield.priority_scan);
    printf("    Unknown98         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown98);
    printf("    Unknown99         = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown99);
    printf("    Unknown100        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown100);
    printf("    Unknown101        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown101);
    printf("    Unknown102        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown102);
    printf("    Unknown103        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown103);
    printf("    Unknown104        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown104);
    printf("    Unknown105        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown105);
    printf("    Unknown106        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown106);
    printf("    Unknown107        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown107);
    printf("    Unknown108        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown108);
    printf("    Unknown109        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown109);
    printf("    Unknown110        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown110);
    printf("    Unknown111        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown111);
    printf("    Unknown112        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown112);
    printf("    Unknown113        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown113);
    printf("    Unknown114        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown114);
    printf("    Unknown115        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown115);
    printf("    Unknown116        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown116);
    printf("    Unknown117        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown117);
    printf("    Unknown118        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown118);
    printf("    Unknown119        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown119);
    printf("    Unknown120        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown120);
    printf("    Unknown121        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown121);
    printf("    Unknown122        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown122);
    printf("    Unknown123        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown123);
    printf("    Unknown124        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown124);
    printf("    Unknown125        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown125);
    printf("    Unknown126        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown126);
    printf("    Unknown127        = %x\n", data->buff.ht1250.radio_configuration.bitfield.unknown127);
    printbuffer(data->buff.ht1250.unknown4, sizeof(data->buff.ht1250.unknown4));
    printf("    Radio Time Out    = %x\n", data->buff.ht1250.radio_time_out);
    printbuffer(data->buff.ht1250.unknown5, sizeof(data->buff.ht1250.unknown5));
    printf("    Mic Gain          = %x\n", data->buff.ht1250.mic_gain);
    printf("    Acc Mic Gain      = %x\n", data->buff.ht1250.accessory_mic_gain);
    printbuffer(data->buff.ht1250.unknown6, sizeof(data->buff.ht1250.unknown6));
    printf("Personality Assignments\n");
    printf("    Number of Personality Assignments %d\n", data->buff.ht1250.personality_assignments.assignment_count);
    printf("    Unknown %02x %02x\n", data->buff.ht1250.personality_assignments.unknown[0], 
                                      data->buff.ht1250.personality_assignments.unknown[1]); 
    for(i = 0; i < data->buff.ht1250.personality_assignments.assignment_count; i++)
    {
        printf("    Personality[%u] = %x\n", i, data->buff.ht1250.personality_assignments.entries[i].personality);
        printf("    Unknown1[%u] = %x\n", i, data->buff.ht1250.personality_assignments.entries[i].unknown[0]);
        printf("    Unknown2[%u] = %x\n", i, data->buff.ht1250.personality_assignments.entries[i].unknown[1]);
    }
    tmp_ptr = (unsigned char*)&(data->buff.ht1250.personality_assignments.entries[i]);
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
    printbuffer(tmp_ptr, sizeof(data->buff.ht1250.unknown8)-(tmp_ptr - data->buff.ht1250.unknown8));

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

#ifndef _MOTOROLA_H_
#define _MOTOROLA_H_

#pragma pack(1)
/*This is an odd date strucutre, it's actually stored so that if you read it in hex
  The value printed would be the date in decimal...*/
typedef struct
{
    unsigned char  year[2];
    unsigned char  month;
    unsigned char  day;
    unsigned char  hour;
    unsigned char  min;
} motorola_date_time;

typedef struct
{
    unsigned char      code_plug_major;
    unsigned char      code_plug_minor;
    unsigned char      source;
    motorola_date_time date_time;
} motorola_program_info;

typedef struct
{
    unsigned char      unknown[2];
    unsigned char      personality;
} motorola_personality_assignment_entry;

typedef struct
{
    unsigned char                         assignment_count;
    unsigned char                         unknown[2];
    motorola_personality_assignment_entry entries[1];
} motorla_personality_assignment;

typedef struct
{
    unsigned char string_size;
    unsigned char string_count;
    unsigned char unknown[3];
    char          string[1];
} motorola_string_struct;

typedef struct 
{
    unsigned char unknown[5];
    unsigned char bandwidth:4;
    unsigned char unknown2:4; 
    unsigned char tx_squelch_type;
    unsigned char tx_squelch_code;
    unsigned char rx_squelch_type;
    unsigned char rx_squelch_code;
    union
    {
        struct
        {
            /*
             * Busy Channel Lockout
             * 0 = Disabled 
             * 1 = On Busy Channel
             * 2 = On Busy Channel with Wrong PL Code
             */
            unsigned int busy_channel_lockout:2;  //Busy Channel Lockout -
            unsigned int rx_dpl_invert:1;
            unsigned int tx_dpl_invert:1;
            unsigned int unknown3:1;
            unsigned int unknown4:1;
            unsigned int unknown5:1;
            unsigned int unknown6:1;
            /*
             * Unmute / Mute Rule
             * 0 = Std Unmuting, Std Muting
             * 2 = And Unmuting, Std Muting
             * 3 = And Unmuting, Or Muting
             */
            unsigned int unmute_mute_rule:2;
            /*
             * Squelch Setting
             * 0 = Normal
             * 1 = Tight
             */
            unsigned int squelch_setting:1;
            unsigned int tx_turn_off_code:1;
            unsigned int rx_only:1;
            unsigned int talkaround:1;
            unsigned int auto_scan:1;
            unsigned int unknown11:1;
            /* 
             * Timeout
             * 0 = Infinite
             * 1 = 15 sec
             * 2 = 30 sec
             * 3 = 45 sec
             * 4 = 1 min
             * 5 = 90 sec
             * 6 = 2 min
             * 7 = 3 min
             */
            unsigned int timeout:3; //Timeout
            unsigned int unknown15:1;
            unsigned int unknown16:1;
            unsigned int unknown17:1;
            unsigned int tx_voice_operated:1;
            unsigned int non_standard_reverse_burst:1;
            unsigned int tx_high_power:1;
            unsigned int tx_auto_power:1; 
            unsigned int unknown21:1;
            unsigned int unknown22:1; //Compression/Expansion
            unsigned int unknown23:1; //Compression/Expansion
            unsigned int unknown24:1;
            /*
             * Compression Type
             * 0 - Disabled
             * 1 - Full Compression
             * 2 - AGC Mode
             */
            unsigned int compression_type:2; //Compression/Expansion
        } bitfield;
        unsigned int dword;
    } additional_squelch;
    unsigned char unknown3[10];
    unsigned char checksum;
} motorola_frequency;

typedef struct
{
    union
    {
        struct
        {
            unsigned int pl_scan_lockout:1;
            unsigned int unknown1:1;
            /*
             * PL Scan Type
             * 0 = Disabled
             * 1 = Non-Priority Channel
             * 2 = Priority Channel
             * 3 = Priority and Non-Priority Channel
             */
            unsigned int pl_scan_type:2;
            /*
             * Signaling Type
             * 0 = Disabled
             * 1 = Non-Priority Channel
             * 2 = Priority Channel
             * 3 = Priority and Non-Priority Channel
             */
            unsigned int signaling_type:2;
            unsigned int user_programmable:1;
            unsigned int nuisance_delete:1;
            /*
             * Sample Time
             * Value * 250
             */
            unsigned int sample_time:5;
            unsigned int priority2:1;
            unsigned int priority1:1;
            unsigned int unknown15:1;
            /*
             * Signaling Hold Time (ms)
             * Value * 25
             */
            unsigned int signaling_hold_time:8;
            /*
             * Tx Channel Personality
             * 0 = Selected
             * 7 = Last Active
             * 2 = LS Trunking
             * 6 = Conventional
             */
            unsigned int personality:3;
            unsigned int talkback:1;
            /*
             * Tx Channel Trunk Group
             * Value + 1
             */
            unsigned int trunk_group:3;
            unsigned int unknown31:1;
        } bitfield;
        unsigned int dword;
    } options;
    unsigned char tx_conventional_personality;
    /*
     * LS Scan Activity Search Time (ms)
     * Value * 25
     */
    unsigned char ls_scan_search_time;
    unsigned char unknown;
    unsigned char checksum;
} motorola_scan_list;

typedef struct
{
    /*
     * Member Personality Type
     *  0x00 - Unassigned
     *  0x01 - Conventional
     *  0x02 - LS Trunking
     *  0x07 - Selected
     */
    unsigned char personality_type;
    unsigned char personality_id;
} motorola_scan_list_member_entry;

typedef struct
{
    unsigned char                   unknown[6];
    unsigned char                   entry_count;
    motorola_scan_list_member_entry entires[1]; 
    unsigned char                   checksum;
} motorola_scan_list_members;

typedef struct
{
    unsigned char                  unknown[5];
    char                           serial_num[10];
    char                           null_padding[2];
    char                           model_num[16];
    char                           null_padding2[2];
    motorola_program_info          original;
    unsigned char                  unknown2[0x5F];
    motorola_program_info          last;
    unsigned char                  checksum; //?
    unsigned char                  unknown3[0x38];
    motorla_personality_assignment personality_assignments;
    unsigned char                  unknown4[5769];
} motorola_ht1250;
#pragma pack()

#endif
/* vim: set tabstop=4 shiftwidth=4 expandtab: */

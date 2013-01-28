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
            unsigned int busy_channel_lockout:2;
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
            unsigned int timeout:3;
            unsigned int unknown15:1;
            unsigned int unknown16:1;
            unsigned int unknown17:1;
            unsigned int tx_voice_operated:1;
            unsigned int non_standard_reverse_burst:1;
            unsigned int tx_high_power:1;
            unsigned int tx_auto_power:1; 
            unsigned int unknown21:1;
            unsigned int unknown22:1; /*Compression/Expansion*/
            unsigned int unknown23:1; /*Compression/Expansion*/
            unsigned int unknown24:1;
            /*
             * Compression Type
             * 0 - Disabled
             * 1 - Full Compression
             * 2 - AGC Mode
             */
            unsigned int compression_type:2;
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
    unsigned char                   unknown[2];
    unsigned char                   phone_settings_count;
    struct
    { 
        struct
        {
            unsigned char               unknown1:1;
            unsigned char               unknown2:1;
            unsigned char               unknown3:1;
            unsigned char               unknown4:1;
            unsigned char               ptt_short_sidetone:1;
            unsigned char               ptt_sidetone:1;
            unsigned char               unknown7:1;
            unsigned char               unknown8:1;
        }                               ptt;
        unsigned char                   unknown2[1];
        /* (Value * 25 ms) */
        unsigned char                   pretime;
        unsigned char                   unknown3[3];
        unsigned char                   encoded_access_code[4];
        unsigned char                   encoded_deaccess_code[4];
        /* (Value + 1) * 25ms */
        unsigned char                   tx_tone_duration;
        /* (Value + 1) * 25ms */
        unsigned char                   tx_tone_interval;
        struct
        {
            unsigned short              live_dial_type:1;
            unsigned short              pl_required:1;
            unsigned short              override_busy_channel_lockout:1;
            unsigned short              mute_access_deaccess:1;
            unsigned short              strip_pl:1;
            /* Multiply value * 0.5s */
            unsigned short              back_porch_delay:3;
            unsigned short              unknown9:1;
            unsigned short              unknown10:1;
            unsigned short              unknown11:1;
            unsigned short              unknown12:1;
            unsigned short              continuous_tone_span:1;
            /*
             * 0x00 Immediate Audio
             * 0x01 Delayed Audio
             * 0x02 Manual
             */
            unsigned short              access_deaccess_type:2;
            unsigned short              unknown16:1;
        }                               connection;
        /* (Value * 25 ms) */
        unsigned char                   pause_duration;
        /* (Value * 25 ms) */
        unsigned char                   tx_hang_time;
        /* 0 = No Revert, 2 = LS Trunking-1 */
        unsigned char                   personality;
        unsigned char                   unknown4[3];
    }                               settings[1];
    unsigned char                   checksum;
} motorola_phone_system;

typedef struct
{
    unsigned char                  unknown[5];
    char                           serial_num[10];
    char                           null_padding[2];
    char                           model_num[16];
    char                           null_padding2[2];
    motorola_program_info          original;
    unsigned char                  unknown1[0x19];
    unsigned char                  tanapa[10];
    char                           null_padding3[2];
    unsigned char                  unknown2[0x3A];
    motorola_program_info          last;
    unsigned char                  checksum;
    unsigned char                  unknown3[3];
    union
    {
        struct
        {
            unsigned int           unknown0:1;
            unsigned int           unknown1:1;
            unsigned int           unknown2:1;
            unsigned int           rf_test:1;
            unsigned int           fpa_test:1;
            unsigned int           unknown5:1;
            unsigned int           unknown6:1;
            unsigned int           tx_override:1;
            unsigned int           unknown8:1;
            unsigned int           unknown9:1;
            unsigned int           unknown10:1;
            unsigned int           unknown11:1;
            unsigned int           sticky_permanent_monitor_alert:1;
            unsigned int           unknown13:1;
            /*
             * Monitor Type
             * 1 = Silent
             * 0 = Open Squelch
             */
            unsigned int           unknown14:1;
            unsigned int           unknown15:1;
            unsigned int           unknown16:1;
            unsigned int           vox_headset:1;
            unsigned int           cloning:1;
            unsigned int           hot_keypad:1;
            unsigned int           unknown20:1;
            unsigned int           unknown21:1;
            /*
             * Power Up Tone Type
             * 0 = Disabled
             * 1 = Normal
             * 2 = Musical (Sounds more like an error tone)
             */
            unsigned int           power_up_tone_type:2; 
            unsigned int           unknown24:1;
            unsigned int           auto_power_mode:1;
            unsigned int           tx_low_batt:1;
            unsigned int           recall_last_menu:1;
            unsigned int           auto_backlight:1;
            unsigned int           power_up_led:1;
            unsigned int           low_batt_led:1;
            unsigned int           busy_led:1;
            /*
             * Rx Low Battery Alert Interval
             * 0 == Disabled
             * Value * 5
             */
            unsigned int           rx_low_batt_interval:7;
            unsigned int           unknown39:1;
            unsigned int           unknown40:1;
            unsigned int           unknown41:1;
            unsigned int           unknown42:1;
            unsigned int           unknown43:1;
            unsigned int           unknown44:1;
            unsigned int           unknown45:1;
            unsigned int           unknown46:1;
            unsigned int           unknown47:1;
            unsigned int           unknown48:1;
            unsigned int           unknown49:1;
            unsigned int           unknown50:1;
            unsigned int           unknown51:1;
            unsigned int           unknown52:1;
            unsigned int           unknown53:1;
            unsigned int           unknown54:1;
            unsigned int           unknown55:1;
            /*
             * Long Press Time (ms)
             * Value * 500
             */
            unsigned int           long_press_time:6;
            unsigned int           unknown62:1;
            unsigned int           unknown63:1;
            unsigned int           unknown64:1;
            unsigned int           unknown65:1;
            unsigned int           unknown66:1;
            unsigned int           unknown67:1;
            unsigned int           unknown68:1;
            unsigned int           unknown69:1;
            unsigned int           unknown70:1;
            unsigned int           unknown71:1;
            unsigned int           unknown72:1;
            unsigned int           unknown73:1;
            unsigned int           unknown74:1;
            unsigned int           unknown75:1;
            unsigned int           unknown76:1;
            unsigned int           unknown77:1;
            unsigned int           unknown78:1;
            unsigned int           unknown79:1;
            /*
             * Option Board Type
             * 0 = None
             * 3 = Simple Decoder
             * 4 = Simple Option Interface
             * 5 = Advanced Option Interface
             * 6 = Voice Storage
             * 7 = Advanced & Voice Storage
             */
            unsigned int           option_board_type:3; //Option Board Type
            unsigned int           unknown83:1;
            unsigned int           unknown84:1;
            unsigned int           unknown85:1;
            unsigned int           unknown86:1;
            unsigned int           unknown87:1;
            unsigned int           unknown88:1;
            unsigned int           unknown89:1;
            unsigned int           unknown90:1;
            unsigned int           unknown91:1;
            unsigned int           unknown92:1;
            unsigned int           audio_processing_filter:1;
            unsigned int           alert_tone_constant_boost:1;
            unsigned int           wrap_around_alert:1;
            unsigned int           unknown96:1;
            unsigned int           priority_scan:1;
            unsigned int           unknown98:1;
            unsigned int           unknown99:1;
            unsigned int           unknown100:1;
            unsigned int           unknown101:1;
            unsigned int           unknown102:1;
            unsigned int           unknown103:1;
            unsigned int           unknown104:1;
            unsigned int           unknown105:1;
            unsigned int           unknown106:1;
            unsigned int           unknown107:1;
            unsigned int           unknown108:1;
            unsigned int           unknown109:1;
            unsigned int           unknown110:1;
            unsigned int           unknown111:1;
            unsigned int           unknown112:1;
            unsigned int           unknown113:1;
            unsigned int           unknown114:1;
            unsigned int           unknown115:1;
            unsigned int           unknown116:1;
            unsigned int           unknown117:1;
            unsigned int           unknown118:1;
            unsigned int           unknown119:1;
            unsigned int           unknown120:1;
            unsigned int           unknown121:1;
            unsigned int           unknown122:1;
            unsigned int           unknown123:1;
            unsigned int           unknown124:1;
            unsigned int           unknown125:1;
            unsigned int           unknown126:1;
            unsigned int           unknown127:1;
        }                          bitfield;
        unsigned long long         qword;
        unsigned long long         qword2;
    }                              radio_configuration;
    unsigned char                  unknown4[3];
    unsigned char                  radio_time_out;
    unsigned char                  unknown5[8];
    /*
     * Mic Gain
     * Value * 1.5 dB
     */
    unsigned char                  mic_gain;
    /*
     * Accessory Mic Gain
     * Value * 1.5 dB
     */
    unsigned char                  accessory_mic_gain;
    unsigned char                  unknown6[0x14];
    unsigned char                  checksum2;
    unsigned char                  unknown7[2];
    motorla_personality_assignment personality_assignments;
    unsigned char                  unknown8[5769]; //unknown8[5] == Top Botton Long Press
} motorola_ht1250;
#pragma pack()

typedef enum
{
    MotorolaBoolValue_VOXHeadset,
    MotorolaBoolValue_AutoPowerMode,
    MotorolaBoolValue_RadioRadioCloning,
    MotorolaBoolValue_TxInhibitQuickKeyOverride,
    MotorolaBoolValue_APF,
    MotorolaBoolValue_AutoBacklight,
    MotorolaBoolValue_BusyLED,
    MotorolaBoolValue_TxLowBattPower,
    MotorolaBoolValue_PowerUpTestLED
} MotorolaBoolValueID;

typedef enum
{
    MotorolaTimeValue_LongPressDuration
} MotorolaTimeValueID;

DllExport int motorola_get_data      (void* com_port, void** radio_data_handle);
DllExport int motorola_get_raw       (void* radio_data_handle, void** raw_data, size_t* size);
DllExport int motorola_get_fw_ver    (void* radio_data_handle, char** fw_ver);
DllExport int motorola_get_model_num (void* radio_data_handle, char** model_num);
DllExport int motorola_get_serial_num(void* radio_data_handle, char** serial_num);
DllExport int motorola_get_cp_info   (void* radio_data_handle, int index, motorola_program_info* info);
DllExport int motorola_get_freq_range(void* radio_data_handle, unsigned int* min, unsigned int* max);
DllExport int motorola_get_bool_val  (void* radio_data_handle, MotorolaBoolValueID id);
DllExport int motorola_set_bool_val  (void* radio_data_handle, MotorolaBoolValueID id, unsigned int val);
DllExport int motorola_get_time_val  (void* radio_data_handle, MotorolaTimeValueID id);
DllExport int motorola_set_time_val  (void* radio_data_handle, MotorolaTimeValueID id, unsigned int val);

#endif
/* vim: set tabstop=4 shiftwidth=4 expandtab: */

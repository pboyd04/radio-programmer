#ifndef _MOTOROLA_SERIAL_H_
#define _MOTOROLA_SERIAL_H_

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

void         motorola_calculate_checksum(unsigned char* cmd, size_t len);
unsigned int motorola_valid_checksum(unsigned char* res, size_t len);

ssize_t      motorola_send_cmd(void* com_port, unsigned char* cmd, size_t cmdlen, /*@out@*/ unsigned char* res, size_t reslen);
ssize_t      motorola_read_data(void* com_port, size_t offset, unsigned char* res, size_t reslen);
ssize_t      motorola_read_data2(void* com_port, size_t start_offset, unsigned int step, size_t length, unsigned char* res, size_t reslen);

int          motorola_read_fw_ver(void* com_port, /*@out@*/ char** fw_ver);
int          motorola_compatible_check(void* com_port);
int          motorola_get_frequency_range(void* com_port, unsigned int* min_freq, unsigned int* max_freq);

#endif

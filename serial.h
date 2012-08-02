#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifdef _MSC_VER
#define OUTPARAM _Out_
#define INPARAM _In_
#else
#define OUTPARAM
#define INPARAM
#endif

int isSerialPortPresent(const char* name);
int getSerialPorts(OUTPARAM unsigned int* count, OUTPARAM char*** portnames);
int openport(const char* portname, /*@out@*/ void** com_port);
size_t write_serial(void* com_port, unsigned char* data, size_t datalen);
size_t read_serial(void* com_port, /*@out@*/ unsigned char* data, size_t datalen);
size_t write_verify_read(void* com_port, unsigned char* in, size_t inlen, /*@out@*/ unsigned char* out, size_t outlen);
int closeport(void* com_port);

#endif

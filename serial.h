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
int openport(const char* portname, void** com_port);
unsigned int write_serial(void* com_port, unsigned char* data, unsigned int datalen);
unsigned int read_serial(void* com_port, unsigned char* data, unsigned int datalen);
unsigned int write_verify_read(void* com_port, unsigned char* in, unsigned int inlen, unsigned char* out, unsigned int outlen);
int closeport(void* com_port);

#endif

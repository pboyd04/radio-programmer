#ifndef _SERIAL_H_
#define _SERIAL_H_
#include "dll.h"

#ifdef _MSC_VER
#define OUTPARAM _Out_
#define INPARAM _In_
#else
#define OUTPARAM
#define INPARAM
#endif

DllExport int isSerialPortPresent(const char* name);
DllExport int getSerialPorts(OUTPARAM unsigned int* count, OUTPARAM char*** portnames);
DllExport int openport(const char* portname, /*@out@*/ void** com_port);
DllExport size_t write_serial(void* com_port, unsigned char* data, size_t datalen);
DllExport size_t read_serial(void* com_port, /*@out@*/ unsigned char* data, size_t datalen);
DllExport size_t write_verify_read(void* com_port, unsigned char* in, size_t inlen, /*@out@*/ unsigned char* out, size_t outlen);
DllExport int closeport(void* com_port);

#endif

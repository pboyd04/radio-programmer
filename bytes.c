#include "bytes.h"

unsigned short be16toh_array(unsigned char* array)
{
    unsigned short ret = (unsigned short)((array[0] << 8) | (array[1]));
    return ret;
}

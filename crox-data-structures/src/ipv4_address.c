
#include "ipv4_address.h"

#include <stdio.h>

char *ipv4_address_to_string(const ipv4_address_t address, char *buffer, u32 size)
{
    if (buffer == NULL || size == 0)
    {
        return NULL;
    }
    snprintf(buffer, size, "%u.%u.%u.%u", address.o3, address.o2, address.o1, address.o0);
    return buffer;
}

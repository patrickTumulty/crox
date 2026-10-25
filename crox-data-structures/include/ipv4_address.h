
#ifndef CROX_IPV4_ADDRESS_H
#define CROX_IPV4_ADDRESS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "crox_types.h"
#include <stddef.h>

/**
 * @brief IPv4 address, e.g. 127.0.0.1
 *
 * The address can be accessed octet by octet (declaration order: o3 is the
 * left-most octet, "127" in "127.0.0.1") or as one 32 bit word.
 */
typedef union {
    struct
    {
        u8 o3;
        u8 o2;
        u8 o1;
        u8 o0;
    };
    u32 ip;
} ipv4_address_t;

/** Simple helper macro for initializing localhost ipv4 address */
#define ipv4_localhost ((ipv4_address_t){127, 0, 0, 1})

/** Simple helper macro for initializing ipv4 address */
#define ipv4(O3, O2, O1, O0) ((ipv4_address_t){(O3), (O2), (O1), O0})

/**
 * @brief Format the address as dot-separated decimals, "a.b.c.d".
 *
 * @param[in]  address Address to format; may be NULL (yields "0.0.0.0").
 * @param[out] buffer  Output buffer; contents are unspecified on failure.
 * @param[in]  size    Capacity of @p buffer, in bytes; 16 bytes are always
 *                     sufficient ("255.255.255.255" plus a terminator).
 *
 * @return @p buffer, or NULL when @p buffer is NULL or @p size is zero.
 */
char *ipv4_address_to_string(const ipv4_address_t address, char *buffer, u32 size);

#ifdef __cplusplus
}
#endif

#endif // CROX_IPV4_ADDRESS_H

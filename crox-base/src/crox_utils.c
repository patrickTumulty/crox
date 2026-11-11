
#define _POSIX_C_SOURCE 199309L
#include "crox_utils.h"
#include "crox_types.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <time.h>

u64 crox_time_get_monotonic_ns(void)
{
    static u64 start_ns = 0;
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0;
    }

    u64 now_ns = ((u64)ts.tv_sec * 1000000000ULL) + (u64)ts.tv_nsec;

    if (start_ns == 0)
    {
        start_ns = now_ns;
    }

    return now_ns - start_ns;
}

u64 crox_time_get_epoch_timestamp_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        return UINT64_MAX;
    }

    return ((u64)ts.tv_sec * 1000ULL) + ((u64)ts.tv_nsec / 1000000ULL);
}

f32 crox_time_get_monotonic_seconds(void)
{
    return (f32)((double)crox_time_get_monotonic_ns() / 1e9);
}

u64 crox_time_get_monotonic_ms(void)
{
    return crox_time_get_monotonic_ns() / 1000000ULL;
}

void crox_str_replace_ip(char *str, const size_t str_capacity, const char *find, const char *replace)
{
    size_t str_len = strlen(str);
    const size_t find_len = strlen(find);
    const size_t replace_len = strlen(replace);

    if (find_len == 0)
    {
        return;
    }

    for (size_t i = 0; str[i] != '\0';)
    {
        if (strncmp(str + i, find, find_len) != 0)
        {
            i++;
            continue;
        }

        size_t new_len = replace_len > find_len ? //
                             str_len + (replace_len - find_len)
                                                : str_len - (find_len - replace_len);

        if (new_len >= str_capacity)
            return;

        char *cont = str + i + find_len;

        memmove(str + i + replace_len, cont, str_len - (i + find_len) + 1);
        memcpy(str + i, replace, replace_len);

        /* Keep the tracked length current: a stale length skews the tail move
         * count of the next replacement and can underflow it. */
        str_len = new_len;
        i += replace_len;
    }
}

void crox_str_replace_cp(const char *str,               //
                         char *out_str,                 //
                         const size_t out_str_capacity, //
                         const char *find,              //
                         const char *replace)
{
    snprintf(out_str, out_str_capacity, "%s", str);
    crox_str_replace_ip(out_str, out_str_capacity, find, replace);
}

void crox_str_join(char *str,
                   const size_t str_capacity,
                   char *tokens[],
                   const size_t tokens_count,
                   const char *delimiter)
{
    str[0] = '\0';
    size_t used = 0;
    for (size_t i = 0; i < tokens_count; i++)
    {
        /* Distinguish the first output by position: an empty token writes
         * nothing, so a used-byte count cannot tell the two apart. */
        const int written = snprintf(str + used, str_capacity - used, "%s%s", i == 0 ? "" : delimiter, tokens[i]);
        if (written < 0 || (size_t)written >= str_capacity - used)
        {
            break; // No room for the next token; keep what fit.
        }
        used += (size_t)written;
    }
}

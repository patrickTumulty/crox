

#include "crox_utils.h"
#include "crox_types.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

u64 get_timestamp_ms(void)
{
    struct timespec ts;
    // TIME_UTC returns the system's real-time clock (Epoch time)
    if (timespec_get(&ts, TIME_UTC) == 0)
    {
        return -1; // Error
    }
    // Convert seconds to ms and nanoseconds to ms
    return ((long long)ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

void str_replace_ip(char *str, const size_t str_capacity, const char *find, const char *replace)
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
                             str_len + (replace_len - find_len) :
                             str_len - (find_len - replace_len);

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

void str_replace_cp(const char *str,               //
                    char *out_str,                 //
                    const size_t out_str_capacity, //
                    const char *find,              //
                    const char *replace)
{
    snprintf(out_str, out_str_capacity, "%s", str);
    str_replace_ip(out_str, out_str_capacity, find, replace);
}

void str_join(char *str, const size_t str_capacity, char *tokens[], const size_t tokens_count, const char *delimiter)
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

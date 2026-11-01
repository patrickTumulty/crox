
#ifndef CROX_UTILS_H
#define CROX_UTILS_H

#include "crox_types.h"
#include <stddef.h>

/**
 * @brief get current system epoch time in millisesconds
 *
 * @return current epoch time in milliseconds
 */
u64 get_timestamp_ms(void);

/**
 * @brief find and replace (in place)
 *
 * @param[[in,out]] str input string
 * @param[[in]] str_capacity input string max buffer size
 * @param[[in]] find pattern to find
 * @param[[in]] replace pattern to replace
 */
void str_replace_ip(char *str, const size_t str_capacity, const char *find, const char *replace);

/**
 * @brief find and replace (copy)
 *
 * @param[[in]] str input string
 * @param[[out]] out_str output string buffer
 * @param[[in]] out_str_capacity output string buffer size
 * @param[[in]] find pattern to find
 * @param[[in]] replace pattern to replace
 */
void str_replace_cp(const char *str,               //
                    char *out_str,                 //
                    const size_t out_str_capacity, //
                    const char *find,              //
                    const char *replace);

/**
 * @brief join N number of tokens into a string buffer, separated by a delimiter
 *
 * @param[[out]] str output string buffer
 * @param[[in]] str_capacity string vuffer capacity
 * @param[[in]] tokens tokens
 * @param[[in]] tokens_count tokens count
 * @param[[in]] delimiter delimiter
 */
void str_join(char *str, const size_t str_capacity, char *tokens[], const size_t tokens_count, const char *delimiter);

#endif // CROX_UTILS_H

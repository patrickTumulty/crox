

#include "crox_utils.h"
#include "crox_types.h"
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

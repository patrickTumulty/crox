#define _POSIX_C_SOURCE 200809L

#include "unix_socket_shared.h"

#include "crox_types.h"
#include "logging_facade.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

bool unix_socket_make_address(const char *path, struct sockaddr_un *addr)
{
    if (path == NULL || path[0] == '\0')
    {
        lfLogError("Unix socket path must not be empty");
        return false;
    }

    // sun_path is a fixed ~108 byte buffer; a silently truncated path would refer to a different socket.
    size_t path_len = strlen(path);
    if (path_len >= UNIX_SOCKET_PATH_CAPACITY)
    {
        lfLogError("Unix socket path too long (%zu >= %zu): %s", path_len, UNIX_SOCKET_PATH_CAPACITY, path);
        return false;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    memcpy(addr->sun_path, path, path_len + 1);
    return true;
}

ssize_t unix_socket_recv(int fd, void *buf, size_t len)
{
    ssize_t n;
    do
    {
        n = recv(fd, buf, len, 0);
    } while (n == -1 && errno == EINTR);
    return n;
}

bool unix_socket_send_all(int fd, const void *data, size_t len)
{
    const u8 *cursor = (const u8 *)data;
    size_t remaining = len;
    while (remaining > 0)
    {
        // MSG_NOSIGNAL: a peer that hung up should surface as EPIPE, not kill the process with SIGPIPE.
        ssize_t n = send(fd, cursor, remaining, MSG_NOSIGNAL);
        if (n == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        cursor += n;
        remaining -= (size_t)n;
    }
    return true;
}

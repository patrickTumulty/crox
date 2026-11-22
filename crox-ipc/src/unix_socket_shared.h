
#ifndef CROX_UNIX_SOCKET_SHARED_H
#define CROX_UNIX_SOCKET_SHARED_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/un.h>

#define UNIX_SOCKET_NO_FD (-1)
#define UNIX_SOCKET_PATH_CAPACITY sizeof(((struct sockaddr_un *)0)->sun_path)

/**
 * @brief Validate @p path and build the matching socket address.
 *
 * @return false (after logging) when @p path is empty or too long for sun_path.
 */
bool unix_socket_make_address(const char *path, struct sockaddr_un *addr);

/** @brief recv() that retries on EINTR. Returns what recv() returns. */
ssize_t unix_socket_recv(int fd, void *buf, size_t len);

/** @brief Send every byte of @p data, retrying on EINTR. On failure returns false with errno set. */
bool unix_socket_send_all(int fd, const void *data, size_t len);

#endif // CROX_UNIX_SOCKET_SHARED_H

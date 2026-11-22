#define _POSIX_C_SOURCE 200809L

#include "unix_socket_client.h"

#include "logging_facade.h"
#include "unix_socket_shared.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct unix_socket_client_s
{
    int socket_fd;
    struct sockaddr_un addr;
};

static void close_connection(unix_socket_client_t *client)
{
    if (client->socket_fd != UNIX_SOCKET_NO_FD)
    {
        close(client->socket_fd);
        client->socket_fd = UNIX_SOCKET_NO_FD;
    }
}

unix_socket_client_t *unix_socket_client_init(const char *path)
{
    struct sockaddr_un addr;
    if (!unix_socket_make_address(path, &addr))
    {
        return NULL;
    }

    unix_socket_client_t *client = (unix_socket_client_t *)calloc(1, sizeof(unix_socket_client_t));
    if (client == NULL)
    {
        lfLogError("Failed to allocate unix socket client");
        return NULL;
    }
    client->socket_fd = UNIX_SOCKET_NO_FD;
    client->addr = addr;

    return client;
}

rc_t unix_socket_client_connect(unix_socket_client_t *client)
{
    if (client == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }

    close_connection(client);

    // A socket whose connect() failed is left in an unspecified state, so every attempt needs a fresh one.
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == UNIX_SOCKET_NO_FD)
    {
        lfLogError("socket() failed: %s", strerror(errno));
        return RC_ERROR;
    }

    int result;
    do
    {
        result = connect(fd, (const struct sockaddr *)&client->addr, sizeof(client->addr));
    } while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        lfLogError("connect(%s) failed: %s", client->addr.sun_path, strerror(errno));
        close(fd);
        return RC_ERROR;
    }

    client->socket_fd = fd;
    lfLogInfo("Unix socket client connected to %s", client->addr.sun_path);
    return RC_OK;
}

rc_t unix_socket_client_disconnect(unix_socket_client_t *client)
{
    if (client == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }

    close_connection(client);
    return RC_OK;
}

bool unix_socket_client_is_connected(const unix_socket_client_t *client)
{
    return client != NULL && client->socket_fd != UNIX_SOCKET_NO_FD;
}

int unix_socket_client_get_fd(const unix_socket_client_t *client)
{
    return client != NULL ? client->socket_fd : UNIX_SOCKET_NO_FD;
}

rc_t unix_socket_client_read(unix_socket_client_t *client, void *buf, size_t buf_len, size_t *bytes_read)
{
    if (client == NULL || buf == NULL || bytes_read == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }
    *bytes_read = 0;

    if (client->socket_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogWarn("Unix socket read on %s while not connected", client->addr.sun_path);
        return RC_NOT_INITIALIZED;
    }

    ssize_t n = unix_socket_recv(client->socket_fd, buf, buf_len);
    if (n == -1)
    {
        lfLogError("recv(%s) failed: %s", client->addr.sun_path, strerror(errno));
        close_connection(client);
        return RC_ERROR;
    }

    if (n == 0)
    {
        lfLogInfo("Unix socket server %s closed the connection", client->addr.sun_path);
        close_connection(client);
        return RC_OK;
    }

    *bytes_read = (size_t)n;
    return RC_OK;
}

rc_t unix_socket_client_write(unix_socket_client_t *client, const void *data, size_t len)
{
    if (client == NULL || (data == NULL && len > 0))
    {
        return RC_BAD_INPUT_ARG;
    }

    if (client->socket_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogWarn("Unix socket write on %s while not connected", client->addr.sun_path);
        return RC_NOT_INITIALIZED;
    }

    if (!unix_socket_send_all(client->socket_fd, data, len))
    {
        lfLogError("send(%s) failed: %s", client->addr.sun_path, strerror(errno));
        close_connection(client);
        return RC_ERROR;
    }

    return RC_OK;
}

rc_t unix_socket_client_free(unix_socket_client_t *client)
{
    if (client == NULL)
    {
        return RC_OK;
    }

    close_connection(client);
    free(client);
    return RC_OK;
}

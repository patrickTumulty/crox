#define _POSIX_C_SOURCE 200809L

#include "unix_socket_server.h"

#include "logging_facade.h"
#include "unix_socket_shared.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 5

struct unix_socket_server_s
{
    int listen_fd;
    int client_fd;
    bool socket_file_created;
    struct sockaddr_un addr;
};

static void close_client(unix_socket_server_t *server)
{
    if (server->client_fd != UNIX_SOCKET_NO_FD)
    {
        close(server->client_fd);
        server->client_fd = UNIX_SOCKET_NO_FD;
    }
}

unix_socket_server_t *unix_socket_server_init(const char *path)
{
    struct sockaddr_un addr;
    if (!unix_socket_make_address(path, &addr))
    {
        return NULL;
    }

    unix_socket_server_t *server = (unix_socket_server_t *)calloc(1, sizeof(unix_socket_server_t));
    if (server == NULL)
    {
        lfLogError("Failed to allocate unix socket server");
        return NULL;
    }
    server->listen_fd = UNIX_SOCKET_NO_FD;
    server->client_fd = UNIX_SOCKET_NO_FD;
    server->addr = addr;

    server->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->listen_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogError("socket() failed: %s", strerror(errno));
        unix_socket_server_free(server);
        return NULL;
    }

    // A previous run that crashed leaves the socket file behind, which would make bind() fail.
    unlink(server->addr.sun_path);

    if (bind(server->listen_fd, (const struct sockaddr *)&server->addr, sizeof(server->addr)) == -1)
    {
        lfLogError("bind(%s) failed: %s", server->addr.sun_path, strerror(errno));
        unix_socket_server_free(server);
        return NULL;
    }
    server->socket_file_created = true;

    if (listen(server->listen_fd, LISTEN_BACKLOG) == -1)
    {
        lfLogError("listen(%s) failed: %s", server->addr.sun_path, strerror(errno));
        unix_socket_server_free(server);
        return NULL;
    }

    lfLogInfo("Unix socket server listening on %s", server->addr.sun_path);
    return server;
}

rc_t unix_socket_server_accept(unix_socket_server_t *server)
{
    if (server == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }

    close_client(server);

    int client_fd;
    do
    {
        client_fd = accept(server->listen_fd, NULL, NULL);
    } while (client_fd == UNIX_SOCKET_NO_FD && errno == EINTR);

    if (client_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogError("accept(%s) failed: %s", server->addr.sun_path, strerror(errno));
        return RC_ERROR;
    }

    server->client_fd = client_fd;
    lfLogInfo("Unix socket client connected on %s", server->addr.sun_path);
    return RC_OK;
}

rc_t unix_socket_server_disconnect_client(unix_socket_server_t *server)
{
    if (server == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }

    close_client(server);
    return RC_OK;
}

bool unix_socket_server_has_client(const unix_socket_server_t *server)
{
    return server != NULL && server->client_fd != UNIX_SOCKET_NO_FD;
}

int unix_socket_server_get_listen_fd(const unix_socket_server_t *server)
{
    return server != NULL ? server->listen_fd : UNIX_SOCKET_NO_FD;
}

int unix_socket_server_get_client_fd(const unix_socket_server_t *server)
{
    return server != NULL ? server->client_fd : UNIX_SOCKET_NO_FD;
}

rc_t unix_socket_server_read(unix_socket_server_t *server, void *buf, size_t buf_len, size_t *bytes_read)
{
    if (server == NULL || buf == NULL || bytes_read == NULL)
    {
        return RC_BAD_INPUT_ARG;
    }
    *bytes_read = 0;

    if (server->client_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogWarn("Unix socket read on %s with no client connected", server->addr.sun_path);
        return RC_NOT_INITIALIZED;
    }

    ssize_t n = unix_socket_recv(server->client_fd, buf, buf_len);
    if (n == -1)
    {
        lfLogError("recv(%s) failed: %s", server->addr.sun_path, strerror(errno));
        close_client(server);
        return RC_ERROR;
    }

    if (n == 0)
    {
        lfLogInfo("Unix socket client disconnected from %s", server->addr.sun_path);
        close_client(server);
        return RC_OK;
    }

    *bytes_read = (size_t)n;
    return RC_OK;
}

rc_t unix_socket_server_write(unix_socket_server_t *server, const void *data, size_t len)
{
    if (server == NULL || (data == NULL && len > 0))
    {
        return RC_BAD_INPUT_ARG;
    }

    if (server->client_fd == UNIX_SOCKET_NO_FD)
    {
        lfLogWarn("Unix socket write on %s with no client connected", server->addr.sun_path);
        return RC_NOT_INITIALIZED;
    }

    if (!unix_socket_send_all(server->client_fd, data, len))
    {
        lfLogError("send(%s) failed: %s", server->addr.sun_path, strerror(errno));
        close_client(server);
        return RC_ERROR;
    }

    return RC_OK;
}

rc_t unix_socket_server_free(unix_socket_server_t *server)
{
    if (server == NULL)
    {
        return RC_OK;
    }

    close_client(server);

    if (server->listen_fd != UNIX_SOCKET_NO_FD)
    {
        close(server->listen_fd);
        server->listen_fd = UNIX_SOCKET_NO_FD;
    }

    if (server->socket_file_created && unlink(server->addr.sun_path) == -1 && errno != ENOENT)
    {
        lfLogWarn("Failed to remove unix socket file %s: %s", server->addr.sun_path, strerror(errno));
    }

    free(server);
    return RC_OK;
}

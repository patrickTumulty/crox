
#ifndef CROX_UNIX_SOCKET_SERVER_H
#define CROX_UNIX_SOCKET_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "crox_types.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Server end of a Unix domain stream socket that serves one client at a time.
 *
 * Streams have no message boundaries: a read may return part of a message
 * or several messages at once, so callers must frame their own data.
 */
typedef struct unix_socket_server_s unix_socket_server_t;

/**
 * @brief Create a Unix domain stream socket bound to @p path and start listening.
 *
 * Any stale socket file at @p path is removed first.
 *
 * @param[in] path Filesystem path to bind; must fit in sun_path.
 *
 * @return the server, or NULL on failure.
 */
unix_socket_server_t *unix_socket_server_init(const char *path);

/**
 * @brief Close any connected client and the listening socket, remove the
 *        socket file and free the server.
 *
 * @param[in] server Server to free; may be NULL.
 *
 * @retval RC_OK Always.
 */
rc_t unix_socket_server_free(unix_socket_server_t *server);

/**
 * @brief Block until a client connects.
 *
 * Only one client is served at a time, so any previously connected client is
 * disconnected first.
 *
 * @retval RC_OK            A client is connected.
 * @retval RC_BAD_INPUT_ARG @p server is NULL.
 * @retval RC_ERROR         accept() failed.
 */
rc_t unix_socket_server_accept(unix_socket_server_t *server);

/**
 * @brief Close the connection to the current client, if any.
 *
 * @retval RC_OK            Disconnected (or no client was connected).
 * @retval RC_BAD_INPUT_ARG @p server is NULL.
 */
rc_t unix_socket_server_disconnect_client(unix_socket_server_t *server);

/**
 * @brief Whether a client is currently connected.
 *
 * @return false when @p server is NULL.
 */
bool unix_socket_server_has_client(const unix_socket_server_t *server);

/**
 * @brief File descriptor of the listening socket, for use with poll()/select().
 *
 * Readable when a client is waiting to be accepted. The descriptor stays
 * owned by the server; do not close it.
 *
 * @return the descriptor, or -1 when @p server is NULL.
 */
int unix_socket_server_get_listen_fd(const unix_socket_server_t *server);

/**
 * @brief File descriptor of the connected client, for use with poll()/select().
 *
 * The descriptor stays owned by the server; do not close it.
 *
 * @return the descriptor, or -1 when no client is connected or @p server is NULL.
 */
int unix_socket_server_get_client_fd(const unix_socket_server_t *server);

/**
 * @brief Block until data arrives, then read up to @p buf_len bytes from the connected client.
 *
 * A @p bytes_read of 0 with RC_OK means the client hung up; the client is
 * disconnected and unix_socket_server_accept() must be called again before
 * further reads or writes.
 *
 * @param[in]  server     Server with a connected client.
 * @param[out] buf        Destination buffer.
 * @param[in]  buf_len    Capacity of @p buf, in bytes.
 * @param[out] bytes_read Number of bytes read.
 *
 * @retval RC_OK              Data read, or the client hung up.
 * @retval RC_BAD_INPUT_ARG   A required argument is NULL.
 * @retval RC_NOT_INITIALIZED No client is connected.
 * @retval RC_ERROR           The read failed; the client is disconnected.
 */
rc_t unix_socket_server_read(unix_socket_server_t *server, void *buf, size_t buf_len, size_t *bytes_read);

/**
 * @brief Write all @p len bytes to the connected client.
 *
 * @param[in] server Server with a connected client.
 * @param[in] data   Bytes to send; may be NULL only when @p len is 0.
 * @param[in] len    Number of bytes to send.
 *
 * @retval RC_OK              All bytes written.
 * @retval RC_BAD_INPUT_ARG   A required argument is NULL.
 * @retval RC_NOT_INITIALIZED No client is connected.
 * @retval RC_ERROR           The write failed; the client is disconnected.
 */
rc_t unix_socket_server_write(unix_socket_server_t *server, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CROX_UNIX_SOCKET_SERVER_H

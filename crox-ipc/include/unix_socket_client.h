
#ifndef CROX_UNIX_SOCKET_CLIENT_H
#define CROX_UNIX_SOCKET_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "crox_types.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Client end of a Unix domain stream socket.
 *
 * Streams have no message boundaries: a read may return part of a message
 * or several messages at once, so callers must frame their own data.
 */
typedef struct unix_socket_client_s unix_socket_client_t;

/**
 * @brief Create a client for the Unix domain socket at @p path.
 *
 * Does not connect; call unix_socket_client_connect() for that.
 *
 * @param[in] path Filesystem path of the server socket; must fit in sun_path.
 *
 * @return the client, or NULL on failure.
 */
unix_socket_client_t *unix_socket_client_init(const char *path);

/**
 * @brief Disconnect from the server if connected and free the client.
 *
 * @param[in] client Client to free; may be NULL.
 *
 * @retval RC_OK Always.
 */
rc_t unix_socket_client_free(unix_socket_client_t *client);

/**
 * @brief Connect to the server.
 *
 * Any existing connection is closed first, so this can also be used to
 * reconnect after the server hangs up.
 *
 * @retval RC_OK            Connected.
 * @retval RC_BAD_INPUT_ARG @p client is NULL.
 * @retval RC_ERROR         The server is not listening or the connection failed.
 */
rc_t unix_socket_client_connect(unix_socket_client_t *client);

/**
 * @brief Close the connection to the server, if any.
 *
 * @retval RC_OK            Disconnected (or was not connected).
 * @retval RC_BAD_INPUT_ARG @p client is NULL.
 */
rc_t unix_socket_client_disconnect(unix_socket_client_t *client);

/**
 * @brief Whether the client currently holds a connection.
 *
 * @return false when @p client is NULL.
 */
bool unix_socket_client_is_connected(const unix_socket_client_t *client);

/**
 * @brief File descriptor of the connection, for use with poll()/select().
 *
 * The descriptor stays owned by the client; do not close it.
 *
 * @return the descriptor, or -1 when not connected or @p client is NULL.
 */
int unix_socket_client_get_fd(const unix_socket_client_t *client);

/**
 * @brief Block until data arrives, then read up to @p buf_len bytes.
 *
 * A @p bytes_read of 0 with RC_OK means the server hung up; the client is
 * disconnected and unix_socket_client_connect() must be called again before
 * further reads or writes.
 *
 * @param[in]  client     Connected client.
 * @param[out] buf        Destination buffer.
 * @param[in]  buf_len    Capacity of @p buf, in bytes.
 * @param[out] bytes_read Number of bytes read.
 *
 * @retval RC_OK              Data read, or the server hung up.
 * @retval RC_BAD_INPUT_ARG   A required argument is NULL.
 * @retval RC_NOT_INITIALIZED Not connected.
 * @retval RC_ERROR           The read failed; the client is disconnected.
 */
rc_t unix_socket_client_read(unix_socket_client_t *client, void *buf, size_t buf_len, size_t *bytes_read);

/**
 * @brief Write all @p len bytes to the server.
 *
 * @param[in] client Connected client.
 * @param[in] data   Bytes to send; may be NULL only when @p len is 0.
 * @param[in] len    Number of bytes to send.
 *
 * @retval RC_OK              All bytes written.
 * @retval RC_BAD_INPUT_ARG   A required argument is NULL.
 * @retval RC_NOT_INITIALIZED Not connected.
 * @retval RC_ERROR           The write failed; the client is disconnected.
 */
rc_t unix_socket_client_write(unix_socket_client_t *client, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CROX_UNIX_SOCKET_CLIENT_H

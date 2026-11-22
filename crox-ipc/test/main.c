#define _POSIX_C_SOURCE 200809L

#include "crox_types.h"
#include "logging_facade.h"
#include "munit.h"
#include "unix_socket_client.h"
#include "unix_socket_server.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Every test gets its own directory, so tests never share a socket file and a
 * crashed test cannot leave a stale socket behind for the next one. */
typedef struct
{
    char dir[64];
    char path[96];
} socket_fixture_t;

static void *fixture_setup(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    socket_fixture_t *fixture = munit_new(socket_fixture_t);
    snprintf(fixture->dir, sizeof(fixture->dir), "/tmp/crox_ipc_test_XXXXXX");
    munit_assert_not_null(mkdtemp(fixture->dir));
    snprintf(fixture->path, sizeof(fixture->path), "%s/test.sock", fixture->dir);
    return fixture;
}

static void fixture_teardown(void *data)
{
    socket_fixture_t *fixture = (socket_fixture_t *)data;
    unlink(fixture->path);
    rmdir(fixture->dir);
    free(fixture);
}

static bool path_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* connect() completes as soon as the kernel queues the connection in the
 * listen backlog, so client and server can be driven from one thread. */
static void connect_pair(const char *path, unix_socket_server_t **server, unix_socket_client_t **client)
{
    *server = unix_socket_server_init(path);
    munit_assert_not_null(*server);
    *client = unix_socket_client_init(path);
    munit_assert_not_null(*client);
    munit_assert_int(unix_socket_client_connect(*client), ==, RC_OK);
    munit_assert_int(unix_socket_server_accept(*server), ==, RC_OK);
}

/* Read until exactly @p len bytes have arrived, since a stream read may return less. */
static void server_read_exactly(unix_socket_server_t *server, u8 *buf, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        size_t n = 0;
        munit_assert_int(unix_socket_server_read(server, buf + total, len - total, &n), ==, RC_OK);
        munit_assert_size(n, >, 0);
        total += n;
    }
}

static MunitResult test_init_invalid_path(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char too_long[256];
    memset(too_long, 'a', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';

    munit_assert_null(unix_socket_server_init(NULL));
    munit_assert_null(unix_socket_server_init(""));
    munit_assert_null(unix_socket_server_init(too_long));

    munit_assert_null(unix_socket_client_init(NULL));
    munit_assert_null(unix_socket_client_init(""));
    munit_assert_null(unix_socket_client_init(too_long));

    return MUNIT_OK;
}

static MunitResult test_server_socket_file_lifecycle(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server = unix_socket_server_init(fixture->path);
    munit_assert_not_null(server);
    munit_assert_true(path_exists(fixture->path));
    munit_assert_int(unix_socket_server_get_listen_fd(server), >=, 0);
    munit_assert_false(unix_socket_server_has_client(server));
    munit_assert_int(unix_socket_server_get_client_fd(server), ==, -1);

    munit_assert_int(unix_socket_server_free(server), ==, RC_OK);
    munit_assert_false(path_exists(fixture->path));

    return MUNIT_OK;
}

static MunitResult test_server_replaces_stale_file(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    FILE *stale = fopen(fixture->path, "w");
    munit_assert_not_null(stale);
    fclose(stale);

    unix_socket_server_t *server = unix_socket_server_init(fixture->path);
    munit_assert_not_null(server);

    unix_socket_client_t *client = unix_socket_client_init(fixture->path);
    munit_assert_int(unix_socket_client_connect(client), ==, RC_OK);

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_client_connect_without_server(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_client_t *client = unix_socket_client_init(fixture->path);
    munit_assert_not_null(client);
    munit_assert_false(unix_socket_client_is_connected(client));

    munit_assert_int(unix_socket_client_connect(client), ==, RC_ERROR);
    munit_assert_false(unix_socket_client_is_connected(client));
    munit_assert_int(unix_socket_client_get_fd(client), ==, -1);

    unix_socket_client_free(client);
    return MUNIT_OK;
}

static MunitResult test_not_connected(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server = unix_socket_server_init(fixture->path);
    unix_socket_client_t *client = unix_socket_client_init(fixture->path);
    munit_assert_not_null(server);
    munit_assert_not_null(client);

    char buf[8];
    size_t n = 123;
    munit_assert_int(unix_socket_client_read(client, buf, sizeof(buf), &n), ==, RC_NOT_INITIALIZED);
    munit_assert_size(n, ==, 0);
    munit_assert_int(unix_socket_client_write(client, "x", 1), ==, RC_NOT_INITIALIZED);

    n = 123;
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), &n), ==, RC_NOT_INITIALIZED);
    munit_assert_size(n, ==, 0);
    munit_assert_int(unix_socket_server_write(server, "x", 1), ==, RC_NOT_INITIALIZED);

    /* Disconnecting when not connected is a harmless no-op. */
    munit_assert_int(unix_socket_client_disconnect(client), ==, RC_OK);
    munit_assert_int(unix_socket_server_disconnect_client(server), ==, RC_OK);

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_round_trip(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    munit_assert_true(unix_socket_client_is_connected(client));
    munit_assert_int(unix_socket_client_get_fd(client), >=, 0);
    munit_assert_true(unix_socket_server_has_client(server));
    munit_assert_int(unix_socket_server_get_client_fd(server), >=, 0);

    char buf[32];
    size_t n = 0;

    munit_assert_int(unix_socket_client_write(client, "ping", 4), ==, RC_OK);
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 4);
    munit_assert_memory_equal(4, buf, "ping");

    munit_assert_int(unix_socket_server_write(server, "pong!", 5), ==, RC_OK);
    munit_assert_int(unix_socket_client_read(client, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 5);
    munit_assert_memory_equal(5, buf, "pong!");

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_write_zero_length(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    munit_assert_int(unix_socket_client_write(client, NULL, 0), ==, RC_OK);
    munit_assert_int(unix_socket_server_write(server, NULL, 0), ==, RC_OK);
    munit_assert_true(unix_socket_client_is_connected(client));
    munit_assert_true(unix_socket_server_has_client(server));

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

#define LARGE_PAYLOAD_SIZE (1024 * 1024)

typedef struct
{
    unix_socket_client_t *client;
    const u8 *data;
    rc_t result;
} large_write_args_t;

static void *write_large_payload(void *arg)
{
    large_write_args_t *args = (large_write_args_t *)arg;
    args->result = unix_socket_client_write(args->client, args->data, LARGE_PAYLOAD_SIZE);
    return NULL;
}

/* A payload far larger than the socket buffer forces partial sends, which the
 * write must keep retrying until every byte is delivered in order. */
static MunitResult test_large_write_is_complete(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    u8 *sent = (u8 *)malloc(LARGE_PAYLOAD_SIZE);
    u8 *received = (u8 *)malloc(LARGE_PAYLOAD_SIZE);
    munit_assert_not_null(sent);
    munit_assert_not_null(received);
    for (size_t i = 0; i < LARGE_PAYLOAD_SIZE; i++)
    {
        sent[i] = (u8)(i * 31 + 7);
    }

    large_write_args_t args = {.client = client, .data = sent, .result = RC_ERROR};
    pthread_t writer;
    munit_assert_int(pthread_create(&writer, NULL, write_large_payload, &args), ==, 0);

    server_read_exactly(server, received, LARGE_PAYLOAD_SIZE);
    pthread_join(writer, NULL);

    munit_assert_int(args.result, ==, RC_OK);
    munit_assert_memory_equal(LARGE_PAYLOAD_SIZE, received, sent);

    free(sent);
    free(received);
    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_server_sees_client_hang_up(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    unix_socket_client_free(client);

    char buf[8];
    size_t n = 123;
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 0);
    munit_assert_false(unix_socket_server_has_client(server));
    munit_assert_int(unix_socket_server_get_client_fd(server), ==, -1);
    munit_assert_int(unix_socket_server_write(server, "x", 1), ==, RC_NOT_INITIALIZED);

    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_client_sees_server_hang_up(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    munit_assert_int(unix_socket_server_disconnect_client(server), ==, RC_OK);
    munit_assert_false(unix_socket_server_has_client(server));

    char buf[8];
    size_t n = 123;
    munit_assert_int(unix_socket_client_read(client, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 0);
    munit_assert_false(unix_socket_client_is_connected(client));
    munit_assert_int(unix_socket_client_write(client, "x", 1), ==, RC_NOT_INITIALIZED);

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

/* Writing to a peer that has gone away must report an error rather than
 * killing the process with SIGPIPE. */
static MunitResult test_write_to_closed_peer(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    munit_assert_int(unix_socket_client_disconnect(client), ==, RC_OK);
    munit_assert_int(unix_socket_server_write(server, "x", 1), ==, RC_ERROR);
    munit_assert_false(unix_socket_server_has_client(server));

    munit_assert_int(unix_socket_client_connect(client), ==, RC_OK);
    munit_assert_int(unix_socket_server_accept(server), ==, RC_OK);
    munit_assert_int(unix_socket_server_disconnect_client(server), ==, RC_OK);
    munit_assert_int(unix_socket_client_write(client, "x", 1), ==, RC_ERROR);
    munit_assert_false(unix_socket_client_is_connected(client));

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_client_reconnects(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);
    unix_socket_server_disconnect_client(server);

    munit_assert_int(unix_socket_client_connect(client), ==, RC_OK);
    munit_assert_int(unix_socket_server_accept(server), ==, RC_OK);

    char buf[8];
    size_t n = 0;
    munit_assert_int(unix_socket_client_write(client, "again", 5), ==, RC_OK);
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 5);
    munit_assert_memory_equal(5, buf, "again");

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_accept_replaces_previous_client(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    unix_socket_server_t *server;
    unix_socket_client_t *first;
    connect_pair(fixture->path, &server, &first);

    unix_socket_client_t *second = unix_socket_client_init(fixture->path);
    munit_assert_int(unix_socket_client_connect(second), ==, RC_OK);
    munit_assert_int(unix_socket_server_accept(server), ==, RC_OK);

    char buf[8];
    size_t n = 123;
    munit_assert_int(unix_socket_client_read(first, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 0);
    munit_assert_false(unix_socket_client_is_connected(first));

    munit_assert_int(unix_socket_client_write(second, "2nd", 3), ==, RC_OK);
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), &n), ==, RC_OK);
    munit_assert_size(n, ==, 3);
    munit_assert_memory_equal(3, buf, "2nd");

    unix_socket_client_free(first);
    unix_socket_client_free(second);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

static MunitResult test_null_safety(const MunitParameter params[], void *user_data)
{
    (void)params;
    socket_fixture_t *fixture = (socket_fixture_t *)user_data;

    char buf[8];
    size_t n = 0;

    munit_assert_int(unix_socket_client_free(NULL), ==, RC_OK);
    munit_assert_int(unix_socket_client_connect(NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_client_disconnect(NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_false(unix_socket_client_is_connected(NULL));
    munit_assert_int(unix_socket_client_get_fd(NULL), ==, -1);
    munit_assert_int(unix_socket_client_read(NULL, buf, sizeof(buf), &n), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_client_write(NULL, "x", 1), ==, RC_BAD_INPUT_ARG);

    munit_assert_int(unix_socket_server_free(NULL), ==, RC_OK);
    munit_assert_int(unix_socket_server_accept(NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_server_disconnect_client(NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_false(unix_socket_server_has_client(NULL));
    munit_assert_int(unix_socket_server_get_listen_fd(NULL), ==, -1);
    munit_assert_int(unix_socket_server_get_client_fd(NULL), ==, -1);
    munit_assert_int(unix_socket_server_read(NULL, buf, sizeof(buf), &n), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_server_write(NULL, "x", 1), ==, RC_BAD_INPUT_ARG);

    unix_socket_server_t *server;
    unix_socket_client_t *client;
    connect_pair(fixture->path, &server, &client);

    munit_assert_int(unix_socket_client_read(client, NULL, sizeof(buf), &n), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_client_read(client, buf, sizeof(buf), NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_client_write(client, NULL, 1), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_server_read(server, NULL, sizeof(buf), &n), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_server_read(server, buf, sizeof(buf), NULL), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(unix_socket_server_write(server, NULL, 1), ==, RC_BAD_INPUT_ARG);

    /* Rejected arguments must not tear down a healthy connection. */
    munit_assert_true(unix_socket_client_is_connected(client));
    munit_assert_true(unix_socket_server_has_client(server));

    unix_socket_client_free(client);
    unix_socket_server_free(server);
    return MUNIT_OK;
}

#define FIXTURE_TEST(NAME, FN) {(char *)(NAME), (FN), fixture_setup, fixture_teardown, MUNIT_TEST_OPTION_NONE, NULL}

static MunitTest test_suite_tests[] = {
    {(char *)"/init/invalid-path", test_init_invalid_path, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    FIXTURE_TEST("/server/socket-file-lifecycle", test_server_socket_file_lifecycle),
    FIXTURE_TEST("/server/replaces-stale-file", test_server_replaces_stale_file),
    FIXTURE_TEST("/server/sees-client-hang-up", test_server_sees_client_hang_up),
    FIXTURE_TEST("/server/accept-replaces-client", test_accept_replaces_previous_client),
    FIXTURE_TEST("/client/connect-without-server", test_client_connect_without_server),
    FIXTURE_TEST("/client/sees-server-hang-up", test_client_sees_server_hang_up),
    FIXTURE_TEST("/client/reconnects", test_client_reconnects),
    FIXTURE_TEST("/io/not-connected", test_not_connected),
    FIXTURE_TEST("/io/round-trip", test_round_trip),
    FIXTURE_TEST("/io/zero-length-write", test_write_zero_length),
    FIXTURE_TEST("/io/large-write", test_large_write_is_complete),
    FIXTURE_TEST("/io/write-to-closed-peer", test_write_to_closed_peer),
    FIXTURE_TEST("/null-safety", test_null_safety),
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

static const MunitSuite test_suite = {(char *)"crox-ipc", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

/* The library logs every connect and hang-up; the tests provoke plenty of
 * both, and munit's report is easier to read without them. */
static void discard_log(LfLogLevel level, const char *file, int file_len, int line, const char *msg, va_list args)
{
    (void)level;
    (void)file;
    (void)file_len;
    (void)line;
    (void)msg;
    (void)args;
}

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    lfRegister(discard_log);
    return munit_suite_main(&test_suite, NULL, argc, argv);
}

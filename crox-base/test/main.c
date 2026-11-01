
#include "crox_utils.h"
#include "crox_types.h"
#include "munit.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/* Run a replacement over a pre-zeroed stack buffer, so stray writes past the
 * string are caught by the assertions that follow. */
#define REPLACE_PREPARE(buf, text)     \
    memset(buf, 0, sizeof(buf));       \
    memcpy(buf, text, sizeof(text));

/* Epoch time in ms should track the wall clock closely enough for a test. */
static MunitResult test_timestamp_sane(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    const u64 t1 = get_timestamp_ms();
    const u64 t2 = get_timestamp_ms();

    /* Later calls report an equal or later time, and terminate promptly. */
    munit_assert_uint64(t2, >=, t1);
    munit_assert_uint64(t2, <, t1 + 5000);

    /* A minute of slack either way against the coarse wall clock. */
    const u64 now_ms = (u64)time(NULL) * 1000U;
    munit_assert_uint64(t1 + 60000U, >, now_ms);
    munit_assert_uint64(now_ms + 60000U, >, t1);

    return MUNIT_OK;
}

/* Growing a single match by substitution in the middle of the string. */
static MunitResult test_replace_ip_single(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    REPLACE_PREPARE(buf, "hello world");
    str_replace_ip(buf, sizeof(buf), "world", "there");
    munit_assert_string_equal(buf, "hello there");

    /* Empty find is a no-op, not a match on every position. */
    str_replace_ip(buf, sizeof(buf), "", "XX");
    munit_assert_string_equal(buf, "hello there");

    return MUNIT_OK;
}

/* A pattern that never matches leaves the string untouched; the same holds for
 * the empty string and for an input that is already at capacity. */
static MunitResult test_replace_ip_no_match(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    REPLACE_PREPARE(buf, "hello world");
    str_replace_ip(buf, sizeof(buf), "zzz", "nope");
    munit_assert_string_equal(buf, "hello world");

    char empty[4];
    REPLACE_PREPARE(empty, "");
    str_replace_ip(empty, sizeof(empty), "a", "bb");
    munit_assert_string_equal(empty, "");

    return MUNIT_OK;
}

/* Shrinking removes the pattern and leaves the remainder glued together. */
static MunitResult test_replace_ip_shrink(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    REPLACE_PREPARE(buf, "hello there world");
    str_replace_ip(buf, sizeof(buf), "there ", "");
    munit_assert_string_equal(buf, "hello world");

    /* Repeated shrinks: every other "aa" of a run is consumed. */
    char run[8];
    REPLACE_PREPARE(run, "aaaa");
    str_replace_ip(run, sizeof(run), "aa", "b");
    munit_assert_string_equal(run, "bb");

    return MUNIT_OK;
}

/* Multiple growing replacements, including one at the very end of the string.
 * Buffer sizes matter here: the tail move count must not underflow. */
static MunitResult test_replace_ip_multi_grow(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    REPLACE_PREPARE(buf, "one fish two fish");
    str_replace_ip(buf, sizeof(buf), "fish", "cat");
    munit_assert_string_equal(buf, "one cat two cat");

    /* Regression: this input used to underflow the tail move count. */
    char run[16];
    REPLACE_PREPARE(run, "aaa");
    str_replace_ip(run, sizeof(run), "a", "bb");
    munit_assert_string_equal(run, "bbbbbb");

    return MUNIT_OK;
}

/* Non-overlapping left-to-right scanning: after a match, the search resumes
 * past the replacement rather than spilling into overlapping matches. */
static MunitResult test_replace_ip_adjacent(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[8];
    REPLACE_PREPARE(buf, "aaa");
    str_replace_ip(buf, sizeof(buf), "aa", "b");
    munit_assert_string_equal(buf, "ba");

    return MUNIT_OK;
}

/* A replacement that contains the pattern must not loop forever. */
static MunitResult test_replace_ip_replacement_contains_find(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char one[16];
    REPLACE_PREPARE(one, "a");
    str_replace_ip(one, sizeof(one), "a", "aa");
    munit_assert_string_equal(one, "aa");

    char many[16];
    REPLACE_PREPARE(many, "aa");
    str_replace_ip(many, sizeof(many), "a", "aa");
    munit_assert_string_equal(many, "aaaa");

    return MUNIT_OK;
}

/* Matches at the leading and trailing edges of the string. */
static MunitResult test_replace_ip_boundaries(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char start[16];
    REPLACE_PREPARE(start, "start_end");
    str_replace_ip(start, sizeof(start), "start", "grow");
    munit_assert_string_equal(start, "grow_end");

    char end[16];
    REPLACE_PREPARE(end, "start_end");
    str_replace_ip(end, sizeof(end), "end", "finish");
    munit_assert_string_equal(end, "start_finish");

    return MUNIT_OK;
}

/* A result that exactly fits (length + NUL) succeeds; one byte short aborts
 * and leaves the original string intact and terminated. */
static MunitResult test_replace_ip_capacity_boundary(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char fits[4];
    REPLACE_PREPARE(fits, "ab");
    str_replace_ip(fits, sizeof(fits), "a", "xy");
    munit_assert_string_equal(fits, "xyb");

    char tight[3];
    REPLACE_PREPARE(tight, "ab");
    str_replace_ip(tight, sizeof(tight), "a", "xy");
    munit_assert_string_equal(tight, "ab");

    return MUNIT_OK;
}

/* When replacement would overflow, the call stops without writing past the
 * buffer. Work already done before the overflow stays behind as a valid
 * NUL-terminated string (best effort, no rollback). */
static MunitResult test_replace_ip_capacity_overflow(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[8];
    REPLACE_PREPARE(buf, "aaaa");
    str_replace_ip(buf, sizeof(buf), "aa", "XYZW");
    munit_assert_string_equal(buf, "XYZWaa");
    munit_assert_size(strlen(buf), <, sizeof(buf));

    char simple[4];
    REPLACE_PREPARE(simple, "aa");
    str_replace_ip(simple, sizeof(simple), "aa", "abcd");
    munit_assert_string_equal(simple, "aa");

    return MUNIT_OK;
}

/* The copy variant replaces into an untouched copy of the input. */
static MunitResult test_replace_cp_basic(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    const char in[] = "hello world";
    char out[32];
    memset(out, 0, sizeof(out));

    str_replace_cp(in, out, sizeof(out), "world", "there");
    munit_assert_string_equal(out, "hello there");

    /* The input buffer is never modified. */
    munit_assert_string_equal(in, "hello world");

    /* An empty find degenerates to a plain copy. */
    str_replace_cp(in, out, sizeof(out), "", "XX");
    munit_assert_string_equal(out, in);

    return MUNIT_OK;
}

/* Size is honored: the input is truncated first and then replaced. */
static MunitResult test_replace_cp_truncation(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char out[16];
    memset(out, 0, sizeof(out));

    /* The pattern straddles the truncation point, so it disappears along with
     * the truncated tail before any replacement happens. */
    str_replace_cp("abcdefgh", out, 5, "gh", "XY");
    munit_assert_string_equal(out, "abcd");

    /* Truncation to a single character, which is then replaced. */
    char tiny[2];
    memset(tiny, 0, sizeof(tiny));
    str_replace_cp("ab", tiny, sizeof(tiny), "a", "z");
    munit_assert_string_equal(tiny, "z");

    return MUNIT_OK;
}

/* An empty input copies through as an empty string. */
static MunitResult test_replace_cp_empty_input(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char out[8];
    memset(out, 0, sizeof(out));
    str_replace_cp("", out, sizeof(out), "a", "bb");
    munit_assert_string_equal(out, "");

    return MUNIT_OK;
}

/* The join buffer is pre-zeroed so stray writes past the string get caught. */
#define JOIN_PREPARE(buf) memset(buf, 0, sizeof(buf));

/* Several tokens joined with a multi-character delimiter, with an empty
 * token counted in the middle. */
static MunitResult test_str_join_basic(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    JOIN_PREPARE(buf);

    char *tokens[] = {"one", "", "two", "three"};
    str_join(buf, sizeof(buf), tokens, 4, ", ");
    munit_assert_string_equal(buf, "one, , two, three");

    return MUNIT_OK;
}

/* A single token produces no delimiter; zero tokens produce an empty string
 * even with no token array at all. */
static MunitResult test_str_join_degenerate_counts(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[16];
    JOIN_PREPARE(buf);

    char *solo[] = {"solo"};
    str_join(buf, sizeof(buf), solo, 1, ", ");
    munit_assert_string_equal(buf, "solo");

    str_join(buf, sizeof(buf), NULL, 0, ", ");
    munit_assert_string_equal(buf, "");

    return MUNIT_OK;
}

/* An empty delimiter degenerates to plain concatenation. */
static MunitResult test_str_join_empty_delimiter(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[32];
    JOIN_PREPARE(buf);

    char *tokens[] = {"GET", "/index.html", "HTTP/1.1"};
    str_join(buf, sizeof(buf), tokens, 3, "");
    munit_assert_string_equal(buf, "GET/index.htmlHTTP/1.1");

    return MUNIT_OK;
}

/* Empty leading tokens still emit their delimiters. */
static MunitResult test_str_join_empty_tokens(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[16];
    JOIN_PREPARE(buf);

    char *tokens[] = {"", "", "a"};
    str_join(buf, sizeof(buf), tokens, 3, "-");
    munit_assert_string_equal(buf, "--a");

    return MUNIT_OK;
}

/* An oversized token truncates mid-way: whatever snprintf already placed
 * stays behind and the string remains terminated. */
static MunitResult test_str_join_truncate_token(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[5];
    JOIN_PREPARE(buf);

    char *tokens[] = {"hello", "world"};
    str_join(buf, sizeof(buf), tokens, 2, "");
    munit_assert_string_equal(buf, "hell");
    munit_assert_size(strlen(buf), <, sizeof(buf));

    return MUNIT_OK;
}

/* The delimiter is counted too: result the size of the buffer minus one fits
 * exactly, one byte short keeps the truncated delimiter written so far. */
static MunitResult test_str_join_capacity_boundary(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char fits[6];
    JOIN_PREPARE(fits);
    char *tokens[] = {"aa", "bb"};

    str_join(fits, sizeof(fits), tokens, 2, ",");
    munit_assert_string_equal(fits, "aa,bb");

    char tight[5];
    JOIN_PREPARE(tight);
    str_join(tight, sizeof(tight), tokens, 2, ",");
    munit_assert_string_equal(tight, "aa,b");
    munit_assert_size(strlen(tight), <, sizeof(tight));

    return MUNIT_OK;
}

/* Tokens already known to fit leave earlier tokens intact once a later one
 * runs out of room. */
static MunitResult test_str_join_truncate_prefix(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    char buf[5];
    JOIN_PREPARE(buf);

    char *tokens[] = {"abc", "def", "ghi"};
    str_join(buf, sizeof(buf), tokens, 3, "");
    munit_assert_string_equal(buf, "abcd");

    /* One byte smaller: the partial character has no room either. */
    char tiny[4];
    JOIN_PREPARE(tiny);
    str_join(tiny, sizeof(tiny), tokens, 3, "");
    munit_assert_string_equal(tiny, "abc");

    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {(char *)"/timestamp/sane",               test_timestamp_sane,                        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/single",            test_replace_ip_single,                     NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/no-match",          test_replace_ip_no_match,                   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/shrink",            test_replace_ip_shrink,                     NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/multi-grow",        test_replace_ip_multi_grow,                 NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/adjacent",          test_replace_ip_adjacent,                   NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/replacement-own",   test_replace_ip_replacement_contains_find,  NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/boundaries",        test_replace_ip_boundaries,                 NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/capacity/boundary", test_replace_ip_capacity_boundary,          NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/ip/capacity/overflow", test_replace_ip_capacity_overflow,          NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/cp/basic",             test_replace_cp_basic,                      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/cp/truncation",        test_replace_cp_truncation,                 NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace/cp/empty-input",       test_replace_cp_empty_input,                NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/basic",                   test_str_join_basic,                        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/counts",                  test_str_join_degenerate_counts,            NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/empty-delimiter",         test_str_join_empty_delimiter,              NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/empty-tokens",            test_str_join_empty_tokens,                 NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/truncate-token",          test_str_join_truncate_token,               NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/capacity/boundary",       test_str_join_capacity_boundary,            NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/join/truncate-prefix",         test_str_join_truncate_prefix,              NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}
};

static const MunitSuite test_suite = {(char *)"crox-base", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&test_suite, NULL, argc, argv);
}

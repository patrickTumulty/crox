
#include "crox_types.h"
#include "greedy_matrix.h"
#include "matrix.h"
#include "standard_matrix.h"
#include "munit.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Every test in this suite runs once per implementation. The only thing that
 * varies is which initializer is used: the rest of the API is shared, so the
 * same assertions must hold for both the greedy and standard matrices. */
static MunitParameterEnum test_params[] = {
    {(char *)"impl", (char *[]){(char *)"greedy", (char *)"standard", NULL}},
    {NULL, NULL},
};

/* Signature shared by every typed (per-variant) initializer. */
typedef Rc (*matrix_init_fn)(matrix_t *out, int32_t height, int32_t width);

/* Pick the typed initializer for @p token on the named implementation. */
static matrix_init_fn variant_init(const char *impl, const char *token)
{
    bool standard = strcmp(impl, "standard") == 0;
    if (strcmp(token, "i8") == 0)
        return standard ? matrix_init_i8 : matrix_init_greedy_i8;
    if (strcmp(token, "u8") == 0)
        return standard ? matrix_init_u8 : matrix_init_greedy_u8;
    if (strcmp(token, "i16") == 0)
        return standard ? matrix_init_i16 : matrix_init_greedy_i16;
    if (strcmp(token, "u16") == 0)
        return standard ? matrix_init_u16 : matrix_init_greedy_u16;
    if (strcmp(token, "i32") == 0)
        return standard ? matrix_init_i32 : matrix_init_greedy_i32;
    if (strcmp(token, "u32") == 0)
        return standard ? matrix_init_u32 : matrix_init_greedy_u32;
    if (strcmp(token, "i64") == 0)
        return standard ? matrix_init_i64 : matrix_init_greedy_i64;
    if (strcmp(token, "u64") == 0)
        return standard ? matrix_init_u64 : matrix_init_greedy_u64;
    if (strcmp(token, "f32") == 0)
        return standard ? matrix_init_f32 : matrix_init_greedy_f32;
    if (strcmp(token, "f64") == 0)
        return standard ? matrix_init_f64 : matrix_init_greedy_f64;
    if (strcmp(token, "bool") == 0)
        return standard ? matrix_init_bool : matrix_init_greedy_bool;
    if (strcmp(token, "ptr") == 0)
        return standard ? matrix_init_ptr : matrix_init_greedy_ptr;
    return NULL;
}

/* Create a matrix on the named implementation through the untyped entry point.
 * Only needed for a cell type that has no built-in variant, such as a struct. */
static Rc init_generic(const char *impl, matrix_t *out, int32_t height, int32_t width, size_t typeSize)
{
    if (strcmp(impl, "standard") == 0)
        return matrix_init(out, height, width, typeSize);
    return matrix_init_greedy(out, height, width, typeSize);
}

/* Basic creation: a matrix reports the dimensions it was created with. */
static MunitResult test_create(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i32_t mat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&mat, 10, 20), ==, RC_OK);
    munit_assert_not_null(mat);
    munit_assert_int32(matrix_get_height(mat), ==, 10);
    munit_assert_int32(matrix_get_width(mat), ==, 20);
    matrix_free(mat);
    return MUNIT_OK;
}

/* Dimension queries on a NULL matrix are safe and report zero. */
static MunitResult test_dimensions_null(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    munit_assert_int32(matrix_get_width(NULL), ==, 0);
    munit_assert_int32(matrix_get_height(NULL), ==, 0);
    return MUNIT_OK;
}

/* int32_t values written anywhere in the matrix can be read back. */
static MunitResult test_get_set_int32(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    const int32_t height = 5;
    const int32_t width = 7;
    matrix_i32_t mat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&mat, height, width), ==, RC_OK);
    munit_assert_not_null(mat);

    int32_t counter = 0;
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            munit_assert_int(matrix_set_i32(mat, x, y, counter++), ==, RC_OK);
        }
    }

    counter = 0;
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            munit_assert_int32(matrix_get_i32(mat, x, y), ==, counter++);
        }
    }

    matrix_free(mat);
    return MUNIT_OK;
}

/* 8-bit signed and unsigned integers. */
static MunitResult test_get_set_8bit(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i8_t mat = NULL;
    munit_assert_int(variant_init(impl, "i8")(&mat, 2, 2), ==, RC_OK);
    munit_assert_not_null(mat);

    munit_assert_int(matrix_set_i8(mat, 0, 0, INT8_C(-128)), ==, RC_OK);
    munit_assert_int(matrix_set_i8(mat, 1, 1, INT8_C(127)), ==, RC_OK);
    munit_assert_int8(matrix_get_i8(mat, 0, 0), ==, INT8_C(-128));
    munit_assert_int8(matrix_get_i8(mat, 1, 1), ==, INT8_C(127));

    matrix_free(mat);

    matrix_u8_t umat = NULL;
    munit_assert_int(variant_init(impl, "u8")(&umat, 2, 2), ==, RC_OK);
    munit_assert_not_null(umat);

    munit_assert_int(matrix_set_u8(umat, 0, 0, UINT8_C(0)), ==, RC_OK);
    munit_assert_int(matrix_set_u8(umat, 1, 1, UINT8_C(255)), ==, RC_OK);
    munit_assert_uint8(matrix_get_u8(umat, 0, 0), ==, UINT8_C(0));
    munit_assert_uint8(matrix_get_u8(umat, 1, 1), ==, UINT8_C(255));

    matrix_free(umat);
    return MUNIT_OK;
}

/* 16-bit signed and unsigned integers. */
static MunitResult test_get_set_16bit(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i16_t mat = NULL;
    munit_assert_int(variant_init(impl, "i16")(&mat, 2, 2), ==, RC_OK);
    munit_assert_not_null(mat);

    munit_assert_int(matrix_set_i16(mat, 0, 0, INT16_C(-32768)), ==, RC_OK);
    munit_assert_int(matrix_set_i16(mat, 1, 1, INT16_C(32767)), ==, RC_OK);
    munit_assert_int16(matrix_get_i16(mat, 0, 0), ==, INT16_C(-32768));
    munit_assert_int16(matrix_get_i16(mat, 1, 1), ==, INT16_C(32767));

    matrix_free(mat);

    matrix_u16_t umat = NULL;
    munit_assert_int(variant_init(impl, "u16")(&umat, 2, 2), ==, RC_OK);
    munit_assert_not_null(umat);

    munit_assert_int(matrix_set_u16(umat, 0, 0, UINT16_C(0)), ==, RC_OK);
    munit_assert_int(matrix_set_u16(umat, 1, 1, UINT16_C(65535)), ==, RC_OK);
    munit_assert_uint16(matrix_get_u16(umat, 0, 0), ==, UINT16_C(0));
    munit_assert_uint16(matrix_get_u16(umat, 1, 1), ==, UINT16_C(65535));

    matrix_free(umat);
    return MUNIT_OK;
}

/* 32-bit signed/unsigned integers and float. */
static MunitResult test_get_set_32bit(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i32_t imat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&imat, 2, 2), ==, RC_OK);
    munit_assert_not_null(imat);

    munit_assert_int(matrix_set_i32(imat, 0, 0, INT32_C(-2147483647)), ==, RC_OK);
    munit_assert_int(matrix_set_i32(imat, 1, 1, INT32_C(2147483647)), ==, RC_OK);
    munit_assert_int32(matrix_get_i32(imat, 0, 0), ==, INT32_C(-2147483647));
    munit_assert_int32(matrix_get_i32(imat, 1, 1), ==, INT32_C(2147483647));

    matrix_free(imat);

    matrix_u32_t umat = NULL;
    munit_assert_int(variant_init(impl, "u32")(&umat, 2, 2), ==, RC_OK);
    munit_assert_not_null(umat);

    munit_assert_int(matrix_set_u32(umat, 0, 0, UINT32_C(0)), ==, RC_OK);
    munit_assert_int(matrix_set_u32(umat, 1, 1, UINT32_C(4294967295)), ==, RC_OK);
    munit_assert_uint32(matrix_get_u32(umat, 0, 0), ==, UINT32_C(0));
    munit_assert_uint32(matrix_get_u32(umat, 1, 1), ==, UINT32_C(4294967295));

    matrix_free(umat);

    matrix_f32_t fmat = NULL;
    munit_assert_int(variant_init(impl, "f32")(&fmat, 2, 2), ==, RC_OK);
    munit_assert_not_null(fmat);

    munit_assert_int(matrix_set_f32(fmat, 0, 0, -3.14159265f), ==, RC_OK);
    munit_assert_int(matrix_set_f32(fmat, 1, 1, 3.14159265f), ==, RC_OK);
    munit_assert_float(matrix_get_f32(fmat, 0, 0), ==, -3.14159265f);
    munit_assert_float(matrix_get_f32(fmat, 1, 1), ==, 3.14159265f);

    matrix_free(fmat);
    return MUNIT_OK;
}

/* 64-bit signed/unsigned integers and double. */
static MunitResult test_get_set_64bit(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i64_t lmat = NULL;
    munit_assert_int(variant_init(impl, "i64")(&lmat, 2, 2), ==, RC_OK);
    munit_assert_not_null(lmat);

    munit_assert_int(matrix_set_i64(lmat, 0, 0, INT64_C(-9223372036854775807)), ==, RC_OK);
    munit_assert_int(matrix_set_i64(lmat, 1, 1, INT64_C(9223372036854775807)), ==, RC_OK);
    munit_assert_int64(matrix_get_i64(lmat, 0, 0), ==, INT64_C(-9223372036854775807));
    munit_assert_int64(matrix_get_i64(lmat, 1, 1), ==, INT64_C(9223372036854775807));

    matrix_free(lmat);

    matrix_u64_t ulmat = NULL;
    munit_assert_int(variant_init(impl, "u64")(&ulmat, 2, 2), ==, RC_OK);
    munit_assert_not_null(ulmat);

    munit_assert_int(matrix_set_u64(ulmat, 0, 0, UINT64_C(0)), ==, RC_OK);
    munit_assert_int(matrix_set_u64(ulmat, 1, 1, UINT64_C(18446744073709551615)), ==, RC_OK);
    munit_assert_uint64(matrix_get_u64(ulmat, 0, 0), ==, UINT64_C(0));
    munit_assert_uint64(matrix_get_u64(ulmat, 1, 1), ==, UINT64_C(18446744073709551615));

    matrix_free(ulmat);

    matrix_f64_t dmat = NULL;
    munit_assert_int(variant_init(impl, "f64")(&dmat, 2, 2), ==, RC_OK);
    munit_assert_not_null(dmat);

    munit_assert_int(matrix_set_f64(dmat, 0, 0, -2.718281828459045), ==, RC_OK);
    munit_assert_int(matrix_set_f64(dmat, 1, 1, 2.718281828459045), ==, RC_OK);
    munit_assert_double(matrix_get_f64(dmat, 0, 0), ==, -2.718281828459045);
    munit_assert_double(matrix_get_f64(dmat, 1, 1), ==, 2.718281828459045);

    matrix_free(dmat);
    return MUNIT_OK;
}

/* Boolean values. */
static MunitResult test_get_set_bool(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_bool_t mat = NULL;
    munit_assert_int(variant_init(impl, "bool")(&mat, 2, 2), ==, RC_OK);
    munit_assert_not_null(mat);

    munit_assert_int(matrix_set_bool(mat, 0, 0, true), ==, RC_OK);
    munit_assert_int(matrix_set_bool(mat, 1, 1, false), ==, RC_OK);
    munit_assert_true(matrix_get_bool(mat, 0, 0));
    munit_assert_false(matrix_get_bool(mat, 1, 1));

    matrix_free(mat);
    return MUNIT_OK;
}

typedef struct
{
    int x;
    int y;
    int sum;
} Point;

/* Pointer values: store, retrieve, and mutate structs through the matrix. */
static MunitResult test_get_set_ptr(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    const int32_t count = 8;
    Point points[8] = {
        {0, 0, 0}, {1, 2, 0}, {3, 4, 0}, {5, 6, 0},
        {7, 8, 0}, {9, 10, 0}, {11, 12, 0}, {13, 14, 0},
    };

    matrix_ptr_t mat = NULL;
    munit_assert_int(variant_init(impl, "ptr")(&mat, 2, 4), ==, RC_OK);
    munit_assert_not_null(mat);

    /* Store pointers to the structs in the matrix. */
    for (int32_t i = 0; i < count; i++)
    {
        int32_t x = i % matrix_get_width(mat);
        int32_t y = i / matrix_get_width(mat);
        munit_assert_int(matrix_set_ptr(mat, x, y, &points[i]), ==, RC_OK);
    }

    /* Retrieve and verify initial values. */
    for (int32_t i = 0; i < count; i++)
    {
        int32_t x = i % matrix_get_width(mat);
        int32_t y = i / matrix_get_width(mat);
        Point *p = matrix_get_ptr(mat, x, y);
        munit_assert_ptr_equal(p, &points[i]);
        munit_assert_int32(p->x, ==, points[i].x);
        munit_assert_int32(p->y, ==, points[i].y);
    }

    /* Mutate the structs through the matrix pointers. */
    for (int32_t i = 0; i < count; i++)
    {
        int32_t x = i % matrix_get_width(mat);
        int32_t y = i / matrix_get_width(mat);
        Point *p = matrix_get_ptr(mat, x, y);
        p->sum = p->x + p->y;
    }

    /* Verify mutations landed in the original structs. */
    for (int32_t i = 0; i < count; i++)
    {
        munit_assert_int32(points[i].sum, ==, points[i].x + points[i].y);
    }

    matrix_free(mat);
    return MUNIT_OK;
}

/* The untyped entry point stores a struct by value; matrix_get_cell() hands
 * back the cell's bytes so the caller can read and write it in place. */
static MunitResult test_get_cell_struct(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_t mat = NULL;
    munit_assert_int(init_generic(impl, &mat, 2, 2, sizeof(Point)), ==, RC_OK);
    munit_assert_not_null(mat);

    for (int32_t y = 0; y < 2; y++)
    {
        for (int32_t x = 0; x < 2; x++)
        {
            Point *p = matrix_get_cell(mat, x, y);
            munit_assert_not_null(p);
            p->x = x;
            p->y = y;
            p->sum = x + y;
        }
    }

    for (int32_t y = 0; y < 2; y++)
    {
        for (int32_t x = 0; x < 2; x++)
        {
            Point *p = matrix_get_cell(mat, x, y);
            munit_assert_int32(p->x, ==, x);
            munit_assert_int32(p->y, ==, y);
            munit_assert_int32(p->sum, ==, x + y);
        }
    }

    /* Out-of-bounds coordinates yield NULL. */
    munit_assert_null(matrix_get_cell(mat, 2, 0));
    munit_assert_null(matrix_get_cell(mat, 0, -1));

    matrix_free(mat);
    return MUNIT_OK;
}

/* Every typed initializer creates a usable matrix of the right dimensions. */
static MunitResult test_init_variants(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    static const char *tokens[] = {
        "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "bool", "ptr",
    };

    for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++)
    {
        matrix_init_fn init = variant_init(impl, tokens[i]);
        munit_assert_not_null(init);

        matrix_t mat = NULL;
        munit_assert_int(init(&mat, 2, 3), ==, RC_OK);
        munit_assert_not_null(mat);
        munit_assert_int32(matrix_get_height(mat), ==, 2);
        munit_assert_int32(matrix_get_width(mat), ==, 3);
        matrix_free(mat);
    }

    /* A wider accessor is rejected: the cell is only 4 bytes. */
    matrix_i32_t imat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&imat, 2, 2), ==, RC_OK);
    munit_assert_int(matrix_set_i64(imat, 0, 0, INT64_C(1)), ==, RC_BAD_INPUT_ARG);
    matrix_free(imat);

    /* Invalid arguments are rejected by the typed initializers too. */
    matrix_init_fn init_i32 = variant_init(impl, "i32");
    munit_assert_int(init_i32(NULL, 2, 2), ==, RC_BAD_INPUT_ARG);
    matrix_i32_t bad = NULL;
    munit_assert_int(init_i32(&bad, 0, 2), ==, RC_BAD_INPUT_ARG);
    munit_assert_null(bad);

    return MUNIT_OK;
}

/* Resizing changes the reported dimensions and new cells are usable. */
static MunitResult test_resize(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_i32_t mat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&mat, 2, 3), ==, RC_OK);
    munit_assert_not_null(mat);
    munit_assert_int32(matrix_get_height(mat), ==, 2);
    munit_assert_int32(matrix_get_width(mat), ==, 3);

    munit_assert_int(matrix_set_i32(mat, 2, 1, 77), ==, RC_OK);

    munit_assert_int(matrix_resize(mat, 5, 7), ==, RC_OK);
    munit_assert_int32(matrix_get_height(mat), ==, 5);
    munit_assert_int32(matrix_get_width(mat), ==, 7);

    /* A standard matrix frees and reallocates on resize, so old contents are
     * gone. A greedy matrix makes no such promise, so only check the standard. */
    if (strcmp(impl, "standard") == 0)
    {
        munit_assert_int32(matrix_get_i32(mat, 2, 1), ==, 0);
    }

    /* Writing and reading inside the new bounds works. */
    munit_assert_int(matrix_set_i32(mat, 6, 4, 12345), ==, RC_OK);
    munit_assert_int32(matrix_get_i32(mat, 6, 4), ==, 12345);

    /* Resizing smaller also reports the new dimensions. */
    munit_assert_int(matrix_resize(mat, 1, 1), ==, RC_OK);
    munit_assert_int32(matrix_get_height(mat), ==, 1);
    munit_assert_int32(matrix_get_width(mat), ==, 1);

    matrix_free(mat);
    return MUNIT_OK;
}

/* Free is safe for valid matrices and for NULL. */
static MunitResult test_free_null(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_free(NULL);

    matrix_i32_t mat = NULL;
    munit_assert_int(variant_init(impl, "i32")(&mat, 1, 1), ==, RC_OK);
    munit_assert_not_null(mat);
    matrix_free(mat);

    return MUNIT_OK;
}

/* NULL handles are rejected by action functions and answered safely by the
 * query functions. */
static MunitResult test_null_safety(const MunitParameter params[], void *user_data)
{
    (void)params;
    (void)user_data;

    matrix_free(NULL);

    munit_assert_int(matrix_resize(NULL, 5, 5), ==, RC_NOT_INITIALIZED);
    munit_assert_int(matrix_set_i32(NULL, 0, 0, 1), ==, RC_NOT_INITIALIZED);

    munit_assert_int32(matrix_get_width(NULL), ==, 0);
    munit_assert_int32(matrix_get_height(NULL), ==, 0);
    munit_assert_int32(matrix_get_i32(NULL, 0, 0), ==, 0);
    munit_assert_int8(matrix_get_i8(NULL, 0, 0), ==, 0);
    munit_assert_uint8(matrix_get_u8(NULL, 0, 0), ==, 0);
    munit_assert_int16(matrix_get_i16(NULL, 0, 0), ==, 0);
    munit_assert_uint16(matrix_get_u16(NULL, 0, 0), ==, 0);
    munit_assert_uint32(matrix_get_u32(NULL, 0, 0), ==, 0);
    munit_assert_int64(matrix_get_i64(NULL, 0, 0), ==, 0);
    munit_assert_uint64(matrix_get_u64(NULL, 0, 0), ==, 0);
    munit_assert_float(matrix_get_f32(NULL, 0, 0), ==, 0.0f);
    munit_assert_double(matrix_get_f64(NULL, 0, 0), ==, 0.0);
    munit_assert_false(matrix_get_bool(NULL, 0, 0));
    munit_assert_null(matrix_get_ptr(NULL, 0, 0));
    munit_assert_null(matrix_get_cell(NULL, 0, 0));

    return MUNIT_OK;
}

/* Input validation, bounds checks and element-size guards. */
static MunitResult test_errors(const MunitParameter params[], void *user_data)
{
    (void)user_data;
    const char *impl = munit_parameters_get(params, "impl");

    matrix_init_fn init_i32 = variant_init(impl, "i32");

    /* Init guards. */
    munit_assert_int(init_i32(NULL, 2, 2), ==, RC_BAD_INPUT_ARG);
    matrix_i32_t mat = NULL;
    munit_assert_int(init_i32(&mat, 0, 2), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(init_i32(&mat, 2, 0), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(init_i32(&mat, -1, 2), ==, RC_BAD_INPUT_ARG);
    munit_assert_null(mat);

    /* The untyped entry point also rejects a zero cell size. */
    matrix_t generic = NULL;
    munit_assert_int(init_generic(impl, &generic, 2, 2, 0), ==, RC_BAD_INPUT_ARG);
    munit_assert_null(generic);

    /* Bounds checks on set and get. */
    munit_assert_int(init_i32(&mat, 2, 2), ==, RC_OK);
    munit_assert_not_null(mat);

    munit_assert_int(matrix_set_i32(mat, 2, 0, 1), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(matrix_set_i32(mat, 0, 2, 1), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(matrix_set_i32(mat, -1, 0, 1), ==, RC_BAD_INPUT_ARG);
    munit_assert_int32(matrix_get_i32(mat, 2, 0), ==, 0);
    munit_assert_int32(matrix_get_i32(mat, 0, -1), ==, 0);
    matrix_free(mat);

    /* Accessor value larger than the element size is rejected. */
    matrix_u8_t tiny = NULL;
    munit_assert_int(variant_init(impl, "u8")(&tiny, 2, 2), ==, RC_OK);
    munit_assert_not_null(tiny);
    munit_assert_int(matrix_set_i32(tiny, 0, 0, 5), ==, RC_BAD_INPUT_ARG);
    munit_assert_int32(matrix_get_i32(tiny, 0, 0), ==, 0);
    matrix_free(tiny);

    /* Resize guards leave the matrix unchanged. */
    matrix_i32_t rmat = NULL;
    munit_assert_int(init_i32(&rmat, 2, 2), ==, RC_OK);
    munit_assert_int(matrix_resize(rmat, 0, 2), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(matrix_resize(rmat, 2, 0), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(matrix_resize(rmat, -1, 2), ==, RC_BAD_INPUT_ARG);
    munit_assert_int(matrix_resize(NULL, 2, 2), ==, RC_NOT_INITIALIZED);
    munit_assert_int32(matrix_get_height(rmat), ==, 2);
    munit_assert_int32(matrix_get_width(rmat), ==, 2);
    matrix_free(rmat);

    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {(char *)"/create", test_create, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/dimensions/null", test_dimensions_null, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/int32", test_get_set_int32, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/8bit", test_get_set_8bit, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/16bit", test_get_set_16bit, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/32bit", test_get_set_32bit, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/64bit", test_get_set_64bit, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/bool", test_get_set_bool, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-set/ptr", test_get_set_ptr, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/get-cell/struct", test_get_cell_struct, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/init/variants", test_init_variants, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/resize", test_resize, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/free/null", test_free_null, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/null-safety", test_null_safety, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {(char *)"/errors", test_errors, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"crox",
    test_suite_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&test_suite, NULL, argc, argv);
}

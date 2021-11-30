/**
 * @file unit-test-cmd_args.c
 * @brief Unit tests for arguments errors cases
 *
 * @author J.-C. Chappelier, EPFL
 * @date 2021
 */

// for thread-safe randomization
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <check.h>
#include <inttypes.h>

#include "tests.h"
#include "imgStore.h"

#define PICTDB_TEST_FILE "tmp-unit-test-cmd_args.pictdb"

// ======================================================================
// tool macro
#define init_imgst(X) \
    struct imgst_file X = { \
      .header.max_files   = 10, \
      .header.res_resized = { 64, 64, 256, 256} \
    }
    
// ======================================================================
START_TEST(do_list_arguments)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    do_list(NULL, 0); // simple call, shall not SegFault
#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
START_TEST(do_create_arguments)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(myfile);
    
    ck_assert_int_eq(do_create(NULL, NULL)            , ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_create(NULL, &myfile)         , ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_create(PICTDB_TEST_FILE, NULL), ERR_INVALID_ARGUMENT);

    ck_assert_int_eq(do_create(PICTDB_TEST_FILE, &myfile), ERR_NONE);
    do_close(&myfile);
    
#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
START_TEST(do_delete_arguments)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(myfile);
    ck_assert_int_eq(do_create(PICTDB_TEST_FILE, &myfile), ERR_NONE);

    ck_assert_int_eq(do_delete(NULL, NULL)   , ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_delete(NULL, &myfile), ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_delete("foo", NULL)  , ERR_INVALID_ARGUMENT);

    ck_assert_int_eq(do_delete("foo", &myfile), ERR_FILE_NOT_FOUND);
    do_close(&myfile);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
START_TEST(do_open_arguments)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(myfile);

    // NULL argument
    ck_assert_int_eq(do_open(NULL            , "rb", &myfile),
                     ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_open(PICTDB_TEST_FILE, NULL, &myfile),
                     ERR_INVALID_ARGUMENT);
    ck_assert_int_eq(do_open(PICTDB_TEST_FILE, "rb", NULL   ),
                     ERR_INVALID_ARGUMENT);
    
    // file does not exist
    ck_assert_int_eq(do_open("/shallnotexists.txt", "rb", &myfile),
                     ERR_IO);

    // file exist but has wrong content
    static const char* dummy_name = "dummy.txt";
    FILE* dummy = fopen(dummy_name, "w");
    ck_assert_ptr_nonnull(dummy);
    fputs("Please erase that file !\n", dummy);
    fputs("This is dummy content.\n"  , dummy);
    fclose(dummy);
    ck_assert_int_eq(do_open(dummy_name, "rb", &myfile), ERR_IO);
    remove(dummy_name);
    
    // OK case
    ck_assert_int_eq(do_open(PICTDB_TEST_FILE, "rb", &myfile),
                     ERR_NONE);
    do_close(&myfile);

    // IMPORTANT: better placed @ end of last test
    remove(PICTDB_TEST_FILE);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
Suite* argserror_test_suite()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
    srand(time(NULL) ^ getpid() ^ pthread_self());
#pragma GCC diagnostic pop

    Suite* s = suite_create("Tests of (some) error cases in arguments");

    Add_Case(s, tc1, "Error tests");
    tcase_add_test(tc1, do_list_arguments  );
    tcase_add_test(tc1, do_create_arguments);
    tcase_add_test(tc1, do_delete_arguments);
    tcase_add_test(tc1, do_open_arguments);

    return s;
}

TEST_SUITE(argserror_test_suite)

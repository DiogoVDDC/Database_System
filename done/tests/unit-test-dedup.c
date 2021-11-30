/**
 * @file unit-test-dedup.c
 * @brief Unit tests for dedup feature
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
#include "dedup.h"

#define MAX_FILES 9

// ======================================================================
// tool macro
#define init_imgst(X) \
    struct imgst_file X = { \
      .header.max_files   = MAX_FILES, \
      .header.res_resized = { 64, 64, 256, 256} \
    }; \
    ck_assert_ptr_nonnull((X).metadata = calloc(X.header.max_files, sizeof(struct img_metadata)))

// ------------------------------------------------------------
static void release_imgst(struct imgst_file* imgst)
{
  free(imgst->metadata);
  imgst->metadata = NULL;
}
  
// ------------------------------------------------------------
static void insert(struct imgst_file* imgst, uint32_t index
                   , const char* id
                   , const char* sha
                   , uint32_t res_X, uint32_t res_Y
                   , const uint32_t sizes[]
                   , const uint64_t offsets[]
                   )
{
    ck_assert_int_lt(index, imgst->header.max_files);

    strncpy(imgst->metadata[index].img_id, id , MAX_IMGST_NAME);
    strncpy((char*) imgst->metadata[index].SHA   , sha, SHA256_DIGEST_LENGTH); // not null-terminated
    imgst->metadata[index].res_orig[0] = res_X;
    imgst->metadata[index].res_orig[1] = res_Y;
    memcpy(imgst->metadata[index].size  , sizes  , NB_RES * sizeof(*sizes  ));
    memcpy(imgst->metadata[index].offset, offsets, NB_RES * sizeof(*offsets));
    imgst->metadata[index].is_valid = NON_EMPTY;
}

// ======================================================================
START_TEST(same_id)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(imgst);

    const uint32_t sizes[]   = {  123,  4567, 89012 };
    const uint64_t offsets[] = { 9475, 13246, 11229 };

    const char* const monid = "some id";
    const uint32_t index1 = 5;
    insert(&imgst, index1,
           monid, "7fb206ca4d40af4f9a9da4ab170982fcdfac078f6cf642ff449ae0a65a59a502",
           1024, 768, // res
           sizes, offsets);

    const uint32_t index2 = 8;
    insert(&imgst, index2,
           monid, "b7f02c6ad44a0ff49ad94aab719082cffdac70f86c6f24ff44a9ea056a5a9025",
           1024, 768, // res
           sizes, offsets);

    int err = do_name_and_content_dedup(&imgst, index1);
    ck_assert_int_eq(err, ERR_DUPLICATE_ID);

    err = do_name_and_content_dedup(&imgst, index2);
    ck_assert_int_eq(err, ERR_DUPLICATE_ID);

    release_imgst(&imgst);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
#define dedup_check(X) \
    ck_assert_msg(imgst.metadata[index3].X == imgst.metadata[index2].X || \
                  imgst.metadata[index3].X == imgst.metadata[index4].X, \
                  "did not dedup " #X " (or didn't take the first image as reference)")
// ------------------------------------------------------------
#define check_unchanged_field(I, F, RES, REF) \
    ck_assert_msg(imgst.metadata[I].F[RES] == REF[RES], \
                  "dedup did change some metadata (" #F "[" #RES "])")
// ------------------------------------------------------------
#define check_unchanged(I, S, O) \
    do { \
        check_unchanged_field(I, size,   RES_THUMB, S); \
        check_unchanged_field(I, size,   RES_SMALL, S); \
        check_unchanged_field(I, size,   RES_ORIG , S); \
        check_unchanged_field(I, offset, RES_THUMB, O); \
        check_unchanged_field(I, offset, RES_SMALL, O); \
        check_unchanged_field(I, offset, RES_ORIG , O); \
    } while(0)
// ------------------------------------------------------------
START_TEST(same_content)
{
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(imgst);

    const uint32_t sizes[]    = {  123,  4567, 89012 };
    const uint64_t offsets[]  = { 9475, 13246, 11229 };
    const uint64_t offsets2[] = { 5555,  6666,  7777 };

    const char* const sha = "7fb206ca4d40af4f9a9da4ab170982fcdfac078f6cf642ff449ae0a65a59a502";

    const uint32_t index1 = 2;
    insert(&imgst, index1,
           "monid1", sha,
           1024, 768, // res
           sizes, offsets2);
    imgst.metadata[index1].is_valid  = EMPTY; // this one is not a valid copy

    // that one IS a valid copy, could be the reference
    const uint32_t index2 = 3;
    insert(&imgst, index2,
           "monid2", sha,
           1024, 768, // res
           sizes, offsets);

    // this is the one to be deduplicated
    const uint32_t sizes3[]   = {  0,  0, sizes[2] };
    const uint64_t offsets3[] = {  0,  0, 12345    };
    const uint32_t index3 = 5;
    insert(&imgst, index3,
           "monid3", sha,
           1024, 768, // res
           sizes3, offsets3);

    // that one is another valid copy, could also be used as reference
    const uint32_t index4 = 7;
    insert(&imgst, index4,
           "monid4", sha,
           1024, 768, // res
           sizes, offsets2);

    // dedup returns sucessfully 
    ck_assert_err_none(do_name_and_content_dedup(&imgst, index3));

    // dedup was done
    dedup_check(offset[RES_THUMB]);
    dedup_check(offset[RES_SMALL]);
    dedup_check(offset[RES_ORIG]);
    dedup_check(size[RES_THUMB]);
    dedup_check(size[RES_SMALL]);

    // dedup didn't change any other metadata
    check_unchanged(index1, sizes, offsets2);
    check_unchanged(index2, sizes, offsets );
    check_unchanged(index4, sizes, offsets2);
    
    // garbage collector
    release_imgst(&imgst);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
START_TEST(different_content)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    init_imgst(imgst);

    const uint32_t sizes[]    = {  123,  4567, 89012 };
    const uint64_t offsets[]  = { 9475, 13246, 11229 };
    const uint64_t offsets2[] = { 5555,  6666,  7777 };

    const uint32_t index1 = 2;
    insert(&imgst, 2,
           "monid1", "7fb206ca4d40af4f9a9da4ab170982fcdfac078f6cf642ff449ae0a65a59a502",
           1024, 768, // res
           sizes, offsets);

    const uint32_t index2 = 5;
    insert(&imgst, index2,
           "monid2", "b7f02c6ad44a0ff49ad94aab719082cffdac70f86c6f24ff44a9ea056a5a9025",
           1024, 768, // res
           sizes, offsets2);

    // dedup returns sucessfully 
    ck_assert_err_none(do_name_and_content_dedup(&imgst, index2));

    // dedup set offset[RES_ORIG] to 0
    ck_assert_msg(imgst.metadata[index2].offset[RES_ORIG] == 0, 
                  " dedup did not set offset[RES_ORIG] to 0");

    // dedup didn't change other metadata
    check_unchanged(index1, sizes, offsets);

    check_unchanged_field(index2, size,   RES_THUMB, sizes);
    check_unchanged_field(index2, size,   RES_SMALL, sizes);
    check_unchanged_field(index2, size,   RES_ORIG , sizes);
    check_unchanged_field(index2, offset, RES_THUMB, offsets2);
    check_unchanged_field(index2, offset, RES_SMALL, offsets2);
    
    // garbage collector
    release_imgst(&imgst);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
START_TEST(error_cases)
{
// ------------------------------------------------------------
#ifdef WITH_PRINT
    printf("=== %s:\n", __func__);
#endif
    ck_assert_invalid_arg(do_name_and_content_dedup(NULL, 1));
                      
    init_imgst(imgst);
    ck_assert_invalid_arg(do_name_and_content_dedup(&imgst, MAX_FILES));
    release_imgst(&imgst);

#ifdef WITH_PRINT
    printf("=== END of %s\n", __func__);
#endif
}
END_TEST

// ======================================================================
Suite* dedup_test_suite()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
    srand(time(NULL) ^ getpid()
#ifdef NEED_THREAD_RANDOM
          ^ pthread_self()
#endif
          );
#pragma GCC diagnostic pop

    Suite* s = suite_create("Tests of dedup");

    Add_Case(s, tc1, "dedup tests");
    tcase_add_test(tc1, same_id);
    tcase_add_test(tc1, same_content);
    tcase_add_test(tc1, different_content);
    tcase_add_test(tc1, error_cases);

    return s;
}

TEST_SUITE(dedup_test_suite)

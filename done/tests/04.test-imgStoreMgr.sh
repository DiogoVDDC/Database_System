#!/bin/bash

## Black-box testing of imgStoreMgr

source $(dirname ${BASH_SOURCE[0]})/test_env.sh

test=0
ok=1

# ======================================================================
# tool function
check_output() {
    exec=imgStoreMgr
    cmd=list
    checkX "command line ImgStore tool (namely $exec exec)" $exec

    EXPECTED_OUTPUT="${2}"

    mytmp="$(new_tmp_file)"
    # gets stdout in case of success, stderr in case of error
    ACTUAL_OUTPUT="$("$exec" "$cmd" tests/data/"$1" 2>"$mytmp"|| cat "$mytmp")"

    diff -w <(echo "$ACTUAL_OUTPUT") <(echo -e "$EXPECTED_OUTPUT") \
        && (echo "PASS"; return 0) \
        || (echo "FAIL"; \
#            echo -e "Expected:\n$EXPECTED_OUTPUT"; \
#            echo -e "Actual:\n$ACTUAL_OUTPUT"; \
            return 1)
}

# ==== FEEDBACK
# ======================================================================
db=test01.imgst_static
printf "Test %1d (with $db): " $((++test))
check_output $db \
"*****************************************
**********IMGSTORE HEADER START**********
TYPE:              EPFL ImgStore binary
VERSION: 0
IMAGE COUNT: 0          MAX IMAGES: 10
THUMBNAIL: 64 x 64      SMALL: 256 x 256
***********IMGSTORE HEADER END***********
*****************************************
<< empty imgStore >>" \
|| ok=0

# ======================================================================
db=test02.imgst_static
printf "Test %1d (with $db): " $((++test))
check_output $db \
"*****************************************
**********IMGSTORE HEADER START**********
TYPE:              EPFL ImgStore binary
VERSION: 2
IMAGE COUNT: 2          MAX IMAGES: 10
THUMBNAIL: 64 x 64      SMALL: 256 x 256
***********IMGSTORE HEADER END***********
*****************************************
IMAGE ID: pic1
SHA: 66ac648b32a8268ed0b350b184cfa04c00c6236af3a2aa4411c01518f6061af8
VALID: 1
UNUSED: 0
OFFSET ORIG. : 2224             SIZE ORIG. : 72876
OFFSET THUMB.: 0                SIZE THUMB.: 0
OFFSET SMALL : 0                SIZE SMALL : 0
ORIGINAL: 1200 x 800
*****************************************
IMAGE ID: pic2
SHA: 1183f8ef10dcb4d87a1857bd16f9b5f8728a8d1ea6c9c7eb37ddfa1da01bff52
VALID: 1
UNUSED: 0
OFFSET ORIG. : 75100            SIZE ORIG. : 369911
OFFSET THUMB.: 0                SIZE THUMB.: 0
OFFSET SMALL : 0                SIZE SMALL : 0
ORIGINAL: 1200 x 800
*****************************************" \
|| ok=0

# ======================================================================
[ "x$ok" = 'x1' ] && echo "$0 SUCCESS" || echo "$0 FAILED at some point" 

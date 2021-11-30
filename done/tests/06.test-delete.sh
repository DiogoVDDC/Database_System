#!/bin/bash

## Black-box testing of imgStoreMgr -- delete command

source $(dirname ${BASH_SOURCE[0]})/test_env.sh
source $(dirname ${BASH_SOURCE[0]})/helptext.sh

test=0
ok=1

expected_output_1="*****************************************
**********IMGSTORE HEADER START**********
TYPE:            EPFL ImgStore binary
VERSION: 3
IMAGE COUNT: 1          MAX IMAGES: 10
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
*****************************************"

expected_output_2="*****************************************
**********IMGSTORE HEADER START**********
TYPE:            EPFL ImgStore binary
VERSION: 4
IMAGE COUNT: 0          MAX IMAGES: 10
THUMBNAIL: 64 x 64      SMALL: 256 x 256
***********IMGSTORE HEADER END***********
*****************************************
<< empty imgStore >>"

expected_size=445011

db="$(new_tmp_file)"

# ======================================================================
# tool functions
# ----------------------------------------------------------------------
safecp() {
    cp "$1" $db || error "Cannot copy \"$1\" to \"$db\""
}

# ----------------------------------------------------------------------
check_output() {
    exec=imgStoreMgr
    checkX "command line ImgStore tool (namely $exec exec)" $exec

    EXPECTED_OUTPUT="$1"; shift
    EXPECTED_ERROR="$1"; shift

    mytmp="$(new_tmp_file)"
    if [ -z "$EXPECTED_ERROR" ]; then
        # gets stdout in case of success, stderr in case of error
        ACTUAL_OUTPUT="$("$exec" "$@" 2>"$mytmp" || cat "$mytmp")"
    else
        # gets stdout, puts stderr in temp file
        ACTUAL_OUTPUT="$("$exec" "$@" 2>"$mytmp")"
    fi

    if ! diff -w <(echo "$ACTUAL_OUTPUT") <(echo -e "$EXPECTED_OUTPUT"); then
        echo "FAIL"
        echo -e "Expected output:\n$EXPECTED_OUTPUT"
        echo -e "Actual:\n$ACTUAL_OUTPUT"
        return 1
    fi
    if ! [ -z "$EXPECTED_ERROR" ]; then
        diff -w "$mytmp" <(echo -e "$EXPECTED_ERROR") \
            && (echo "PASS"; return 0) \
            || (echo "FAIL"; \
              echo -e "Expected error:\n$EXPECTED_ERROR"; \
              echo 'Actual:'; cat "$mytmp"; \
                return 1)
    else
        echo 'PASS'
    fi
    return 0
}

# ----------------------------------------------------------------------
check_delete_one_image() {
    printf "\t$1.1. delete $2: "
    check_output "" "" delete "$db" $2 || return 1

    printf "\t$1.2. list: "
    check_output "$3" "" list "$db" || return 1
    return 0
}

# ----------------------------------------------------------------------
do_test_3 () {
    testdb=tests/data/test03.imgst_static
    ref=tests/data/"$1"; shift
    printf "Test %1d (delete $* in (copy of) $testdb):\n" $((++test))
    safecp "$testdb"
    my_ok=1
    nb=1
    for id in "$@"; do
        printf "\t${nb}. delete $id: "
        check_output "" "" delete "$db" $id || my_ok=0
        nb=$(( $nb + 1 ))
    done
    
    printf "\t${nb}. list: "
    check_output "$(imgStoreMgr list "$ref")" "" list "$db" || my_ok=0
    nb=$(( $nb + 1 ))
    
    printf "\t${nb}. size: "
    if [ $(stat -c%s $db) -eq $(stat -c%s "$ref") ]; then
        echo 'PASS'
    else
        echo "Wrong size: got ${actual_size}, where I was expecting $expected_size"
        my_ok=0
    fi

    [ "x$my_ok" = 'x1' ] && echo '==> PASS' || { echo '==> FAIL'; ok=0; }
}

# ==== FEEDBACK
# ======================================================================
# make a working copy
safecp tests/data/test02.imgst_static

# ======================================================================
printf "Test %1d (delete with bad image id): " $((++test))
check_output "$helptxt" "ERROR: File not found" delete $db foo \
|| ok=0

# ======================================================================
my_ok=1
printf "Test %1d (delete command):\n" $((++test))
check_delete_one_image a pic2 "$expected_output_1" || my_ok=0
check_delete_one_image b pic1 "$expected_output_2" || my_ok=0

printf '\tc. size: '
actual_size=$(stat -c%s $db)
if [ $actual_size -eq $expected_size ]; then
    echo 'PASS'
else
    echo "Wrong size: got ${actual_size}, where I was expecting $expected_size"
    my_ok=0
fi

[ "x$my_ok" = 'x1' ] && echo '==> PASS' || { echo '==> FAIL'; ok=0; }

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo "$0 SUCCESS"
    exit 0
else
   echo "$0 FAILED at some point"
   exit 1
fi

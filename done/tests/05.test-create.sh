#!/bin/bash

## Black-box testing of imgStoreMgr -- create command

source $(dirname ${BASH_SOURCE[0]})/test_env.sh
source $(dirname ${BASH_SOURCE[0]})/helptext.sh

test=0
ok=1


empty_db="*****************************************
**********IMGSTORE HEADER START**********
TYPE:            EPFL ImgStore binary
VERSION: 0
IMAGE COUNT: 0          MAX IMAGES: 10
THUMBNAIL: 64 x 64      SMALL: 256 x 256
***********IMGSTORE HEADER END***********
*****************************************"

# ======================================================================
# tool function
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

# ==== This one is whole FEEDBACK
# ======================================================================
db="$(new_tmp_file)"

doutput="Create
11 item(s) written"

printf "Test %1d (create command):\n" $((++test))
printf '\ta. '
check_output "$doutput 
$empty_db" \
"" \
create "$db" \
|| ok=0

printf '\tb. '
check_output \
"$empty_db
<< empty imgStore >>" \
"" \
list "$db" \
|| ok=0

printf '\tc. '
if cmp "$db" tests/data/test01.imgst_static; then
    echo 'PASS'
else
    echo 'cmp with empty ImgStore FAILED'
    ok=0
fi

[ "x$ok" = 'x1' ] && echo '==> PASS' || echo '==> FAIL'

# ======================================================================
printf "Test %1d (create without further argument): " $((++test))
check_output "$helptxt" "ERROR: Not enough arguments" create \
|| ok=0

# ======================================================================
printf "Test %1d (create with unauthorized file): " $((++test))
check_output "Create
$helptxt" "ERROR: I/O Error" create /root/tmp$$.pictdb \
|| ok=0

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo "$0 SUCCESS"
    exit 0
else
   echo "$0 FAILED at some point"
   exit 1
fi

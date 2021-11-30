#!/bin/bash

## Black-box testing of imgStoreMgr -- create command, now with arguments

source $(dirname ${BASH_SOURCE[0]})/test_env.sh
source $(dirname ${BASH_SOURCE[0]})/helptext.sh

test=0
ok=1

output_template="Create
11 item(s) written
*****************************************
**********IMGSTORE HEADER START**********
TYPE:            EPFL ImgStore binary
VERSION: 0
IMAGE COUNT: 0          MAX IMAGES: 10
THUMBNAIL: 64 x 64      SMALL: 256 x 256
***********IMGSTORE HEADER END***********
*****************************************"

db="$(new_tmp_file)"

# error messages
nea='Not enough arguments'
imfn='Invalid max_files number'
ires='Invalid resolution(s)'
iarg='Invalid argument'

# ======================================================================
# tool functions
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
            && { echo "PASS"; return 0; } \
            || { echo "FAIL"; \
                 echo -e "Expected error:\n$EXPECTED_ERROR"; \
                 echo 'Actual:'; cat "$mytmp"; \
                 return 1; }
    else
        echo 'PASS'
    fi
    return 0
}

# ----------------------------------------------------------------------
error_test () {
    info="$1"; shift
    error_msg="ERROR: $1"; shift
    printf "Test %1d ($info): " $((++test))
    check_output "$helptxt" "$error_msg" create $db "$@"
}

# ----------------------------------------------------------------------
standard_test () {
    info="$1"; shift
    expected_output="$1"; shift
    expected_size="$1"; shift
#    error_msg="ERROR: $1"; shift

    printf "Test %1d ($info):\n" $((++test))
    printf "\ta. : "
    check_output "$expected_output" "" create $db "$@" || return 1
    printf '\tb. size: '
    actual_size=$($stat -c%s $db)
    if [ $actual_size -eq $expected_size ]; then
        echo 'PASS'
    else
        echo "Wrong size: got ${actual_size}, where I was expecting $expected_size"
        return 1
    fi

    echo '==> PASS'
    return 0
}

# ----------------------------------------------------------------------
ch_res () {
    echo "$output_template" | sed "8{s/: 64 /: $1 /;s/x 64 /x $2 /;s/: 256 /: $3 /;s/x 256\$/x $4/}"
}

# ----------------------------------------------------------------------
resolution_test () {
    case $1 in
        -thumb_res) rezs="$2 $3 256 256" ;;
        -small_res) rezs="64 64 $2 $3"   ;;
                 *) rezs='64 64 256 256' ;;
    esac
    standard_test "$*"  "$(ch_res $rezs)" 2224 "$@"
}

# ---- 1. some error cases
echo "I. Error cases:"

# ==== FEEDBACK
# ======================================================================
error_test '-max_files without value'       "$nea"  -max_files         || ok=0
error_test '-max_files with too big value'  "$imfn" -max_files 2000000 || ok=0
error_test '-thumb_res missing a value (1)' "$nea"  -thumb_res 100     || ok=0
error_test '-thumb_res missing a value (2)' "$ires" -thumb_res 100 -max_files 1000 || ok=0
error_test 'wrong option'                   "$iarg" -maxfiles          || ok=0

# ---- 2. standard cases
printf '\nII. Standard cases:\n'

# ======================================================================
# -max_files
# ==== FEEDBACK
testlist=12

for mf in $testlist; do
    mfp1=$(($mf + 1))
    esize=$((64 + 216 * $mf))
    standard_test "-max_files $mf" "$(echo "$output_template" | sed "2s/^11 /$mfp1 /;7s/ 10\$/ $mf/")" $esize -max_files $mf || ok=0
done

# ======================================================================
# resolutions
res=125
for opt in -thumb_res -small_res; do
# ==== FEEDBACK
    resolution_test $opt $res $res || ok=0
done

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo "$0 SUCCESS"
    exit 0
else
   echo "$0 FAILED at some point"
   exit 1
fi

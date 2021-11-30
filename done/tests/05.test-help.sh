#!/bin/bash

## Black-box testing of imgStoreMgr -- help command

source $(dirname ${BASH_SOURCE[0]})/test_env.sh
source $(dirname ${BASH_SOURCE[0]})/helptext.sh

test=0
ok=1


# ======================================================================
# tool function
check_output() {
    exec=imgStoreMgr
    checkX "command line ImgStore tool (namely $exec exec)" $exec

    EXPECTED_OUTPUT="$helptxt"
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
        echo "PASS"
    fi
    return 0
}

# ==== This one is whole FEEDBACK
# ======================================================================
printf "Test %1d (help command): " $((++test))
check_output "" help \
|| ok=0

# ======================================================================
printf "Test %1d (help command with useless extra argument [no error]): " $((++test))
check_output "" help foo \
|| ok=0

# ======================================================================
printf "Test %1d (without any command): " $((++test))
check_output "ERROR: Not enough arguments" \
|| ok=0

# ======================================================================
printf "Test %1d (with bad command): " $((++test))
check_output "ERROR: Invalid command" "bad" \
|| ok=0

# ======================================================================
printf "Test %1d (with empty command): " $((++test))
check_output "ERROR: Invalid command" "" \
|| ok=0

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo "$0 SUCCESS"
    exit 0
else
   echo "$0 FAILED at some point"
   exit 1
fi

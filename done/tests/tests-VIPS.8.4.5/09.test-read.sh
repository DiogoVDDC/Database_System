#!/bin/bash

## Black-box testing of imgStoreMgr -- read command

# ======================================================================
# options

with_colors=0
while [ $# -ge 1 ]; do
    case "$1" in
        -c|--color|--colors) with_colors=1 ;;
        *) break ;;
    esac
    shift
done

if [ "x$with_colors" = 'x1' ]; then
    esc="\033["
    red="${esc}31m"
    green="${esc}32m"
    yellow="${esc}33m"
    blue="${esc}34m"
    magenta="${esc}35m"
    cyan="${esc}36m"
    bold="${esc}1m"
    end="${esc}0m"
else
    red=
    green=
    yellow=
    blue=
    magenta=
    cyan=
    bold=
    end=
fi

# ======================================================================
printf "${bold}${blue}"
source $(dirname ${BASH_SOURCE[0]})/test_env.sh
printf "${end}"
source $(dirname ${BASH_SOURCE[0]})/helptext.sh

test=0
ok=1

# error messages
nea='Not enough arguments'
ires='Invalid resolution(s)'
iarg='Invalid argument'
fnf='File not found'
ioerr='I/O Error'

db="$(new_tmp_file)"

# ======================================================================
# tool functions
# ----------------------------------------------------------------------
safecp() {
    local file="tests/data/$1"
    cp "$file" $db || error "Cannot copy \"$file\" to \"$db\""
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
        echo -e "${red}FAIL${end}"
        echo -e "${yellow}Expected:${end}\n$EXPECTED_OUTPUT"
        echo -e "${cyan}Actual:${end}\n$ACTUAL_OUTPUT"
        return 1
    fi
    if ! [ -z "$EXPECTED_ERROR" ]; then
        if diff -w "$mytmp" <(echo -e "$EXPECTED_ERROR"); then
            echo -e "${green}PASS${end}"
            return 0
        else
            echo -e "${red}FAIL${end}"
            echo -e "${yellow}Expected error:${end}\n$EXPECTED_ERROR";
            echo -e "Actual:${end}"
            cat "$mytmp"
            return 1
        fi
    else
        echo -e "${green}PASS${end}"
    fi
    return 0
}

# ----------------------------------------------------------------------
error_test () {
    info="$1"; shift
    error_msg="ERROR: $1"; shift
    printf "${magenta}Test %1d${end} ($info): " $((++test))
    check_output "$helptxt" "$error_msg" read $db "$@"
}

# ----------------------------------------------------------------------
standard_test () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} ($info):\n" $((++test))
    local reffile="tests/data/$3"
    [ -f "$reffile" ] || error "Cannot launch test, reference file $reffile not present"

    local db_size=0
    if [ $# -ge 4 ]; then
        db_size="$4"
        local actual_size=$($stat -c%s $db)
        [ $actual_size -eq $db_size ] || error "WEIRD pre-test condition: $db does not have expected size ${db_size}, but has size $actual_size"
    else
        db_size=$($stat -c%s $db)
    fi
    
    local db_size_after="$db_size"
    [ $# -ge 5 ] && db_size_after="$5"
    
    printf "\ta. reading: "
    local res=orig
    if [ "x$2" = 'x' ]; then
        # testing default resolution
        check_output '' '' read "$db" "$1" || return 1
    else
        res="$2"
        [ "$res" = 'original'  ] && res=orig
        [ "$res" = 'thumbnail' ] && res=thumb
        check_output '' '' read "$db" "$1" "$2" || return 1
    fi
    local file="${1}_${res}.jpg"
    if ! [ -f "$file" ]; then
        echo -e "${red}FAIL${end}: image $file not created"
        return 1
    fi

    printf '\tb. check content: '
    local actual_size=$($stat -c%s "$file")
    local expected_size=$($stat -c%s "$reffile")
    if [ $actual_size -eq $expected_size ]; then
        if cmp -s "$file" "$reffile"; then
            rm "$file"
            echo -e "${green}PASS${end}"
        else
            rm "$file"
            echo -e "${red}FAIL${end}: content of image $file is not what I was expecting"
            return 1
        fi
    else
        rm "$file"
        echo -e "${red}FAIL${end}: wrong image size ($file): got ${actual_size}, where I was expecting $expected_size"
        return 1
    fi

    printf '\tc. ImgStore size: '
    actual_size=$($stat -c%s $db)
    if [ $actual_size -eq $db_size_after ]; then
        echo -e "${green}PASS${end}"
    else
        echo -e "${red}FAIL${end}: wrong ImgStore size: is ${actual_size}, where it shall be $db_size_after"
        return 1
    fi

    echo -e "==> ${green}PASS${end}"
    return 0
}

# ---- 1. some error cases
echo
echo 'TEST POUR VIPS 8.4.5 !'
echo
echo -e "${yellow}I. Error cases:${end}"

# ==== FEEDBACK
# ======================================================================
# make a working copy
safecp test02.imgst_dynamic

# ======================================================================
error_test 'missing argument' "$nea"              || ok=0
error_test 'wrong id'         "$fnf"  pic24 small || ok=0
error_test 'wrong resolution' "$ires" pic2  foo   || ok=0

# ---- 2. standard cases
printf "\n${yellow}II. Standard cases:${end}\n"

# ======================================================================
# make a working copy
safecp test02.imgst_dynamic

# ==== FEEDBACK
standard_test 'default resolution' pic1 '' papillon.jpg || ok=0
standard_test 'pic1 orig' pic1 orig papillon.jpg    || ok=0
standard_test 'pic2 orig' pic2 orig coquelicots.jpg || ok=0

# read with resized creation
standard_test 'thumb first time' pic1 thumb papillon_thumb.jpg 192659 204801 || ok=0

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo -e "$0 ${bold}${green}SUCCESS${end}"
    exit 0
else
   echo -e "$0 ${bold}${red}FAILED${end} at some point"
   exit 1
fi

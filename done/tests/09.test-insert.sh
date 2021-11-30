#!/bin/bash

## Black-box testing of imgStoreMgr -- insert command

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
iiid='Invalid image ID'
exiid='Existing image ID'
fnf='File not found'
ioerr='I/O Error'

sha1=66ac648b32a8268ed0b350b184cfa04c00c6236af3a2aa4411c01518f6061af8
size1=72876

sha2=95962b09e0fc9716ee4c2a1cf173f9147758235360d7ac0a73dfa378858b8a10
size2=98119

sha3=1183f8ef10dcb4d87a1857bd16f9b5f8728a8d1ea6c9c7eb37ddfa1da01bff52
size3=369911

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
# params: [version, [count, [max images, [thumb, small]]]]
header() {
    local ver=0
    local count=0
    local maxim=10
    local thumb=64
    local small=256
    [ $# -ge 1 ] && ver=$1   && shift
    [ $# -ge 1 ] && count=$1 && shift
    [ $# -ge 1 ] && maxim=$1 && shift
    [ $# -ge 1 ] && thumb=$1 && shift
    [ $# -ge 1 ] && small=$1 && shift
    
    echo "*****************************************
**********IMGSTORE HEADER START**********
TYPE:            EPFL ImgStore binary
VERSION: $ver
IMAGE COUNT: $count          MAX IMAGES: $maxim
THUMBNAIL: $thumb x $thumb      SMALL: $small x $small
***********IMGSTORE HEADER END***********
*****************************************"
}

# ----------------------------------------------------------------------
# params: imgId, SHA, size, offset[, resolution]
image_txt() {
    local res='1200 x 800'
    [ $# -ge 5 ] && res="$5"
    echo "IMAGE ID: $1
SHA: $2
VALID: 1
UNUSED: 0
OFFSET ORIG. : $4		SIZE ORIG. : $3
OFFSET THUMB.: 0		SIZE THUMB.: 0
OFFSET SMALL : 0		SIZE SMALL : 0
ORIGINAL: $res
*****************************************"
}

# ----------------------------------------------------------------------
error_test () {
    info="$1"; shift
    error_msg="ERROR: $1"; shift
    printf "${magenta}Test %1d${end} ($info): " $((++test))
    check_output "$helptxt" "$error_msg" insert $db "$@"
}

# ----------------------------------------------------------------------
standard_test () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} ($info):\n" $((++test))
    local insfile="tests/data/$2"
    [ -f "$insfile" ] || error "Cannot launch test, reference file $insfile not present"

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
    
    printf "\ta. inserting: "
    check_output '' '' insert "$db" "$1" "$insfile" || return 1

    printf '\tb. list: '
    check_output "$3" '' list "$db" || return 1

    printf '\tc. ImgStore size: '
    actual_size=$($stat -c%s $db)
    if [ $actual_size -eq $db_size_after ]; then
        echo -e "${green}PASS${end}"
    else
        echo "Wrong ImgStore size: is ${actual_size}, where it shall be $db_size_after"
        return 1
    fi

    echo -e "==> ${green}PASS${end}"
    return 0
}

# ---- 1. some error cases
echo -e "${yellow}I. Error cases:${end}"

# ==== FEEDBACK
# ======================================================================
# make a working copy
safecp test02.imgst_dynamic

# ======================================================================
error_test 'missing argument'  "   $nea"    || ok=0
error_test 'missing argument (2)' "$nea" id || ok=0
error_test 'inexisting file' "$ioerr" some_id "$(mktemp -u)" || ok=0
error_test 'existing id but other content' "$exiid" pic1 tests/data/coquelicots.jpg || ok=0
error_test 'too long image id' "$iiid" \
129aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \
tests/data/papillon.jpg || ok=0

# ---- 2. standard cases
printf "\n${yellow}II. Standard cases:${end}\n"

# ======================================================================
# make a fresh working copy
safecp test02.imgst_dynamic

# ==== FEEDBACK
offset1=21664
offset3=192659
standard_test 'basic insert' pic3 foret.jpg "$(header 3 3 100)
$(image_txt pic1 $sha1 $size1 $offset1)
$(image_txt pic2 $sha2 $size2  94540)
$(image_txt pic3 $sha3 $size3 $offset3)" $offset3 $(($offset3 + $size3)) || ok=0
# -----
standard_test 'insert duplicate content; case 1: of an existing image' \
pic4 papillon.jpg "$(header 4 4 100)
$(image_txt pic1 $sha1 $size1 $offset1)
$(image_txt pic2 $sha2 $size2  94540)
$(image_txt pic3 $sha3 $size3 $offset3)
$(image_txt pic4 $sha1 $size1 $offset1)" || ok=0
# -----
standard_test 'insert duplicate content; case 2: of a newly inserted image' \
pic5 foret.jpg "$(header 5 5 100)
$(image_txt pic1 $sha1 $size1 $offset1)
$(image_txt pic2 $sha2 $size2  94540)
$(image_txt pic3 $sha3 $size3 $offset3)
$(image_txt pic4 $sha1 $size1 $offset1)
$(image_txt pic5 $sha3 $size3 $offset3)" || ok=0

# Still FEEDBACK
# undelete a picture with duplicate (delete then insert the same id + content)
printf "${magenta}Test %1d${end} (delete pic1): " $((++test))
check_output '' '' delete "$db" pic1 || ok=0
output_txt="$(header 7 5 100)
$(image_txt pic1 $sha1 $size1 $offset1)
$(image_txt pic2 $sha2 $size2  94540)
$(image_txt pic3 $sha3 $size3 $offset3)
$(image_txt pic4 $sha1 $size1 $offset1)
$(image_txt pic5 $sha3 $size3 $offset3)"
standard_test 'undelete of duplicate' \
pic1 papillon.jpg "$output_txt" || ok=0

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo -e "$0 ${bold}${green}SUCCESS${end}"
    exit 0
else
   echo -e "$0 ${bold}${red}FAILED${end} at some point"
   exit 1
fi

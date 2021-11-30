#!/bin/bash

## Black-box testing of imgStoreMgr -- garbage collector

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
offset1=21664
size1t=12144
size1s=16311
thumbs1="$size1t 94540 $size1s 106666"

sha2=95962b09e0fc9716ee4c2a1cf173f9147758235360d7ac0a73dfa378858b8a10
size2=98119
offset2=94540
offset2_bis=122965
size2t=12309
size2s=17349
thumbs2a="$size2t 221084 $size2s 233393"
thumbs2b="$size2t 119783 $size2s 132092"
thumbs2c="$size2t 590995 $size2s 603304"

sha3=1183f8ef10dcb4d87a1857bd16f9b5f8728a8d1ea6c9c7eb37ddfa1da01bff52
size3=369911
size3s=18460
offset3=192659

db="$(new_tmp_file)"
dbbkup="$(new_tmp_file)"

# ======================================================================
# tool functions
# ----------------------------------------------------------------------
safecp() {
    local file="tests/data/$1"
    cp "$file" $db || error "Cannot copy \"$file\" to \"$db\""
    rm -f $dbbkup
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
    echo "IMAGE ID: $1
SHA: $2
VALID: 1
UNUSED: 0
OFFSET ORIG. : $4		SIZE ORIG. : $3
OFFSET THUMB.: ${6:-0}		SIZE THUMB.: ${5:-0}
OFFSET SMALL : ${8:-0}		SIZE SMALL : ${7:-0}
ORIGINAL: ${9:-1200 x 800}
*****************************************"
}

line1a="$(image_txt pic1 $sha1 $size1 $offset1 0 0 $size1s 574714)"
line2a="$(image_txt pic2 $sha2 $size2 $offset2 0 0 $size2s 591025)"
line3a="$(image_txt pic3 $sha3 $size3 $offset3 $size3s 608374)"
line4a="$(image_txt pic4 $sha1 $size1 $offset1 $size1t 562570)"

line1c="$(image_txt pic1 $sha1 $size1 $offset1)"
line2c="$(image_txt pic2 $sha2 $size2 $offset2)"
line3c="$(image_txt pic3 $sha3 $size3 $offset3)"

# ----------------------------------------------------------------------
error_test () {
    info="$1"; shift
    error_msg="ERROR: $1"; shift
    printf "${magenta}Test %1d${end} ($info): " $((++test))
    check_output "$helptxt" "$error_msg" gc "$@"
}

# ----------------------------------------------------------------------
# tests a ImgStoreMgr command
#
# $1: info message
# $2: expected stdout
# $3: expected prior DB size, or 0 if we don't want to test
# $4: if $3 != 0, expected posterior DB size, or '=' if it as to be the same
# $4|5: expected text for the listing, not tested if empty string
# others: command to be tested and all its arguments

standard_test () {
    local info="$1"; shift
    printf "${magenta}Test %1d${end} ($info):\n" $((++test))

    local e_msg="$1"; shift

    local db_size="$1"; shift
    if [ $db_size -ne 0 ]; then
        local actual_size=$($stat -c%s $db)
        [ $actual_size -eq $db_size ] || error "WEIRD pre-test condition: $db does not have expected size ${db_size}, but has size $actual_size"
    
        local db_size_after="$1"; shift
        [ "$db_size_after" = '=' ] && db_size_after=$db_size
    fi

    local list_output="$1"; shift
    
    printf "\ta. doing $1: "
    check_output "$e_msg" '' "$@" || return 1

    local next=b
    if [ "x$list_output" != 'x' ]; then
        printf '\tb. list: '
        check_output "$list_output" '' list "$db" || return 1
        next=c
    fi

    if [ $db_size -ne 0 ]; then
        printf '\t%c. ImgStore size: ' $next
        actual_size=$($stat -c%s $db)
        if [ $actual_size -eq $db_size_after ]; then
            echo -e "${green}PASS${end}"
        else
            echo "Wrong ImgStore size: is ${actual_size}, where it shall be $db_size_after"
            return 1
        fi
    fi

    # garbage collecting
    if [ "x$1" = 'xread' ]; then
        # delete created image
        rm -f $3_$4.jpg
    fi

    echo -e "==> ${green}PASS${end}"
    return 0
}

# ----------------------------------------------------------------------
gc_test () {
    local info="$1"; shift
    local msg="$1"; shift
    local size1="$1"; shift ## needs that to change them in 2nd pass
    local size2="$1"; shift
    local output="$1"; shift
        
    # 2 passes: gc shall be idempotent
    for pass in 1 2; do
        echo "==== pass $pass ===="
        standard_test "gc on $info" "$msg" $size1 $size2 "$output" gc $db $dbbkup || return 1
        rm -f $dbbkup
        # in 2nd pass, DB shall not change
        size1=$($stat -c%s $db)
        size2='='
    done
}

# ----------------------------------------------------------------------
gc_ref_test () {
    # make a fresh working copy
    local name=test$1.imgst_dynamic
    [ $1 -eq 1 ] && name=test01.imgst_static
    safecp $name
    shift
    gc_test "$name" "$@"
}

# ---- 1. some error cases
echo -e "${yellow}I. Error cases:${end}"

# ==== FEEDBACK
# ======================================================================
# make a working copy
safecp test02.imgst_dynamic

# ======================================================================
error_test 'missing argument'  "   $nea"     || ok=0
error_test 'missing argument (2)' "$nea" $db || ok=0
error_test 'inexisting file' "$ioerr" "$(mktemp -u)" $dbbkup || ok=0

# ---- 2. standard cases
printf "\n${yellow}II. Standard cases:${end}\n"

## --------------------------------------------------
## test on reference files


gc_ref_test 01 '11 item(s) written' 2224 = "$(header 0 0)
<< empty imgStore >>" \
|| ok=0

gc_ref_test 02 '101 item(s) written' 192659 = "$(header 2 2 100)
$line1c
$line2c" \
|| ok=0

## --------------------------------------------------
## testing on a scenario
printf "\n${yellow}+ testing on a scenario:${end}\n"

# make a working copy
safecp test02.imgst_dynamic

size_after=$(($offset3 + $size3))
standard_test 'insert pic3 image' '' \
$offset3 $size_after \
"$(header 3 3 100)
$line1c
$line2c
$line3c" \
insert $db pic3 tests/data/foret.jpg || ok=0

standard_test 'insert duplicate image as pic4' '' \
$size_after = \
"$(header 4 4 100)
$line1c
$line2c
$line3c
$(image_txt pic4 $sha1 $size1 $offset1)" \
insert $db pic4 tests/data/papillon.jpg  || ok=0

size_before=$size_after
size_after=$(($size_before + $size1t))
standard_test 'read pic4 thumb' '' \
$size_before $size_after \
"$(header 4 4 100)
$line1c
$line2c
$line3c
$line4a" \
read $db pic4 thumb || ok=0

size_before=$size_after
size_after=$(($size_before + $size1s))
standard_test 'read pic1 small' '' \
$size_before $size_after \
"$(header 4 4 100)
$line1a
$line2c
$line3c
$line4a" \
read $db pic1 small || ok=0

size_before=$size_after
size_after=$(($size_before + $size2s))
standard_test 'read pic2 small' '' \
$size_before $size_after \
"$(header 4 4 100)
$line1a
$line2a
$line3c
$line4a" \
read $db pic2 small || ok=0

size_before=$size_after
size_after=$(($size_before + $size3s))
standard_test 'read pic3 thumb' '' \
$size_before $size_after \
"$(header 4 4 100)
$line1a
$line2a
$line3a
$line4a" \
read $db pic3 thumb || ok=0

standard_test 'delete pic2' '' \
$size_after = \
"$(header 5 3 100)
$line1a
$line3a
$line4a" \
delete $db pic2 || ok=0

standard_test 'delete pic1' '' \
$size_after = \
"$(header 6 2 100)
$line3a
$line4a" \
delete $db pic1 || ok=0

gc_test 'resulting imgStore' '101 item(s) written' \
$size_after 495009 \
"$(header 2 2 100)
$(image_txt pic3 $sha3 $size3 $offset1 $size3s 391575)
$(image_txt pic4 $sha1 $size1 410035   $size1t 482911)" \
|| ok=0

# ======================================================================
if [ "x$ok" = 'x1' ]; then
    echo -e "$0 ${bold}${green}SUCCESS${end}"
    exit 0
else
   echo -e "$0 ${bold}${red}FAILED${end} at some point"
   exit 1
fi

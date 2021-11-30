#!/bin/bash

## Black-box testing of imgStore webserver

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

test=0
ok=1

# Logfiles root name
LOG=server-$$

# webserver exec 
exec="${PWD}/imgStore_server"

# base URL
baseURL=http://localhost:8000

# where is libmongoose
libmongoose="${PWD}/libmongoose"

# server PID
job_pid=

# error messages
nea='Not enough arguments'
ires='Invalid resolution(s)'
iarg='Invalid argument'
iiid='Invalid image ID'
exiid='Existing image ID'
fnf='File not found'
ioerr='I/O Error'

original_size=192659

db="$(new_tmp_file)"

# ======================================================================
# tool functions

# ----------------------------------------------------------------------
quit() {
    stop_server
    error "$*"
}

# ----------------------------------------------------------------------
safecp() {
    local file="tests/data/$1"
    cp "$file" $db || quit "Cannot copy \"$file\" to \"$db\""
}

# ----------------------------------------------------------------------
check() {
    EXPECTED_OUTPUT="$1"; shift
    EXPECTED_ERROR="$1" ; shift
    ACTUAL_OUTPUT="$1"  ; shift
    mytmp="$1"          ; shift

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
check_logs() {
    check "$1" "$2" "$(cat "${LOG}.log")" "${LOG}-err.log"
}

# ----------------------------------------------------------------------
check_curl() {
    EXPECTED_OUTPUT="$1"; shift
    EXPECTED_ERROR="$1"; shift

####    echo "### curl -sS $@"
    mytmp="$(new_tmp_file)"
    if [ -z "$EXPECTED_ERROR" ]; then
        # gets stdout in case of success, stderr in case of error
        ACTUAL_OUTPUT="$(curl -sS "$@" 2>"$mytmp" || cat "$mytmp")"
    else
        # gets stdout, puts stderr in temp file
        ACTUAL_OUTPUT="$(curl -sS "$@" 2>"$mytmp")"
    fi

    check "$EXPECTED_OUTPUT" "$EXPECTED_ERROR" "$ACTUAL_OUTPUT" "$mytmp"
}

# ----------------------------------------------------------------------
# params: [version, [count, [max images, [thumb, small]]]]
header() {
    local ver=0
    local count=0
    local maxim=100
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

# --------------------------------------------------
launch_server_core()
{
    opid=$($pidof "$exec") && error "another $(basename "$exec") is already running (PID=$opid)!"
    $stdbuf -oL "$exec" "$db" 1> "${LOG}.log" 2> "${LOG}-err.log" &
    job_pid=$!
    sleep 1 #wait a bit
    echo "$(pwd)/${LOG}.log"     >> "$TMP_FILES"
    echo "$(pwd)/${LOG}-err.log" >> "$TMP_FILES"
}

# --------------------------------------------------
stop_server()
{
    if [ "x$job_pid" != 'x' ]; then
        ps -p$job_pid >/dev/null 2>&1 && kill -TERM $job_pid
        sleep 1 # wait a bit
        [ $# -ne 0 ] && [ "$1" = '-v' ] && echo "$(basename "$exec") PID $job_pid stoped."
        return 0
    fi
}

# --------------------------------------------------
launch_server()
{
    launch_server_core
    ps -p$job_pid >/dev/null 2>&1 || error "cannot lauch \"$(basename exec)\" (with image store \"$db\").
Here is stdout: 
$(cat ${LOG}.log)

and here is stderr:
$(cat ${LOG}-err.log)
"
    [ $# -ne 0 ] && [ "$1" = '-v' ] && echo "$(basename "$exec") launched with $db, PID $job_pid."
    return 0
}

# --------------------------------------------------
launch_erroneous_server()
{
    launch_server_core
    if ps -p$job_pid >/dev/null 2>&1; then
        echo "can lauch \"$(basename exec)\" (with image store \"$db\"),"
        echo "where I shall NOT!"
        echo "Here is stdout: 
$(cat ${LOG}.log)

and here is stderr:
$(cat ${LOG}-err.log)
"
        stop_server
        return 1
    fi
    return 0
}

# ----------------------------------------------------------------------
relaunch_with()
{
    printf "${magenta}Test %1d${end} (launching server): " $((++test))
    stop_server
    safecp "$1"
    launch_server
    check "$2" '' "$(tail -n +2 "${LOG}.log")" "${LOG}-err.log"
}

# ----------------------------------------------------------------------
error_test () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} ($info): " $((++test))
    launch_erroneous_server
    check_logs '' "$1"
}

# --------------------------------------------------
test_url() {
    url="${baseURL}/$1"
    printf "${magenta}Test %1d${end} (testing URL $1): " $((++test))
    check_curl "$2" '' "$url"
}

# ----------------------------------------------------------------------
test_delete () {
    printf "${magenta}Test %1d${end} (delete $1):\n" $((++test))
    printf "\ta. deleting        : "
    check_curl '' '' "${baseURL}/imgStore/delete?img_id=$1" || return 1
    printf "\tb. checking content: "
    check_curl "$2" '' "${baseURL}/imgStore/list" || return 1
    echo -e "==> ${green}PASS${end}"
}

# ----------------------------------------------------------------------
test_delete_again () {
    printf "${magenta}Test %1d${end} (delete again $1): " $((++test))
    check_curl "Error: $fnf" '' "${baseURL}/imgStore/delete?img_id=$1"
}

# ----------------------------------------------------------------------
set_sizes ()
{
    if [ $1 -ne 0 ]; then
        db_size=$1
        local actual_size=$($stat -c%s $db)
        [ $actual_size -eq $db_size ] || quit "WEIRD pre-test condition: $db does not have expected size ${db_size}, but has size $actual_size"
    else
        db_size=$($stat -c%s $db)
    fi

    [ $2 -ne 0 ] && db_size_after=$2 || db_size_after=$db_size
}

# ----------------------------------------------------------------------
check_imgstore_size() {
    printf '\tc. ImgStore size: '
    actual_size=$($stat -c%s $db)
    if [ $actual_size -eq $db_size_after ]; then
        echo -e "${green}PASS${end}"
    else
        echo -e "${red}FAIL${end}: wrong ImgStore size: is ${actual_size}, where it shall be $db_size_after"
        return 1
    fi
}

# ----------------------------------------------------------------------
test_read () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} (read $info):\n" $((++test))
    local reffile="tests/data/$3"
    [ -f "$reffile" ] || quit "Cannot launch test, reference file $reffile not present"

    set_sizes ${4:-0} ${5:-0}
    
    printf "\ta. reading      : "
    local file="$1_$2.jpg"
    rm -f "$file"
    check_curl '' '' "${baseURL}/imgStore/read?res=$2&img_id=$1" -o "$file" || return 1
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

    check_imgstore_size || return 1

    echo -e "==> ${green}PASS${end}"
}

# ----------------------------------------------------------------------
do_insert () {
    local insfile="tests/data/$2"
    [ -f "$insfile" ] || quit "Cannot launch test, reference file $insfile not present"

    local size=$($stat -c%s "$insfile")

    # insertion is done in chunks, the last one has to be empty
    printf "\t\t i- send chunk: "
    check_curl '' '' --data-binary @"$insfile" "${baseURL}/imgStore/insert?offset=0&name=$1" || return 1
    printf "\t\tii- do insert : "
    check_curl "$3" '' -d '' "${baseURL}/imgStore/insert?offset=${size}&name=$1" || return 1
    echo -e "\t-> ${green}PASS${end}"
}

# ----------------------------------------------------------------------
test_insert () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} (insert $info):\n" $((++test))

    set_sizes ${4:-0} ${5:-0}
    
    printf "\ta. inserting:\n"
    do_insert "$1" "$2" '' || return 1

    printf '\tb. list: '
    check_curl "$3" '' "${baseURL}/imgStore/list" || return 1

    check_imgstore_size || return 1

    echo -e "==> ${green}PASS${end}"
}

# ----------------------------------------------------------------------
test_insert_err () {
    info="$1"; shift
    printf "${magenta}Test %1d${end} (erroneous insert: $info):\n" $((++test))
    do_insert "$1" "$2" "Error: $3" || return 1
}

# ======================================================================
# ---- 0. test required material

checkX "webserver exec ($exec)" $exec

checked=curl
# shall be in PATH
command -v "$checked" > /dev/null || error "cannot launch command \"$checked\", which is required. Please install it"

[ -d "$libmongoose" ] || error "cannot find required \"$libmongoose\" directory"
export LD_LIBRARY_PATH="$libmongoose"
export DYLD_FALLBACK_LIBRARY_PATH="$libmongoose"

# ======================================================================
# ---- 1. some error cases
echo -e "${yellow}I. Error cases:${end}"

# --------------------------------------------------
# ==== FEEDBACK
printf "${magenta}Test %1d${end} (missing argument): " $((++test))
check '' "$nea" "$("$exec" 2> $db)" $db || ok=0

# --------------------------------------------------
# launch server on a fresh working copy
relaunch_with test02.imgst_dynamic "Starting imgStore server on http://localhost:8000
$(header 2 2)" || ok=0

## --------------------------------------------------
## read error cases

# wrong id
test_url 'imgStore/read?res=small&img_id=pic42' "Error: $fnf" || ok=0

# wrong resolution
test_url 'imgStore/read?res=foo&img_id=pic1' "Error: $ires" || ok=0

# no id
test_url 'imgStore/read?res=small' "Error: $iarg" || ok=0

# no resolution
test_url 'imgStore/read?img_id=pic1' "Error: $iarg" || ok=0

## --------------------------------------------------
## delete error cases

test_url 'imgStore/delete?img_id=pic42' "Error: $fnf" || ok=0

## --------------------------------------------------
## insert error cases

# non-POST request to insert
test_url 'imgStore/insert?name=pic3' 'Not found' || ok=0

# existing id new content
test_insert_err 'existing id new content' pic2 foret.jpg "$exiid" || ok=0

# too long image id
test_insert_err 'too long image id' \
129aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \
papillon.jpg "$iiid" || ok=0


# ======================================================================
# ---- 2. standard cases
printf "\n${yellow}II. Standard cases:${end}\n"

# --------------------------------------------------
# launch server on a fresh working copy (in case the above errors did change it)
relaunch_with test02.imgst_dynamic "Starting imgStore server on http://localhost:8000
$(header 2 2)" || ok=0

## --------------------------------------------------
## test list url
test_url imgStore/list '{ "Images": [ "pic1", "pic2" ] }' || ok=0

## --------------------------------------------------
## test of read

test_read 'first img' pic1 orig papillon.jpg    || ok=0
test_read '2nd img'   pic2 orig coquelicots.jpg || ok=0

# read with resized creation
size_before=$original_size
size_after=$(($size_before + 12126))
test_read 'thumb first time' pic1 thumb papillon_thumb.jpg $size_before $size_after || ok=0

## --------------------------------------------------
## test of delete

test_delete pic1 '{ "Images": [ "pic2" ] }' || ok=0
test_delete pic2 '{ "Images": [ ] }' || ok=0
test_delete_again pic1 || ok=0

## --------------------------------------------------
## test of insert

# restarting server with fresh copy
relaunch_with test02.imgst_dynamic "Starting imgStore server on http://localhost:8000
$(header 2 2)" || ok=0

size_before=$original_size
size_after=$(($size_before + 369911))
test_insert 'basic' pic3 foret.jpg \
'{ "Images": [ "pic1", "pic2", "pic3" ] }' $size_before $size_after || ok=0
# -----
test_insert 'duplicate content; case 1: of an existing image' \
pic4 papillon.jpg '{ "Images": [ "pic1", "pic2", "pic3", "pic4" ] }' || ok=0
# -----
test_insert 'duplicate content; case 2: of a newly inserted image' \
pic5 foret.jpg '{ "Images": [ "pic1", "pic2", "pic3", "pic4", "pic5" ] }' || ok=0

# Still FEEDBACK
# undelete a picture with duplicate (delete then insert the same id + content)
output_txt='"pic2", "pic3", "pic4", "pic5"'
test_delete pic1 "{ \"Images\": [ $output_txt ] }" || ok=0
test_insert ': undelete of duplicate' \
pic1 papillon.jpg "{ \"Images\": [ \"pic1\", $output_txt ] }" || ok=0

## --------------------------------------------------

# ======================================================================
stop_server

if [ "x$ok" = 'x1' ]; then
    echo -e "$0 ${bold}${green}SUCCESS${end}"
    exit 0
else
   echo -e "$0 ${bold}${red}FAILED${end} at some point"
   exit 1
fi

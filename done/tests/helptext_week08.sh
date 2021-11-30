#!/bin/sh

helptxt="imgStoreMgr [COMMAND] [ARGUMENTS]
  help: displays this help.
  list <imgstore_filename>: list imgStore content."
helptxt_next="  create <imgstore_filename> [options]: create a new imgStore.
      options are:
          -max_files <MAX_FILES>: maximum number of files.
                                  default value is 10
                                  maximum value is 100000
          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.
                                  default value is 64x64
                                  maximum value is 128x128
          -small_res <X_RES> <Y_RES>: resolution for small images.
                                  default value is 256x256
                                  maximum value is 512x512"
helptxt_next="$helptxt_next
  delete <imgstore_filename> <imgID>: delete image imgID from imgStore."
helptxt="$helptxt
$helptxt_next"

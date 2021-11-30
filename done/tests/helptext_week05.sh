#!/bin/sh

helptxt="imgStoreMgr [COMMAND] [ARGUMENTS]
  help: displays this help.
  list <imgstore_filename>: list imgStore content."
helptxt_next='  create <imgstore_filename>: create a new imgStore.'
helptxt_next="$helptxt_next
  delete <imgstore_filename> <imgID>: delete image imgID from imgStore."
helptxt="$helptxt
$helptxt_next"

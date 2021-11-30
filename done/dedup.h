#pragma once
#include "imgStore.h"

/**
 * check that a metadata has a unique name and that a same image is not stored several times
 * @param imgstFile
 * @param index of the metadata we want to check
 * @return ERR_DUPLICATE_ID if found name conflict else ERR_NONE
 * @details offset[RES_ORIGIN] = 0 if no image duplicate found
 */
int do_name_and_content_dedup(struct imgst_file * imgstFile, uint32_t index);
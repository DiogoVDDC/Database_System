#include "imgStore.h"
#include <stdbool.h>

bool SHA_equal(unsigned char sha1[], unsigned char sha2[])
{
    for(int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        if(sha1[i] != sha2[i]) {
            return false;
        }
    }
    return true;
}

/** @copybrief */
int do_name_and_content_dedup(struct imgst_file * imgstFile, uint32_t index)
{
    if(imgstFile == NULL || imgstFile->metadata == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if(index >= imgstFile->header.max_files) {
        fprintf(stderr, "ERROR: invalid index");
        return ERR_INVALID_ARGUMENT;
    }

    int ret = ERR_NONE;
    bool content_dup = false;
    for(int i = 0; i < imgstFile->header.max_files; ++i) {

        if(imgstFile->metadata[i].is_valid && i != index) {

            if(!strcmp(imgstFile->metadata[i].img_id, imgstFile->metadata[index].img_id)) {
                ret = ERR_DUPLICATE_ID;
            }
            if(SHA_equal(imgstFile->metadata[i].SHA, imgstFile->metadata[index].SHA)) {
                for(int j = RES_THUMB; j <= RES_ORIG; ++j) {
                    content_dup = true;
                    imgstFile->metadata[index].size[j] = imgstFile->metadata[i].size[j];
                    imgstFile->metadata[index].offset[j] = imgstFile->metadata[i].offset[j];
                }
            }
        }
    }

    if(!content_dup) {
        imgstFile->metadata[index].offset[RES_ORIG] = 0; //if no duplication => res_origin = 0
    }
    return ret;
}


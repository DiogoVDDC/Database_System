#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "imgStore.h"
#include "image_content.h"

/**
 *
 * @param origin_imgstFile
 * @param temp_imgstFile
 * @return error code as defined in error.h
 */
int gc_useful_data_copy(struct imgst_file* origin_imgstFile, struct imgst_file* temp_imgstFile)
{
    if(origin_imgstFile == NULL || temp_imgstFile == NULL ||
       origin_imgstFile->metadata == NULL || temp_imgstFile->metadata == NULL) return ERR_INVALID_ARGUMENT;

    int ret; //used to store the returned value of functions
    size_t max_size = 0;

    for(size_t i = 0; i < origin_imgstFile->header.max_files; ++i) {
        size_t img_size = origin_imgstFile->metadata[i].size[RES_ORIG];
        max_size = max_size < img_size ? img_size : max_size;
    }

    char* buffer = calloc(1, max_size);
    if(buffer == NULL) return ERR_IO;
    size_t nb_valid_images = 0; //used as index for the array of metadata of the temp imgstFile
    fseek(temp_imgstFile->file, sizeof(struct img_metadata) * origin_imgstFile->header.max_files + sizeof(struct imgst_header), SEEK_SET);

    for(size_t i = 0; i < origin_imgstFile->header.max_files; ++i) {
        if(origin_imgstFile->metadata[i].is_valid) {
            //read img and store it in the buffer
            fseek(origin_imgstFile->file, origin_imgstFile->metadata[i].offset[RES_ORIG], SEEK_SET);
            fread(buffer, origin_imgstFile->metadata[i].size[RES_ORIG], 1, origin_imgstFile->file);
            //insert this image in the temp imgstFile
            do_insert(buffer,
                      origin_imgstFile->metadata[i].size[RES_ORIG],
                      origin_imgstFile->metadata[i].img_id,
                      temp_imgstFile);
            //insert the other resolutions of this image if they are already created (ie stored in origin imgstFile)
            for(size_t res = RES_THUMB; res <= RES_SMALL; ++res) {
                if(origin_imgstFile->metadata[i].size[res] != 0) {
                    fseek(origin_imgstFile->file, origin_imgstFile->metadata[i].offset[res], SEEK_SET);
                    fseek(temp_imgstFile->file, NO_OFFSET, SEEK_END);
                    fread(buffer, origin_imgstFile->metadata[i].size[res], 1, origin_imgstFile->file);
                    temp_imgstFile->metadata[nb_valid_images].size[res] = origin_imgstFile->metadata[i].size[res];
                    temp_imgstFile->metadata[nb_valid_images].offset[res] = ftell(temp_imgstFile->file);
                    fwrite(buffer, origin_imgstFile->metadata[i].size[res], 1, temp_imgstFile->file);

                }
            }
            ++nb_valid_images;
        }
    }
    //need to rewrite metadata since have added smaller res
    for(size_t i = 0; i < temp_imgstFile->header.max_files; ++i) {
        if((ret = write_metadata(temp_imgstFile, i)) != ERR_NONE) return ret;
    }
    free(buffer);
    return ERR_NONE;
}
/**
 * determine if this imgstFile need a garbage collection (ie is there holes in its file)
 * @param imgstFile
 * @return true if need GC else false
 */
bool needGC(struct imgst_file* imgstFile)
{
    if(imgstFile == NULL || imgstFile->metadata == NULL || imgstFile->file == NULL) return false;
    fseek(imgstFile->file, NO_OFFSET, SEEK_END);
    long end_offset = ftell(imgstFile->file);
    long curr_offset = sizeof(struct imgst_header) + imgstFile->header.max_files * sizeof(struct img_metadata);

    while(curr_offset < end_offset) {
        bool found_next = false;
        size_t i = 0;
        while(i < imgstFile->header.max_files && !found_next) {
            int res = RES_THUMB;
            while(res <= RES_ORIG) {
                if( imgstFile->metadata[i].is_valid &&
                    imgstFile->metadata[i].size[res] != 0 &&
                    imgstFile->metadata[i].offset[res] == curr_offset) {

                    curr_offset += imgstFile->metadata[i].size[res];
                    found_next = true;
                }
                ++res;
            }
            ++i;
        }
        if(!found_next) {
            return true;
        }
    }
    return false;
}

/**
 * helper function called to close imgstFiles open during the garbage collection
 * @param origin_imgstFile
 * @param temp_imgstFile
 */
void close_gc(struct imgst_file* origin_imgstFile, struct imgst_file* temp_imgstFile)
{
    do_close(origin_imgstFile);
    do_close(temp_imgstFile);
}

/**
 * do the garbage collection of the databse
 * @param imgst_name of the original data base file
 * @param temp_name of the temporary file we will use while gc
 * @return error code as defined in error.h
 */
int do_gbcollect(const char* imgst_name, const char* temp_name)
{

    /* --- phase 1 :
     *              check if gc necessary and if can do it, prepare it */

    if(imgst_name == NULL || temp_name == NULL) return ERR_INVALID_ARGUMENT;

    int ret; //used to store the returned value of functions

    //open/create the imgst_files
    struct imgst_file origin_imgstFile;
    if((ret = do_open(imgst_name, "rb", &origin_imgstFile)) != ERR_NONE) return ret;


    struct imgst_file temp_imgstFile = {.header = origin_imgstFile.header};
    //other fields will be init in do_create
    if((ret = do_create(temp_name, &temp_imgstFile)) != ERR_NONE) return ret;

    //if there is no "holes" in the origin file, no gc to do
    if(!needGC(&origin_imgstFile)) {
        close_gc(&origin_imgstFile, &temp_imgstFile);
        return ERR_NONE;
    }

    /* --- phase 2:
     *          Do the copy of all the useful data from origin file to the temp one */
    ret = gc_useful_data_copy(&origin_imgstFile, &temp_imgstFile);

    /* ---phase 3:
     *          gc finished, delete origin file, keep only the temp that we will rename with the name of the origin's one */

    if(ret == ERR_NONE) {
        if ((ret = remove(imgst_name)) != ERR_NONE) {
            close_gc(&origin_imgstFile, &temp_imgstFile);
            return ret;
        }
        if ((ret = rename(temp_name, imgst_name)) != ERR_NONE) {
            close_gc(&origin_imgstFile, &temp_imgstFile);
            return ret;
        }
    }
    close_gc(&origin_imgstFile, &temp_imgstFile);
    return ERR_NONE;
}
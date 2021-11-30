#include <openssl/sha.h>
#include <stdbool.h>
#include "imgStore.h"
#include "dedup.h"
#include "image_content.h"

#define NO_OFFSET 0

/** @copybrief */
int do_insert(const char* buffer, size_t img_size, const char* img_id, struct imgst_file* imgst_file)
{
    if(img_size == 0 || img_id == NULL || buffer == NULL || imgst_file == NULL || imgst_file->metadata == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if(imgst_file->header.num_files >= imgst_file->header.max_files) {
        fprintf(stderr, "The database is full, it has reached %u files capacity", imgst_file->header.max_files);
        return ERR_FULL_IMGSTORE;
    }

    size_t i = 0;
    bool found_space = false;

    // check for space to insert new file
    while(i < imgst_file->header.max_files && !found_space) {

        //once file is found start initialising the metadata
        if(!imgst_file->metadata[i].is_valid) {
            found_space = true;
            SHA256(buffer, img_size, imgst_file->metadata[i].SHA);
            strncpy(imgst_file->metadata[i].img_id, img_id, MAX_IMG_ID);
            imgst_file->metadata[i].size[RES_ORIG] = img_size;
            imgst_file->metadata[i].is_valid = NON_EMPTY;

            for(int res = RES_THUMB; res <= RES_SMALL; ++res) {
                imgst_file->metadata[i].offset[res] = 0;
                imgst_file->metadata[i].size[res] = 0;
            }
        }
        i++;
    }
    i--;

    // Dedup content of newly semi initialised metadata i
    int err_dedup = do_name_and_content_dedup(imgst_file, i);
    if(err_dedup != ERR_NONE) {
        return err_dedup;
    }

    // If no duplicate found insert image at the end of the file
    if(imgst_file->metadata[i].offset[RES_ORIG] == 0) { //offset == 0 is an indicator of the absence of a duplicate
        //set the writing pointer to the end of the file
        if (fseek(imgst_file->file, NO_OFFSET, SEEK_END) != ERR_NONE) {
            fprintf(stderr, "Error: can't set head reader at the end of the file");
            return ERR_IO;
        }

        //get offset by getting where is the end of the file
        imgst_file->metadata[i].offset[RES_ORIG] = ftell(imgst_file->file);

        //write new resized image to end of file
        int nb_image_to_write = 1;
        if (fwrite(buffer, img_size, nb_image_to_write, imgst_file->file) != nb_image_to_write) {
            fprintf(stderr, "ERROR: fail to write resized image");
            return ERR_IO;
        }
    }

    //returns the height and width of image loaded in buffer
    uint32_t height = 0;
    uint32_t width = 0;
    int err_get_res = get_resolution(&height, &width, buffer, img_size);
    if (err_get_res != ERR_NONE) {
        return err_get_res;
    }
    //update metadata and header parameters
    imgst_file->metadata[i].res_orig[0] = width;
    imgst_file->metadata[i].res_orig[1] = height;


    int err_write_metadata = write_metadata(imgst_file, i);
    if(err_write_metadata != ERR_NONE) {
        return err_write_metadata;
    }
    //update the header and write it on the file
    imgst_file->header.num_files += 1;
    imgst_file->header.imgst_version += 1;
    int err_write_header = write_header(imgst_file);

    if(err_write_header != ERR_NONE) {
        return err_write_header;
    }

    return ERR_NONE;
}
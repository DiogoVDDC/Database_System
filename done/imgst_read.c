#include <stdlib.h>
#include "imgStore.h"
#include "image_content.h"

/** @copybrief */
int do_read(const char* img_id, const int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file)
{
    if(img_id == NULL || resolution < 0 || resolution >= NB_RES
       || imgst_file == NULL || imgst_file->metadata == NULL
       || image_buffer == NULL || image_size == NULL) {
        fprintf(stderr, "ERROR: invalid argument given to do_read");
        return ERR_INVALID_ARGUMENT;
    }

    if(imgst_file->header.num_files == 0) return ERR_FILE_NOT_FOUND;

    // Loop to find valid image with image id equal img_id
    for (int i = 0; i < imgst_file->header.max_files; i++) {
        if(imgst_file->metadata[i].is_valid && !strncmp(img_id, imgst_file->metadata[i].img_id, MAX_IMG_ID+1)) {

            // if image does not already exist in resolution requested then resize
            if (imgst_file->metadata[i].size[resolution] == 0 || imgst_file->metadata[i].offset[resolution] == 0) {
                int err_resize = lazily_resize(resolution, imgst_file, i);

                if (err_resize != ERR_NONE) {
                    fprintf(stderr, "ERROR: failed during resizing \n");
                    return err_resize;
                }
            }

            *image_size = imgst_file->metadata[i].size[resolution];

            // create pointer in memory to store buffer
            char* img_buffer = calloc(1, *image_size);

            if (img_buffer == NULL) {
                return ERR_OUT_OF_MEMORY;
            }

            //moving to the position of the metadata
            if (fseek(imgst_file->file, imgst_file->metadata[i].offset[resolution], SEEK_SET) !=
                ERR_NONE) {
                free(img_buffer);
                fprintf(stderr, "Error: can't set head reader at the location of the image we want to read");
                return ERR_IO;
            }

            int nb_image_to_read = 1;
            // writing into the img_buffer from imgst_file
            if (fread(img_buffer, *image_size, nb_image_to_read, imgst_file->file) != nb_image_to_read) {
                free(img_buffer);
                fprintf(stderr, "ERROR: fail to read from file to img_buffer");
                return ERR_IO;
            }

            // affecting the changes
            *image_buffer = img_buffer;
            return ERR_NONE;
        }
    }
    return ERR_FILE_NOT_FOUND;
}
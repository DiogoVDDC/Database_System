#include "imgStore.h"

/**
 * delete a given image in the database
 * @param img_id
 * @param imgstFile
 * @return err_code as def in error.h
 */
int do_delete(const char * img_id, struct imgst_file* imgstFile)
{
    if(img_id == NULL || imgstFile == NULL) {
        fprintf(stderr, "at leat one of do_create arguments is NULL");
        return ERR_INVALID_ARGUMENT;
    }

    if(strlen(img_id) == 0 || strlen(img_id) > MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }
    if(imgstFile->metadata == NULL) {
        fprintf(stderr, "ERROR: metadata pointer");
        return ERR_INVALID_ARGUMENT;
    }
    if(imgstFile->header.num_files == 0) {
        return ERR_FILE_NOT_FOUND;
    }

    for (int i = 0; i < imgstFile->header.max_files; i++) {
        //the metadata must have the same ids, they must be of the same length and have their valid bit set to 1
        if (strncmp(img_id, imgstFile->metadata[i].img_id, strlen(img_id)) == 0 &&
            imgstFile->metadata[i].is_valid == NON_EMPTY) {
            //reset the file pointer to the start of the file
            rewind(imgstFile->file);

            //modify the header
            imgstFile->header.num_files--;
            imgstFile->header.imgst_version++;

            //write the header to the stream
            if (fwrite(&imgstFile->header, sizeof(struct imgst_header), NB_HEADER_PER_FILE, imgstFile->file) !=
                NB_HEADER_PER_FILE) {
                fprintf(stderr, "Error while deleting image, when write back the updated header");
                return ERR_IO;
            }

            //change the file pointer of the stream to where the image metadata starts
            if (fseek(imgstFile->file, i * sizeof(struct img_metadata) + sizeof(struct imgst_header), SEEK_SET) !=
                ERR_NONE) {
                fprintf(stderr, "Error during fseek to correct metadata location");
                return ERR_IO;
            }

            //modify the metadata to be not valid
            imgstFile->metadata[i].is_valid = EMPTY;

            //write metadata to the stream
            size_t const nb_metadata_to_write = 1;
            if (fwrite(&(imgstFile->metadata[i]), sizeof(struct img_metadata), nb_metadata_to_write,
                       imgstFile->file) != nb_metadata_to_write) {
                fprintf(stderr, "Error while deleting image, when write back metadata of deleted file");
                return ERR_IO;
            }

            return ERR_NONE;
        }
    }
    // return file not found if there was no match in the ids
    return ERR_FILE_NOT_FOUND;
}
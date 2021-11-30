/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 */

#include "imgStore.h"

#include <string.h> // for strncpy
#include <stdio.h>
#include <stdlib.h>

/**
 * write an imageFile in the dataBase of name imgst_filename
 * @param imgst_filename
 * @param DBFILE the struct imgst_file* that will represent the database
 * @return err_code as def in error.h
 */
int do_create(const char *imgst_filename, struct imgst_file* DBFILE)
{
    if (imgst_filename == NULL || strlen(imgst_filename) == 0 || DBFILE == NULL) {
        fprintf(stderr, "at leat one of do_create arguments is NULL");
        return ERR_INVALID_ARGUMENT;
    }

    DBFILE->file = fopen(imgst_filename, "wb");

    if(DBFILE->file == NULL) {
        return ERR_IO;
    }

    // Sets header fields
    strncpy(DBFILE->header.imgst_name, CAT_TXT, MAX_IMGST_NAME);
    DBFILE->header.imgst_name[MAX_IMGST_NAME] = '\0';
    DBFILE->header.imgst_version = 0;
    DBFILE->header.num_files = 0;

    //writes header to DBFILE stream
    size_t const exp_nb_elem_w = NB_HEADER_PER_FILE + DBFILE->header.max_files;
    size_t nb_elem_written = fwrite(&(DBFILE->header), sizeof(struct imgst_header), NB_HEADER_PER_FILE, DBFILE->file);

    DBFILE->metadata = calloc(DBFILE->header.max_files, sizeof (struct img_metadata));
    if(DBFILE->metadata == NULL) {
        fprintf(stderr, "ERROR: can't calloc");
        return ERR_OUT_OF_MEMORY;
    }
    //write metadata to DBFILE stream
    nb_elem_written += fwrite(DBFILE->metadata, sizeof(struct img_metadata), DBFILE->header.max_files, DBFILE->file);

    //throw error if not enough element were written
    if (nb_elem_written != exp_nb_elem_w) {
        fprintf(stderr, "Unable to completely write the database on the disk,\n"
                "only %zu elements wrote among %zu\n", nb_elem_written, exp_nb_elem_w);
        fclose(DBFILE->file);
        return ERR_IO;
    }

    printf("%zu item(s) written\n", nb_elem_written);
    return ERR_NONE;
}

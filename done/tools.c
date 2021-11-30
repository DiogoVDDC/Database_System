/* ** NOTE: undocumented in Doxygen
 * @file tools.c
 * @brief implementation of several tool functions for imgStore
 *
 * @author Mia Primorac
 */

#include "imgStore.h"

#include <stdint.h> // for uint8_t
#include <stdlib.h> // for malloc and calloc
#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define ERR_ATOI -1

/** @copybrief */
int write_header(struct imgst_file* imgst_file)
{
    //moving to the start of the file
    if (fseek(imgst_file->file, NO_OFFSET, SEEK_SET) != ERR_NONE) {
        fprintf(stderr, "Error: can't set head reader at the start of the file");
        return ERR_IO;
    }

    // writing updated header to file
    int nb_header_to_write = 1;
    if(fwrite(&imgst_file->header, sizeof (struct imgst_header), nb_header_to_write, imgst_file->file) != nb_header_to_write) {
        fprintf(stderr, "ERROR: fail to write updated metadata");
        return ERR_IO;
    }

    return ERR_NONE;
}

/** @copybrief */
int write_metadata(struct imgst_file* imgst_file, size_t i)
{
    //moving to the start of the file
    if (fseek(imgst_file->file, i * sizeof (struct img_metadata) + sizeof (struct imgst_header), SEEK_SET) != ERR_NONE) {
        fprintf(stderr, "Error: can't set head reader at the start of the file");
        return ERR_IO;
    }

    // writing metadata to file
    int nb_metadata_to_write = 1;
    if(fwrite(&imgst_file->metadata[i], sizeof (struct img_metadata), nb_metadata_to_write, imgst_file->file) != nb_metadata_to_write) {
        return ERR_IO;
    }
    return ERR_NONE;
}


/********************************************************************//**
 * Human-readable SHA
 */
static void
sha_to_string(const unsigned char *SHA,
              char *sha_string)
{
    if (SHA == NULL) {
        return;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * imgStore header display.
 */
void print_header(const struct imgst_header * header)
{
    printf("*****************************************\n");
    printf("**********IMGSTORE HEADER START**********\n");
    printf("TYPE: %31s\n", header->imgst_name);
    printf("VERSION: %"
           PRIu32
           "\n", header->imgst_version);
    printf("IMAGE COUNT: %"
           PRIu32
           "\t\tMAX IMAGES: %"
           PRIu32
           "\n",
           header->num_files, header->max_files);
    printf("THUMBNAIL: %d x %d\tSMALL: %d x %d\n", header->res_resized[0], header->res_resized[1], header->res_resized[2], header->res_resized[3]);
    printf("***********IMGSTORE HEADER END***********\n");
    printf("*****************************************\n");
}

/********************************************************************//**
 * Metadata display.
 */
void
print_metadata(const struct img_metadata* metadata)
{

    char sha_printable[2 * SHA256_DIGEST_LENGTH + 1];
    sha_to_string(metadata->SHA, sha_printable);

    printf("IMAGE ID: %s\n", metadata->img_id);
    printf("SHA: %s\n", sha_printable);
    printf("VALID: %"
           PRIu16
           "\n", metadata->is_valid);
    printf("UNUSED: %"
           PRIu16
           "\n", metadata->unused_16);
    printf("OFFSET ORIG.: %"
           PRIu64
           "\t\tSIZE ORIG.: %"
           PRIu32
           "\n",
           metadata->offset[RES_ORIG], metadata->size[RES_ORIG]);
    printf("OFFSET THUMB.: %"
           PRIu64
           "\t\tSIZE THUMB.: %"
           PRIu32
           "\n",
           metadata->offset[RES_THUMB], metadata->size[RES_THUMB]);
    printf("OFFSET SMALL : %"
           PRIu64
           "\t\tSIZE SMALL : %"
           PRIu32
           "\n",
           metadata->offset[RES_SMALL], metadata->size[RES_SMALL]);
    printf("ORIGINAL: %"
           PRIu32
           " x %"
           PRIu32
           "\n",
           metadata->res_orig[0], metadata->res_orig[1]); //size of dim 0 and dim 1
    printf("*****************************************\n");
}

/**
 * open the database file with the given open_mode and use it to construct imgst_file
 * @param imgst_filename
 * @param open_mode
 * @param imgst_file
 * @return err code as def in error.h
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file)
{
    if(imgst_filename == NULL || open_mode == NULL || imgst_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if(strncmp(open_mode, "rb+", 2) || strncmp(open_mode, "rb", 2)) {
        return ERR_INVALID_ARGUMENT;
    }


    // open stream and return pointer into imgst_file stream
    imgst_file->file = fopen(imgst_filename, open_mode);
    if(imgst_file->file == NULL) {
        return ERR_IO;
    }

    // read header from stream
    if(fread(&(imgst_file->header), sizeof(struct imgst_header), NB_HEADER_PER_FILE, imgst_file->file) != NB_HEADER_PER_FILE) {
        fclose(imgst_file->file);
        return ERR_IO;
    }

    imgst_file->metadata = calloc(imgst_file->header.max_files, sizeof (struct img_metadata));

    //failed calloc
    if(imgst_file->metadata == NULL) {
        fclose(imgst_file->file);
        return ERR_OUT_OF_MEMORY;
    }

    // read metadatas from file to the imgst_file->metadata
    if(fread(imgst_file->metadata, sizeof(struct img_metadata), imgst_file->header.max_files, imgst_file->file)
       != imgst_file->header.max_files) {
        fclose(imgst_file->file);
        free(imgst_file->metadata);
        return ERR_IO;
    }

    return ERR_NONE;
}

/**
 * close database file and other stuff
 * @param imgst_file to close
 */
void do_close (struct imgst_file* imgst_file)
{
    if(imgst_file->file == NULL) {
        fprintf(stderr, "ERROR: Pointer to file stream was null\n");
        return;
    }
    vector_metadata_delete(imgst_file);
    fclose(imgst_file->file);
    imgst_file->file = NULL;
}

/** @copybrief */
void vector_metadata_delete(struct imgst_file* imgst_file)
{
    if (imgst_file != NULL) { //if metadata already NULL no problem
        free(imgst_file->metadata);
        imgst_file->metadata = NULL;
    }
}
/** @copybrief */
int resolution_atoi(const char* resolution)
{
    if(resolution == NULL) return ERR_ATOI;

    if(!strcmp(resolution, "thumb") || !strcmp(resolution, "thumbnail")) {
        return RES_THUMB;
    } else if(!strcmp(resolution, "small")) {
        return RES_SMALL;
    } else if (!strcmp(resolution, "orig") || !strcmp(resolution, "original")) {
        return RES_ORIG;
    }
    return ERR_ATOI;
}


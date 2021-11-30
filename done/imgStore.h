/**
 * @file imgStore.h
 * @brief Main header file for imgStore core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The image imgStore starts with exactly one header structure
 * followed by exactly imgst_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * imgStore file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 */

#ifndef IMGSTOREPRJ_IMGSTORE_H
#define IMGSTOREPRJ_IMGSTORE_H

#include "error.h" /* not needed in this very file,
                    * but we provide it here, as it is required by
                    * all the functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define CAT_TXT "EPFL ImgStore binary"

/* constraints */
#define MAX_IMGST_NAME  31  // max. size of a ImgStore name
#define MAX_IMG_ID     127  // max. size of an image id
#define MAX_MAX_FILES 100000
#define VECTOR_PADDING 100

/* For is_valid in imgst_metadata */
#define EMPTY 0
#define NON_EMPTY 1

//describe how are stored the resolutions in the res_resized array
#define X_COORD_LOCATION 0
#define Y_COORD_LOCATION 1
// imgStore library internal codes for different image resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3
#define MAX_THUMB_RES 128
#define MAX_SMALL_RES 256

#define NB_DIMENSIONS 2
#define NO_OFFSET 0

#define NB_HEADER_PER_FILE 1

#ifdef __cplusplus
extern "C" {
#endif

struct imgst_header {
    char imgst_name[MAX_IMGST_NAME + 1];
    uint32_t imgst_version;
    uint32_t num_files; //number of files currently in the database
    const uint32_t max_files; //maximum number of files in the database
    // (also size of metadata array)
    const uint16_t res_resized[(NB_RES - 1)*(NB_DIMENSIONS)]; // max resolution of images thumbnailX, thumbnailY, smallX,_y
    //don't have origin res here + should not be modified after initialisation (image creation)

    //both fields below are reserved for potential future improvements
    uint32_t unused_32;
    uint64_t unused_64;
};

struct img_metadata {
    char img_id[MAX_IMG_ID + 1];
    unsigned char SHA[SHA256_DIGEST_LENGTH]; //hash code of the image
    uint32_t res_orig[NB_DIMENSIONS];
    uint32_t size[NB_RES]; //size (in bytes) of images of different resolutions (thumb, small, origin) (indices given by RES_X)
    uint64_t offset[NB_RES]; //positions of images in the database, in same order than size
    uint16_t is_valid; // has value NON_EMPTY when valid, and EMPTY when invalid
    uint16_t unused_16;
};


struct imgst_file {
    FILE *file;
    struct imgst_header header;
    struct img_metadata* metadata;
};

/** different format types accepted */
enum do_list_mode {
    STDOUT,
    JSON
};
//----------------------------------------------------------------------------------------------------------

/**
 * @brief frees the metadata pointer and memory
 *
 * @param vector_metadata is the vector to be freed
 */
void vector_metadata_delete(struct imgst_file* imgst_file);

/**
 * @brief Prints imgStore header informations.
 *
 * @param header The header to be displayed.
 */
void print_header( const struct imgst_header * header);

/**
 * @brief Prints image metadata informations.
 *
 * @param metadata The metadata of one image.
 */
void print_metadata (const struct img_metadata * metadata);

/**
 * @brief Open imgStore file, read the header and all the metadata.
 *
 * @param imgst_filename Path to the imgStore file
 * @param open_mode Mode for fopen(), eg.: "rb", "rb+", etc.
 * @param imgst_file Structure for header, metadata and file pointer.
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file);

/**
 * @brief Do some clean-up for imgStore file handling.
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
void do_close (struct imgst_file * const imgst_file);

/**
 * @brief List of possible output modes for do_list
 * @param format specify the format of the returned string
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 * @return string containing info about the database (header fields and metadatas)
 */
char* do_list(const struct imgst_file* imgst_file, enum do_list_mode format);

/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 *
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 */
int do_create(const char *imgst_filename, struct imgst_file* imgst_file);

/**
 * @brief Deletes an image from a imgStore imgStore.
 *
 * Effectively, it only invalidates the is_valid field and updates the
 * metadata.  The raw data content is not erased, it stays where it
 * was (and  new content is always appended to the end; no garbage
 * collection).
 *
 * @param img_id The ID of the image to be deleted.
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_delete(const char * img_id, struct imgst_file* imgst_file);

/**
 * @brief Transforms resolution string to its int value.
 *
 * @param resolution The resolution string. Shall be "original",
 *        "orig", "thumbnail", "thumb" or "small".
 * @return The corresponding value or -1 if error.
 */
int resolution_atoi(const char* resolution);

/**
 * @brief Reads the content of an image from a imgStore.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char* img_id, int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file);

/**
 * @brief Insert image in the imgStore file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @return Some error code. 0 if no error.
 */
int do_insert(const char* buffer, size_t img_size, const char* img_id, struct imgst_file* imgst_file);

/**
 * @brief Removes the deleted images by moving the existing ones
 *
 * @param imgst_path The path to the imgStore file
 * @param imgst_tmp_bkp_path The path to the a (to be created) temporary imgStore backup file
 * @return Some error code. 0 if no error.
 */
int do_gbcollect (const char *imgst_path, const char *imgst_tmp_bkp_path);


/**
 * helper method to write the header on the disk
 * @param number_files
 * @param file the file where we want to write the header (not always imgst_file->file @see gc)
 * @return error code as defined in error.h
 */
int write_header(struct imgst_file* imgst_file);

/**
 * helper method to write the metadata of the ith image on the file
 * @param i index of the metadata in the array
 * @param imgst_file
 * @param file the file where we want to write this metadata (not always imgst_file->file @see gc)
 * @return error code as defined in error.h
 */
int write_metadata(struct imgst_file* imgst_file, size_t i);

#ifdef __cplusplus
}
#endif
#endif

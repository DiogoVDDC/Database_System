/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpretor for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for _unused
#include "imgStore.h"
#include "error.h"

#include "image_content.h"
#include "dedup.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <vips/vips.h>

#define STR(X) #X

#define NB_OF_COMMANDS 7
#define EXPECTED_NB_ARGS_DO_LIST 1
#define EXPECTED_NB_ARGS_DO_CREATE_MAX_FILES 1
#define EXPECTED_NB_ARGS_DO_CREATE_RES 2
#define EXPECTED_NB_ARGS_DO_DELETE 2
#define EXPECTED_NB_ARGS_DO_INSERT 3
#define EXPECTED_NB_ARGS_GC 2
#define MIN_NB_ARGS_DO_READ 2
#define MAX_NB_ARGS_DO_READ 3

typedef int (*command)(int args, char* argv[]);

struct command_mapping {
    const char* command_name;
    command command;
};

/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int
do_list_cmd (int argc, char* argv[])
{
    if(argv == NULL) return ERR_INVALID_ARGUMENT;

    if(argc < EXPECTED_NB_ARGS_DO_LIST) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if(argc>EXPECTED_NB_ARGS_DO_LIST) {
        fprintf(stderr, "too much arguments given to list_cmd");
        return ERR_INVALID_ARGUMENT;
    }
    const char* filename = argv[0];
    if(strlen(filename) == 0 || strlen(filename) > MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }

    struct imgst_file myfile = {0};

    //read data from my filename and open the stream of imgst_file
    int err_do_open = do_open(filename, "rb", &myfile);

    // printf("ERR DO OPEN: %d", err_do_open);

    if(err_do_open != ERR_NONE) {
        return err_do_open;
    }

    // display the data of my file
    do_list(&myfile, STDOUT);

    //close the stream
    do_close(&myfile);

    return ERR_NONE;
}

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int
do_create_cmd (int argc, char* argv[])
{
    if(argv == NULL) return ERR_INVALID_ARGUMENT;

    if(argc <= 0) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    const char* filename = argv[0]; ++argv; --argc;

    // default initialization :
    uint32_t max_files = 10;
    uint16_t thumb_res_x = 64;
    uint16_t thumb_res_y = 64;
    uint16_t small_res_x = 256;
    uint16_t small_res_y = 256;

    while(argc > 0) { //read others arguments to set img characteristics
        if (!strcmp(argv[0], "-max_files")) {
            if (--argc < EXPECTED_NB_ARGS_DO_CREATE_MAX_FILES) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            max_files = atouint32((++argv)[0]);
            --argc;
            ++argv;
            if (max_files == 0 || max_files > MAX_MAX_FILES) { //if given value was 0 or not representable on 32 bits
                return ERR_MAX_FILES;
            }
        } else {
            if (!strcmp(argv[0], "-thumb_res")) {
                if (--argc < EXPECTED_NB_ARGS_DO_CREATE_RES) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                }
                thumb_res_x = atouint16((++argv)[0]);
                thumb_res_y = atouint16((++argv)[0]);
                ++argv;
                argc -= 2;

                if (thumb_res_x == 0 || thumb_res_y == 0 ||
                    thumb_res_x > MAX_THUMB_RES || thumb_res_y > MAX_THUMB_RES ||
                    small_res_x > MAX_SMALL_RES || small_res_y > MAX_SMALL_RES) {
                    return ERR_RESOLUTIONS;
                }
            } else {
                if (!strcmp(argv[0], "-small_res")) {
                    if (--argc < EXPECTED_NB_ARGS_DO_CREATE_RES) {
                        return ERR_NOT_ENOUGH_ARGUMENTS;
                    }

                    small_res_x = atouint16((++argv)[0]);
                    --argc;
                    small_res_y = atouint16((++argv)[0]);
                    --argc;
                    ++argv;
                    if (small_res_x == 0 || small_res_y == 0) {
                        return ERR_RESOLUTIONS;
                    }
                } else {
                    return ERR_INVALID_ARGUMENT;
                }
            }
        }
    }

    puts("Create");

    struct imgst_file myfile = {.header.max_files = max_files, .header.num_files = 0, .header.imgst_version = 0,
               .header.res_resized = {thumb_res_x, thumb_res_y, small_res_x, small_res_y}
    };

    //call to 'do_create' will initialize the others fields of myfile
    int err_do_create = do_create(filename, &myfile);

    if(err_do_create != ERR_NONE) {
        return err_do_create;
    }

    print_header(&myfile.header);
    do_close(&myfile);
    return ERR_NONE;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int help (int argc, char* argv[])
{
    printf("imgStoreMgr [COMMAND] [ARGUMENTS]\n"
           "  help: displays this help.\n"
           "  list <imgstore_filename>: list imgStore content.\n"
           "  create <imgstore_filename> [options]: create a new imgStore.\n"
           "      options are:\n"
           "          -max_files <MAX_FILES>: maximum number of files.\n"
           "                                  default value is 10\n"
           "                                  maximum value is 100000\n"
           "          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n"
           "                                  default value is 64x64\n"
           "                                  maximum value is 128x128\n"
           "          -small_res <X_RES> <Y_RES>: resolution for small images.\n"
           "                                  default value is 256x256\n"
           "                                  maximum value is 512x512\n"
           "  read <imgstore_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n"
           "      read an image from the imgStore and save it to a file.\n"
           "      default resolution is \"original\".\n"
           "  insert <imgstore_filename> <imgID> <filename>: insert a new image in the imgStore.\n"
           "  delete <imgstore_filename> <imgID>: delete image imgID from imgStore.\n"
           "gc <imgstore_filename> <tmp imgstore_filename>: performs garbage collecting on imgStore. Requires a temporary filename for copying the imgStore.");

    return ERR_NONE;
}

/********************************************************************//**
 * Opens imgStore file and calls do_delete command.
 ********************************************************************** */
int do_delete_cmd (int argc, char* argv[])
{
    //check args
    if(argv == NULL) return ERR_INVALID_ARGUMENT;

    if(argc < EXPECTED_NB_ARGS_DO_DELETE) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if(argc > EXPECTED_NB_ARGS_DO_DELETE) {
        fprintf(stderr, "too much arguments given to delete");
        return ERR_INVALID_ARGUMENT;
    }
    const char* img_store_filename = argv[0]; --argc;
    if(strlen(img_store_filename) == 0 || strlen(img_store_filename) > MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }
    const char* imgID = (++argv)[0]; --argc;
    if(strlen(imgID) == 0 || strlen(imgID) > MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }

    struct imgst_file myfile;
    int err_do_open = do_open(img_store_filename, "rb+", &myfile);
    if(err_do_open != ERR_NONE) return err_do_open;

    int err_do_delete = do_delete(imgID, &myfile);

    if(err_do_delete != ERR_NONE) {
        do_close(&myfile);
        return err_do_delete;
    }

    do_close(&myfile);

    return ERR_NONE;
}
//----------------------------------------------------------------------------------
/**
 * to read an extern image and put it the buffer
 * @param buffer
 * @param img_size
 * @param filename
 * @return error code as defined in error.h
 */
int read_disk_image(char** buffer, size_t* img_size, const char* filename)
{
    // check args
    if(filename == NULL || img_size == NULL || buffer == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    //try to open database file
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return ERR_IO;
    }

    //Get file length
    fseek(fp, 0, SEEK_END);
    *img_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    //Allocate memory
    *buffer = calloc(1, *img_size);
    if (*buffer == NULL) {
        fclose(fp);
        return ERR_OUT_OF_MEMORY;
    }
    //read img and store it in the buffer
    if(fread(*buffer, sizeof(unsigned char), *img_size, fp) != *img_size) {
        fclose(fp);
        return ERR_IO;
    }
    fclose(fp);
    return ERR_NONE;
}

/**
 * helper method to check arguments given to do_read_cmd and do_insert_cmd
 * @param argc
 * @param argv
 * @param img_store_filename
 * @param imgID
 * @param nb_min_arg
 * @param nb_max_arg
 * @return
 */
int check_args_insert_and_read(int* argc, char** argv[], const char** img_store_filename, const char** imgID, int nb_min_arg, int nb_max_arg)
{

    if(argc == NULL || argv == NULL || img_store_filename == NULL || imgID == NULL) return ERR_INVALID_ARGUMENT;

    if(*argv == NULL) return ERR_INVALID_ARGUMENT;

    if(*argc < nb_min_arg) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if(*argc > nb_max_arg) {
        return ERR_INVALID_ARGUMENT;
    }

    *img_store_filename = (*argv)[0]; --(*argc);
    if(strlen(*img_store_filename) == 0 || strlen(*img_store_filename) > MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }

    *imgID = (++(*argv))[0]; --(*argc);
    if(strlen(*imgID) == 0 || strlen(*imgID) > MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }
    return ERR_NONE;
}


/********************************************************************//**
 * Prepares and calls do_insert command.
********************************************************************** */
int do_insert_cmd (int argc, char* argv[])
{
    const char* img_store_filename = NULL;
    const char* imgID = NULL;

    int ret = check_args_insert_and_read(&argc, &argv, &img_store_filename, &imgID, EXPECTED_NB_ARGS_DO_INSERT, EXPECTED_NB_ARGS_DO_INSERT);
    if(ret != ERR_NONE) return ret;

    const char* filename = (++argv)[0]; --argc;
    if(strlen(img_store_filename) == 0) {
        return ERR_INVALID_FILENAME;
    }

    struct imgst_file myfile;
    //open the database
    int err_do_open = do_open(img_store_filename, "rb+", &myfile);
    if(err_do_open != ERR_NONE) {
        do_close(&myfile);
        return err_do_open;
    }

    if(myfile.header.num_files +1 >= myfile.header.max_files) {
        return ERR_FULL_IMGSTORE;
    }

    char* buffer = NULL;
    size_t img_size = 0;

    int err_read_disk = read_disk_image(&buffer, &img_size, filename);

    if(err_read_disk != ERR_NONE) {
        free(buffer);
        return err_read_disk;
    }

    int err_do_insert = do_insert(buffer, img_size, imgID, &myfile);

    free(buffer);
    do_close(&myfile);

    return err_do_insert;

}
//============================================================================================================
/**
 * store the constructed name with img_id and resolution_suffix and store it in name_out
 * @param img_id
 * @param resolution_suffix
 * @param name_out
 */
int create_name(const char * img_id, const char * resolution_suffix, char** name_out)
{
    if(img_id == NULL || resolution_suffix == NULL || name_out == NULL) {
        fprintf(stderr, "ERROR invalid arguments given to create_name function");
        return ERR_INVALID_ARGUMENT;
    }

    const char * image_type = ".jpg";
    size_t name_len = strlen(resolution_suffix) + strlen(img_id) + strlen(image_type);

    *name_out = calloc(1,  name_len+1);
    if(*name_out == NULL) {
        fprintf(stderr, "ERROR while allocation in create_name");
        return ERR_OUT_OF_MEMORY;
    }

    strcat(*name_out, img_id);
    strcat(*name_out, resolution_suffix);
    strcat(*name_out, image_type);
    (*name_out)[name_len] = '\0';
    return ERR_NONE;
}
/**
 * write an img on an external file on the disk
 * @param img_id
 * @param res
 * @param image_buffer
 * @param image_size
 * @return error code as defined in error.h
 */
int write_disk_image(const char * img_id, const int res, char* image_buffer, uint32_t image_size)
{

    if(img_id == NULL || res < 0 || res >= NB_RES || image_buffer == NULL || image_size == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    char* name_out = NULL;
    int ret;

    switch (res) {
    case 0:  ret = create_name(img_id, "_thumb", &name_out); break;
    case 1:  ret = create_name(img_id, "_small", &name_out); break;
    default: ret = create_name(img_id, "_orig", &name_out);
    }
    if(ret != ERR_NONE) return ret;

    FILE* fp = fopen(name_out, "wb");
    if (fp == NULL) {
        free(name_out);
        return ERR_IO;
    }

    int nb_files_to_write = 1;
    if(fwrite(image_buffer, image_size, nb_files_to_write, fp) != nb_files_to_write) {
        free(name_out);
        fclose(fp);
        fprintf(stderr, "ERROR: can't write image into disk");
        return ERR_IO;
    }

    free(name_out);
    fclose(fp);
    return ERR_NONE;
}

/********************************************************************//**
 * Prepares and calls do_read command.
********************************************************************** */
int do_read_cmd (int argc, char* argv[])
{
    const char* imgID = NULL;
    const char* img_store_filename = NULL;

    int ret = check_args_insert_and_read(&argc, &argv, &img_store_filename, &imgID, MIN_NB_ARGS_DO_READ, MAX_NB_ARGS_DO_READ);
    if(ret != ERR_NONE) return ret;

    char* resolution_argument = "original"; //default resolution
    if(argc != 0) {
        resolution_argument = (++argv)[0]; --argc;
    }

    int res = resolution_atoi(resolution_argument);

    if(res == -1) { //error return value of resolution_atoi
        return ERR_RESOLUTIONS;
    }

    struct imgst_file myfile;

    int err_do_open = do_open(img_store_filename, "rb+", &myfile);
    if(err_do_open != ERR_NONE) {
        do_close(&myfile);
        return err_do_open;
    }

    char* buffer = NULL;
    uint32_t img_size = 0;

    int err_do_read = do_read(imgID, res, &buffer, &img_size, &myfile);
    if(err_do_read != ERR_NONE) {
        do_close(&myfile);
        return err_do_read;
    }

    int err_write = write_disk_image(imgID, res, buffer, img_size);
    if(err_write != ERR_NONE) {
        free(buffer);
        do_close(&myfile);
        fprintf(stderr, "Error: can't write image on the disk");
        return err_write;
    }

    free(buffer);
    do_close(&myfile);

    return ERR_NONE;
}

/********************************************************************//**
 * clean images' data base
********************************************************************** */
int do_gc_cmd(int argc, char* argv[])
{
    if(argv == NULL) return ERR_INVALID_ARGUMENT;
    if(argc < EXPECTED_NB_ARGS_GC) return ERR_NOT_ENOUGH_ARGUMENTS;
    if(argc > EXPECTED_NB_ARGS_GC) return ERR_INVALID_ARGUMENT;

    const char* img_store_filename = argv++[0]; --argc;
    if(strlen(img_store_filename) == 0 || strlen(img_store_filename) > MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }
    const char* tmp_img_store_filename = argv++[0]; --argc;
    if(strlen(tmp_img_store_filename) == 0 || strlen(tmp_img_store_filename) > MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }

    return do_gbcollect(img_store_filename, tmp_img_store_filename);
}

/********************************************************************//**
 * Create Command Mappings
 */
struct command_mapping* set_cmd_mappings()
{
    struct command_mapping* mappings = calloc(NB_OF_COMMANDS, sizeof(struct command_mapping));
    mappings[0] = (struct command_mapping) {
        "list", do_list_cmd
    };
    mappings[1] = (struct command_mapping) {
        "create", do_create_cmd
    };
    mappings[2] = (struct command_mapping) {
        "help", help
    };
    mappings[3] = (struct command_mapping) {
        "delete", do_delete_cmd
    };
    mappings[4] = (struct command_mapping) {
        "read", do_read_cmd
    };
    mappings[5] = (struct command_mapping) {
        "insert", do_insert_cmd
    };
    mappings[6] = (struct command_mapping) {
        "gc", do_gc_cmd
    };
    return mappings;
}

/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
        return ERR_IMGLIB;
    }
    int ret = ERR_NONE;
    bool find_cmd = false;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        argc--;
        argv++; // skips command call program name : imgStoreMgr
        struct command_mapping* mappings = set_cmd_mappings();
        int i = 0;
        while(i < NB_OF_COMMANDS && !find_cmd) {
            if (!strcmp(mappings[i].command_name, argv[0])) {
                find_cmd = true;
                ++argv; --argc;
                ret = mappings[i].command(argc, argv);
            }
            ++i;
        }
        if (!find_cmd) {
            ret = ERR_INVALID_COMMAND;
        }
        free(mappings);
    }

    vips_shutdown();

    if (ret != ERR_NONE) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help(0, NULL);
    }
    return ret;
}



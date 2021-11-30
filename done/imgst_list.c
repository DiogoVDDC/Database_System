/**
 * @file imgst_list.c
 */

#include "imgStore.h"
#include "error.h"
#include <stdbool.h>
#include <json-c/json.h>

#define FORMAT_ERR_STR "unimplemented do_list output mode"

/**
 * helper method to do_list in case of stdout format (print infos on the standard output stream)
 */
char* do_list_stdout(const struct imgst_file* imgst_file)
{
    if(imgst_file == NULL || imgst_file->metadata == NULL) return NULL;

    print_header(&imgst_file->header);

    //to inform user that the imgStore is empty
    if (imgst_file->header.num_files == 0) {
        printf("<< empty imgStore >>\n");
    } else {
        //print metadata of all images (only valid ones)
        for (uint32_t i = 0; i < imgst_file->header.max_files; ++i) {
            if (imgst_file->metadata[i].is_valid != EMPTY) {
                print_metadata(&imgst_file->metadata[i]);
            }
        }
    }
    return NULL; //always return NULL
}

/**
 * helper method to do_list in case of json format
 */
char* do_list_json(const struct imgst_file* imgst_file)
{
    if (imgst_file == NULL || imgst_file->metadata == NULL ) {
        return NULL;
    }

    struct json_object* array = json_object_new_array();
    for(size_t i = 0; i < imgst_file->header.max_files; ++i) {
        if(imgst_file->metadata[i].is_valid) {
            struct json_object*  img_id = json_object_new_string(imgst_file->metadata[i].img_id);
            json_object_array_add(array, img_id);
        }
    }
    struct json_object* top_level = json_object_new_object();
    if(json_object_object_add(top_level, "Images", array) != 0) return "ERROR: impossible to build json_object from database";

    //can't simply return the string, will become garbage after top_level object will have been
    //collected as soon as we quit this function
    size_t len = strlen(json_object_to_json_string(top_level));

    char* output_string = calloc(1, len + 1);

    strncpy(output_string, json_object_to_json_string(top_level), len);
    return output_string;
}

/** @copybrief */
char* do_list(const struct imgst_file* imgst_file, enum do_list_mode format)
{
    if(imgst_file == NULL) return NULL;

    char* s = NULL;

    switch(format) {
    case STDOUT: s =  do_list_stdout(imgst_file); break;
    case JSON  : s =  do_list_json(imgst_file);   break;
    default    : {
        size_t len = strlen(FORMAT_ERR_STR);
        s = calloc(1, len + 1);
        if (s == NULL) return NULL;
        strncpy(s, FORMAT_ERR_STR, len);
        s[len] = '\0';
    }
    }
    return s;
}


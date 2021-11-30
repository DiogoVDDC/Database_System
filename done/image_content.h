#pragma once
#ifndef DONE_IMAGE_CONTENT_H
#define DONE_IMAGE_CONTENT_H

#endif //DONE_IMAGE_CONTENT_H

#include <stdint.h> // for uint32_t, uint64_t
#include <stddef.h>
#include "imgStore.h"

/**
 * @brief: Add image of the res specified to the database if not already present
 *
 * @param res of the image we want to create
 * @param imgstFile structure for header, metadata
 * @param index of image we have to modify
 * @return Some error code. 0 if no error.
 */
int lazily_resize(uint16_t res, struct imgst_file* imgstFile, uint32_t index);

/**
 * @brief stores the height and width of the image
 *
 * @param height output of the height stored at pointer height
 * @param width output of width stored at pointer width
 * @param image_buffer where the image in stored
 * @param image_size is the size in bytes of the image
 * @return
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer,size_t image_size);
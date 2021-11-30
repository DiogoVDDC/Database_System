#include <vips/vips.h>
#include <stdint.h> // for uint32_t, uint64_t
#include <stdlib.h>
#include "image_content.h"
#include "imgStore.h"

//position of img in the image_array
#define INDEX_ORIG_IMG 0
#define INDEX_RESIZED_IMG 1



/**
 * @param imgstFile structure for header, metadata and file pointer.
 * @param resolution resolution of image selected
 * @param origin_size size of the original picture
 * @return the shirk factor of the image
 */
double shrink_value(const VipsImage *image, struct imgst_file* imgstFile, uint16_t res)
{
    int max_width  = imgstFile->header.res_resized[2*res + X_COORD_LOCATION];
    int max_height = imgstFile->header.res_resized[2*res + Y_COORD_LOCATION];

    const double h_shrink = (double) max_width  / (double) image->Xsize ;
    const double v_shrink = (double) max_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

//----------------------------------------------------------------------------------------------------------
/**
 *
 * @param res resolution of picture interested
 * @param imgstFile structure for header, metadata and file pointer.
 * @param index at which we want to check the arguments
 * @return Some error code. 0 if no error.
 */
int check_arg_resize(uint16_t res, struct imgst_file* imgstFile, size_t index)
{
    if(res != RES_THUMB && res != RES_SMALL && res != RES_ORIG) {
        fprintf(stderr, "res not corresponding to RES_SMALL: %d or RES_THUMB: %d \n", RES_SMALL, RES_THUMB);
        return ERR_INVALID_ARGUMENT;
    }
    if(imgstFile == NULL) {
        fprintf(stderr, "Error: lazily_resize received NULL pointer as pointer of imgstFile");
        return ERR_INVALID_ARGUMENT;
    }
    if(index >= imgstFile->header.max_files || !imgstFile->metadata[index].is_valid) {
        fprintf(stderr, "invalid index");
        return ERR_INVALID_ARGUMENT;
    }

    //move file pointer to the specified image
    if (fseek(imgstFile->file, imgstFile->metadata[index].offset[res], SEEK_SET) != ERR_NONE) {
        fprintf(stderr, "Error during fseek to correct metadata location");
        return ERR_IO;
    }
    return ERR_NONE;
}

/**
 * helper method to unref a VIPS_OBJECT to_unref and/or free a ptr and set it to NULL after
 * @param to_unref
 * @param to_free
 */
void free_and_unref(void* to_unref, void* to_free)
{
    if(to_unref != NULL) {
        g_object_unref(to_unref);
    }
    if(to_free != NULL) {
        free(to_free);
        to_free = NULL;
    }
}

/**
 * helper function to load, resize an image with VIPS, and then write it into the database
 * @param res
 * @param imgstFile
 * @param index
 * @param img_size_out
 * @return error code as defined in error.h
 */
int load_resize_image(uint16_t res, struct imgst_file* imgstFile, uint32_t index, size_t* img_size_out)
{

    //res of the original image
    uint32_t size_origin_img = imgstFile->metadata[index].size[RES_ORIG];
    int const nb_image_to_resize = 1;

    //allocate memory to be able to store image at image pointer
    void* img_buffer = calloc(1, size_origin_img);
    if(img_buffer == NULL) {
        fprintf(stderr, "Error while allocate space in memory for an size_image_in");
        return ERR_OUT_OF_MEMORY;
    }

    //read file at position specified by metadata and store it at allocated memory location pointed by orig_img
    fseek(imgstFile->file, imgstFile->metadata[index].offset[RES_ORIG], SEEK_SET);
    if(fread(img_buffer, size_origin_img, nb_image_to_resize, imgstFile->file) != nb_image_to_resize) {
        fprintf(stderr, "Error: while loading metadata");
        free_and_unref(NULL, img_buffer);
        return ERR_IO;
    }

    VipsObject* parent = VIPS_OBJECT(vips_image_new());
    //at index 0 store origin image and at 1 the resized one
    VipsImage** image_array = (VipsImage**) vips_object_local_array(parent, 2*nb_image_to_resize);
    //will be unref when its parent will be so

    if(vips_jpegload_buffer(img_buffer, size_origin_img, &image_array[INDEX_ORIG_IMG], (char*) NULL) != ERR_NONE) {
        fprintf(stderr, "Error: while loading image to the buffer");
        free_and_unref(NULL, img_buffer);
        return ERR_IMGLIB;
    }

    //END IMAGE LOADING ---------------------------------------------------------------------------------------

    double const ratio = shrink_value(image_array[INDEX_ORIG_IMG], imgstFile, res);

    if(vips_resize(image_array[INDEX_ORIG_IMG], &image_array[INDEX_RESIZED_IMG], ratio, NULL) != ERR_NONE) {
        fprintf(stderr, "Error: while vips was resizing the image");
        free_and_unref(parent, img_buffer);
        return ERR_IMGLIB;
    }

    //END IMAGE RESIZING ----------------------------------------------------------------------------------


    if(vips_jpegsave_buffer(image_array[INDEX_RESIZED_IMG], &img_buffer, img_size_out, (char*) NULL) != ERR_NONE) {
        fprintf(stderr, "Error: while saving image to the buffer");
        free_and_unref(parent, img_buffer);
        free_and_unref(NULL, image_array);
        return ERR_IMGLIB;
    };
    //don't need image_array & image_ptr_in anymore

    if(img_buffer == NULL) {
        fprintf(stderr, "Error while allocate space in memory for an size_image_out");
        free_and_unref(parent, img_buffer);
        return ERR_OUT_OF_MEMORY;
    }

    //set the writing pointer to the end of the file
    if (fseek(imgstFile->file, NO_OFFSET, SEEK_END) != ERR_NONE) {
        fprintf(stderr, "Error: can't set head reader at the end of the file");
        free_and_unref(parent, img_buffer);
        return ERR_IO;
    }

    //write new resized image to end of file
    if(fwrite(img_buffer, *img_size_out, nb_image_to_resize, imgstFile->file) != nb_image_to_resize) {
        fprintf(stderr, "ERROR: can't write resized");
        free_and_unref(parent, img_buffer);
        return ERR_IO;
    }

    //don't need image_ptr_out anymore
    free_and_unref(parent, img_buffer);

    return ERR_NONE;
}

//----------------------------------------------------------------------------------------------------------
/**
 * @copybref
 */
int lazily_resize(uint16_t res, struct imgst_file* imgstFile, uint32_t index)
{
    int err_check_arg_resize = check_arg_resize(res, imgstFile, index);
    if(err_check_arg_resize != ERR_NONE) {
        return err_check_arg_resize;
    }
    // if image already exist in this resolution do nothing
    if(res == RES_ORIG || imgstFile->metadata[index].size[res] != 0) {
        return ERR_NONE;
    }

    size_t size_image_out = 0;
    //loads the image and resizes it;
    int err_load_resize = load_resize_image(res, imgstFile, index, &size_image_out);
    if(err_load_resize != ERR_NONE) {
        return err_load_resize;
    }

    //update metadata
    imgstFile->metadata[index].offset[res] = ftell(imgstFile->file) - size_image_out;
    imgstFile->metadata[index].size[res] = size_image_out;

    //write updated metadata into the file
    return write_metadata(imgstFile, index);
}

/**
 * @copybref
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size)
{
    if(image_buffer == NULL || image_size == 0 || height == NULL || width == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    VipsImage* image = (VipsImage*) VIPS_OBJECT(vips_image_new());

    if(vips_jpegload_buffer ((void *)image_buffer,  image_size, &image, (char*) NULL)!= ERR_NONE) {
        fprintf(stderr, "%s", vips_error_buffer());
        g_object_unref(image);
        return ERR_IMGLIB;
    }

    *height = image->Ysize;
    *width  = image->Xsize;
    g_object_unref(image);
    return ERR_NONE;

}
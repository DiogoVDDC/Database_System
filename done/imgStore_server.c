#include <stdlib.h>
#include <stdio.h>
#include <vips/vips.h>
#include <signal.h>
#include "mongoose.h"
#include "imgStore.h"
#include "util.h"

#define POLLING_TIME_MS 1000 //each poll takes 1000 ms = 1 sec
#define EXPECTED_NB_ARGS_MAIN 2
#define FOUND_HTTP_CODE 302
#define ERROR_HTTP_CODE 500
#define OFFSET_SIZE 40
#define MAX_RES_LEN 10

static const char*  LISTENING_ADDR = "http://localhost:8000";
static const char* WEB_DIRECTORY = ".";
static const char* TMP_DIRECTORY = "/tmp";




/**
 * default_val = 0
 * interrupt_val = SIG_INT
 * termination_req_val = SIGTERM
 * we are not using any other signal_type
 */
static int terminal_signal = 0;

/**
 * method reply with html error, and specific error message
 */
void mg_error_msg(struct mg_connection* connection, int error)
{
    mg_http_reply(connection, ERROR_HTTP_CODE, "", "Error: %s \n", ERR_MESSAGES[error]);
}


/**
 * handle terminal's signals and update the terminal_signal variable which
 * is used to check if the program is asked to stop
 */
static void terminal_signals_handler(int s)
{
    terminal_signal = s;
}

//-------------------------------------------------------------------------------
/**
 * do the the do_list request
 */
static void handle_list_call(struct imgst_file* imgstFile, struct mg_connection* connection)
{
    //create json
    char* json_str = do_list(imgstFile, JSON);

    //http respond with content as json
    mg_printf(connection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %zu\r\n\r\n%s",
              strlen(json_str), json_str);

}


static void handle_delete_call(struct imgst_file* imgstFile, struct mg_http_message* hm,  struct mg_connection* connection)
{
    char img_id[MAX_IMG_ID];

    //get img_id with
    int err_img_id_uri = mg_http_get_var(&hm->query, "img_id", img_id, MAX_IMG_ID);
    if(err_img_id_uri <= 0) {
        mg_error_msg(connection, ERR_INVALID_ARGUMENT);
        return;
    }

    //delete image from database
    int err_do_delete = do_delete(img_id, imgstFile);
    if(err_do_delete != ERR_NONE) {
        mg_error_msg(connection, err_do_delete);
        return;
    }

    //respond with index.html
    mg_printf(connection,
              "HTTP/1.1 302 Found\r\n"
              "Location: %s/index.html\r\n", LISTENING_ADDR);

}



static void handle_read_call(struct imgst_file* imgstFile, struct mg_http_message* hm, struct mg_connection* connection)
{

    char res_char[MAX_RES_LEN];
    char img_id[MAX_IMG_ID];

    //read res variable from url
    int err_res_uri = mg_http_get_var(&hm->query, "res", res_char, MAX_RES_LEN);
    if(err_res_uri <= 0) {
        mg_error_msg(connection, ERR_INVALID_ARGUMENT);
        return;
    }

    //read img_id variable from url
    int err_img_id_uri = mg_http_get_var(&hm->query, "img_id", img_id, MAX_IMG_ID);
    if(err_img_id_uri <= 0) {
        mg_error_msg(connection, ERR_INVALID_ARGUMENT);
        return;
    }

    //get res index from the res name
    int res = resolution_atoi(res_char);
    if(res == -1) {
        mg_error_msg(connection, ERR_RESOLUTIONS);
        return;
    }

    char* img_buffer = NULL;
    u_int32_t img_size = 0;

    //read image with img_id and store it in img_buffer
    int err_do_read = do_read(img_id, res, &img_buffer, &img_size, imgstFile);
    if(err_do_read != ERR_NONE) {
        mg_error_msg(connection, err_do_read);
        return;
    }

    // reply with image jpeg html
    mg_printf(connection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: image/jpeg\r\n"
              "Content-Length: %zu\r\n\r\n",
              img_size);

    //send the actual image
    mg_send(connection, img_buffer, img_size);

    free(img_buffer);
}

static void handle_insert_call(struct imgst_file* imgstFile, struct mg_http_message* hm, struct mg_connection* connection)
{
    //upload to /tmp until the body of the http_message is empty
    if(hm->body.len != 0) {
        int err = mg_http_upload(connection, hm, "/tmp");
        if(err < 0) {
            mg_error_msg(connection, ERR_FILE_NOT_FOUND);
            return;
        }
    } else { //get img_id from uri
        char img_id[MAX_IMG_ID];
        int err_img_id_uri = mg_http_get_var(&hm->query, "name", img_id, MAX_IMG_ID);
        if(err_img_id_uri <= 0) {
            mg_error_msg(connection, ERR_INVALID_IMGID);
            return;
        }

        //get the offset from the uri
        char* offset = calloc(1, OFFSET_SIZE);
        int err_img_size_uri =  mg_http_get_var(&hm->query, "offset", offset, OFFSET_SIZE);
        if(err_img_size_uri <= 0) {
            mg_http_reply(connection, ERROR_HTTP_CODE, "", "Not found\n");
            return;
        }

        //converts the offset char* to an unsigned int
        u_int32_t img_size = atouint32(offset);

        //allocate memory for the filepath
        char* filepath = calloc(strlen(TMP_DIRECTORY) + MAX_IMG_ID + 1, sizeof(char));
        if(filepath == NULL) {
            mg_error_msg(connection, ERR_OUT_OF_MEMORY);
            return;
        }

        // build the filepath tmp/img_id
        strcpy(filepath,  TMP_DIRECTORY);
        strcat(filepath, "/");
        strcat(filepath, img_id);

        //open the image stored in tmp
        FILE* fp = fopen(filepath,"r");
        if(fp == NULL) {
            mg_error_msg(connection, ERR_FILE_NOT_FOUND);
            return;
        }
        char* buffer = calloc(1, img_size);
        if(buffer == NULL) {
            mg_error_msg(connection, ERR_OUT_OF_MEMORY);
            return;
        }

        // read image in allocated buffer
        fread(buffer, img_size, 1, fp);
        fclose(fp);

        //insert image in database
        int err_do_insert = do_insert(buffer, img_size, img_id, imgstFile);
        if(err_do_insert != ERR_NONE) {
            mg_error_msg(connection, err_do_insert);
            return;
        }

        //reply with the index.html page
        mg_printf(connection,
                  "HTTP/1.1 302 Found\r\n"
                  "Location: %s/index.html\r\n", LISTENING_ADDR);
        free(buffer);
    }
}


/**
 * handle the different URI
 * @param connection
 * @param ev
 * @param ev_data
 * @param data
 */
static void imgst_event_handler(struct mg_connection* connection, int ev, void *ev_data, void *data)
{
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message *) ev_data;
        struct imgst_file* imgstFile = (struct imgst_file *) data;

        //switch between the handlers for the different url
        if (mg_http_match_uri(hm, "/imgStore/list")) {
            handle_list_call(data, connection);
            connection->is_draining = 1;
        } else if(mg_http_match_uri(hm, "/imgStore/read")) {
            handle_read_call(data, hm, connection);
            connection->is_draining = 1;
        } else if(mg_http_match_uri(hm, "/imgStore/delete")) {
            handle_delete_call(data, hm, connection);
            connection->is_draining = 1;
        } else if(mg_http_match_uri(hm, "/imgStore/insert")) {
            handle_insert_call(data, hm, connection);
            connection->is_draining = 1;
        } else {
            //replies with static content
            struct mg_http_serve_opts opts = {.root_dir = WEB_DIRECTORY};
            mg_http_serve_dir(connection, ev_data, &opts);
        }
    }
}


//-------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if(argc < EXPECTED_NB_ARGS_MAIN) {
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_NOT_ENOUGH_ARGUMENTS]);
        return EXIT_FAILURE;
    }
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
        return ERR_IMGLIB;
    }

    /* Open the imgStore */
    const char* imgst_filename = argv[1];
    struct imgst_file imgstFile = {0};
    int ret = do_open(imgst_filename, "rb+", &imgstFile);
    if(ret != ERR_NONE) {
        fprintf(stderr, "Error: %s\n", ERR_MESSAGES[ret]);
        return EXIT_FAILURE;
    }

    /* Create server */
    struct mg_mgr mgr; //event manager
    mg_mgr_init(&mgr);
    if(mg_http_listen(&mgr, LISTENING_ADDR, imgst_event_handler, &imgstFile) == NULL) {
        fprintf(stderr, "Error starting server on address %s\n", LISTENING_ADDR);
        return EXIT_SUCCESS;
    }

    printf("Starting imgStore server on %s\n", LISTENING_ADDR);
    print_header(&imgstFile.header);

    /* infinite event loop */
    signal(SIGINT, terminal_signals_handler); //accept interrupts signals as CTRL+C
    signal(SIGTERM, terminal_signals_handler); // accept termination requests
    //while don't receive an SIGINT or a SIGTERM from the terminal, loop (other signals are ignored)
    while (terminal_signal == 0) mg_mgr_poll(&mgr, POLLING_TIME_MS);

    /* Cleanup */
    do_close(&imgstFile);
    mg_mgr_free(&mgr);
    vips_shutdown();

    return EXIT_SUCCESS;
}
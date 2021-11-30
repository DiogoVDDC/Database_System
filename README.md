## Database Management System (DMS):

Done by Diogo Valdivieso Damasio Da Costa and Alexis Favre

The user can interact with the Database Management system in two ways:
 1)  With the command line
 2)  With the web interface

In this project we refer to img_store as the file which stores all the pictures and metadata.

### Command Line Interface:

The command line interface to DMS is through the executable imgStoreMgr.

You can find the command interface below:

![alt text](https://github.com/DiogoVDDC/Database_System/blob/main/Screenshot%20from%202021-11-30%2014-42-07.png)

Here is an example on how to create and img_store:

![alt text](https://github.com/DiogoVDDC/Database_System/blob/main/create_img_store.png)

Here is an example on how to add an image and list the content of the img_store:

![alt text](https://github.com/DiogoVDDC/Database_System/blob/main/add_list_img_store.png)


### Web Interface:

To lauch the webserver an img_store store is needed as argument.

Here is a video example of how one might use both the web interface and command interface to use operate the DMS.

https://user-images.githubusercontent.com/56833126/144067057-2ffb6c35-28dd-4314-a18e-03a8bd1fedef.mp4


### Overview of the design architecture:

struct imgst_header is the decription of the img_store, for example how many image it can store, the size of those images ...

struct img_metadata describes the metadata of each image in the database. Particularly important is the offset which allows to find the image in the img_store file.

struct imgst_file represents the img_store file.

Images are stored in different sizes, if a size that is not present in the database is requested, the image is created and added to the database. 

The garbage collector is responsible for removing the images in the file, since the delete command only changes the valid bit to 0. 
The garbage collection is not done in place an requires a temporary file. The image with valid different to 1 aren't copied to the temp file. The temp file is then copied to the img_store file and deleted


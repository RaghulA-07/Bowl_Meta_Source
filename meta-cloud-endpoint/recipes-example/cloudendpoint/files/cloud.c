
/*
 * @file    cloud.c
 * @author  Nestle Firmware Team
 * @version V1.0.0
 * @date
 * @brief   Source file for Cloud upload interface
 */

/* Standard Includes --------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <time.h>
#include <errno.h>

/* Project Includes ---------------------------------------------------------*/

#include "cloud.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

#define MAX_MESSAGE_SIZE        100
#define CHECK_INTERVAL          5                      // Check every 5 seconds
#define PROCESSED_FILES         "processed_files.inc"  // File to store processed file names
#define AZURE_STORAGE_ACCOUNT   "namsprdigsoluseasta"
#define AZURE_CONTAINER_NAME    "pet-datalake"
#define AZURE_SAS_TOKEN         "sp=r&st=2024-06-24T06:45:15Z&se=2024-08-31T14:45:15Z&spr=https&sv=2022-11-02&sr=c&sig=hJCm06E7aadDCl6320quoT7rqUYNuIXO2rkTnFSJzxI%3D"
#define AZURE_BLOB_NAME         "landing/dog_bowl_test"

/* Private macro ------------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

struct message
{
    long message_type;
    uint8_t message_text[MAX_MESSAGE_SIZE];
};


int    msgid;
key_t  cloudkey;
struct message cloudhook_msg;
uint8_t   is_fileopen;
uint8_t   Harvestmode;

const char *folder_path ;
DIR *dir;
struct dirent *ent;

static int session_active = 0;

/* Private function prototypes -----------------------------------------------*/

void process_ipc_queue_reception(void);
void upload_file(const char *folder_path, const char *file_name);
void record_processed_file(const char *file_name);
int is_file_complete(const char *folder_path, const char *file_name);
int file_already_processed(const char *file_name);



/*******************************************************************************
 * @name   main
 * @brief  Function handles the message queue and upload part
 * @param
 * @retval
 ********************************************************************************/

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "\n CLOUD_HOOK_INFO: Usage: %s <folder_path>\n", argv[0]);
        return 1;
    }

    printf("\n CLOUD_HOOK_INFO: Starting the cloudendpoint application...");

    /*File path need to be hard code here .......... */
    folder_path = argv[1];


    // unique key  
    cloudkey = CLOUD_IPC_MSG_KEY;

    //Message queue handling
     msgid = msgget(cloudkey, 0666 | IPC_CREAT);


       if ((dir = opendir(folder_path)) != NULL)
        {
          is_fileopen=1;
          printf("\n CLOUD_HOOK_INFO: folder is open.......");
        }
        else
        {
            perror("\n CLOUD_HOOK_INFO: Error .....Could not open directory or File for writing the Sensor harvest data ");
            // return EXIT_FAILURE;
            is_fileopen=0;
        }

        Harvestmode = HARVEST_STOP;

    while (1)
    {

        process_ipc_queue_reception();



        if (is_fileopen==1)
        {

            switch(Harvestmode)
            {
            case HARVEST_RUN:

                // if(session_active==1)
                //  {
                //     /*Check file is empty or not */
                //     if ((ent = readdir(dir)) != NULL)
                //     {
                //         if (ent->d_type == DT_REG)
                //         {
                //             // Check if it's a regular file
                //             if (!file_already_processed(ent->d_name) &&
                //                 (strstr(ent->d_name, ".mp4") != NULL || strstr(ent->d_name, ".jpg") != NULL || strstr(ent->d_name, ".jpeg") != NULL || strstr(ent->d_name, ".txt") != NULL) &&
                //                 is_file_complete(folder_path, ent->d_name))
                //             {
                //                 upload_file(folder_path, ent->d_name);
                //                 record_processed_file(ent->d_name);
                //             }
                //         }
                //     }
                //  }

            break;
            case HARVEST_STOP_INITATED:

                if(session_active==0)
                 {
                    printf("\n CLOUD_HOOK_INFO: Harvest stop initiated, checking remaining files upload status.......");
                    while ((ent = readdir(dir)) != NULL)
                    {
                        printf("\n%s", ent->d_name); 
                        if (ent->d_type == DT_REG)
                        { // Check if it's a regular file
                            if (!file_already_processed(ent->d_name) &&
                                (strstr(ent->d_name, ".mp4") != NULL || strstr(ent->d_name, ".jpg") != NULL || strstr(ent->d_name, ".jpeg") != NULL || strstr(ent->d_name, ".txt") != NULL) &&
                                is_file_complete(folder_path, ent->d_name))
                            {
                                upload_file(folder_path, ent->d_name);
                                record_processed_file(ent->d_name);
                            }
                        }
                    }
                    closedir(dir);
                    printf("\n CLOUD_HOOK_INFO:Harvest sensor file upload done.......");
                    Harvestmode = HARVEST_STOP;
                }
            break;
            case HARVEST_STOP:
             //Do nothing wait for harvest command 

              //sleep(CHECK_INTERVAL); // Wait for CHECK_INTERVAL seconds
            break;

            default:
            break;

            }

        }   

    }

    return EXIT_SUCCESS;
}


/*******************************************************************************
 * @name   process_ipc_queue_reception
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void process_ipc_queue_reception(void)
{

        // Clearing the msg
         memset(&cloudhook_msg.message_text, 0, sizeof(cloudhook_msg.message_text));
    
        // reads a message from the queue associated with the message queue identifier specified by msgid
        if (msgrcv(msgid, &cloudhook_msg, sizeof(cloudhook_msg), 1, IPC_NOWAIT) == -1)
        {
            if (errno == ENOMSG)
            {
                // LPRINTF("\n CLOUD_HOOK_INFO:IPC No message available");
            }
            else
            {
                LPRINTF("\n CLOUD_HOOK_INFO: IPC Error");
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // LPRINTF("\n CLOUD_HOOK_INFO: Received Data from BLE HOOK = %s", cloudhook_msg.message_text);

            uint8_t command_id = cloudhook_msg.message_text[0];
            uint8_t *data = (uint8_t *)cloudhook_msg.message_text + 1;

            switch (command_id)
            {

            case HARVEST_ENABLE_DISABLE_CMD: /* Start & stop harvesting */
                
                if (data[0] == 1)
                {
                    session_active = 1;   
                    Harvestmode=HARVEST_RUN;         
                    printf("\n CLOUD_HOOK_INFO:Session Start Command received from Harvest main Hook");
                }
                else if (data[0] == 0)
                {
                    session_active = 0;    
                    Harvestmode=HARVEST_STOP_INITATED;       
                    printf("\n CLOUD_HOOK_INFO:Session Stop Command received from Harvest main Hook");
                
                }
                break;


            case A55_SHUTDOWN_CMD: /* A55 Core shutdown*/   

            printf("\n CLOUD_HOOK_INFO:Shutdown Command received from Harvest main Hook");    

                break;

            /* Future commands need to be added here ..................*/

            default:
                // printf("\n RPMSG_INFO:invalid Command received from BLE & sending to M33");
                break;
            }
        }
}
/*******************************************************************************
 * @name   upload_file
 * @brief  Function upload files to cloud
 * @param
 * @retval
 ********************************************************************************/
void upload_file(const char *folder_path, const char *file_name)
{
    // Determine the Content-Type based on the file extension
    const char *content_type = NULL;
    if (strstr(file_name, ".mp4") != NULL)
    {
        content_type = "Content-Type: video/mp4";
    }
    else if (strstr(file_name, ".txt") != NULL)
    {
        content_type = "Content-Type: text/plain";
    }
    else if (strstr(file_name, ".jpg") != NULL || strstr(file_name, ".jpeg") != NULL)
    {
        content_type = "Content-Type: image/jpeg";
    }
    else
    {
        fprintf(stderr, "\n CLOUD_HOOK_INFO: Unsupported file type\n");
        return;
    }

    char full_file_path[1024];
    snprintf(full_file_path, sizeof(full_file_path), "%s/%s", folder_path, file_name);

    FILE *file = fopen(full_file_path, "rb");
    if (!file)
    {
        perror("\n CLOUD_HOOK_INFO: Error opening file");
        return;
    }

    // Calculate file size
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    printf("\n CLOUD_HOOK_INFO: File size = %ld", file_size);
    rewind(file);

    // Initialize libcurl
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "\n CLOUD_HOOK_INFO: Error initializing libcurl");
        fclose(file);
        return;
    }

    printf("\n\n CLOUD_HOOK_INFO: Setting up the cloud endpoint URL...");

    // Added by Raghul
    char url[256];
    snprintf(url, sizeof(url), "https://%s.blob.core.windows.net/%s/%s/%s?%s", AZURE_STORAGE_ACCOUNT, AZURE_CONTAINER_NAME, AZURE_BLOB_NAME, file_name, AZURE_SAS_TOKEN);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    printf("\n\n CLOUD_HOOK_INFO: Cloud endpoint URL = %s\n\n", url);

    // Tell libcurl that we want to upload
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, file);

    // Setting the upload file size
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_size);

    // Set custom headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "x-ms-blob-type: BlockBlob");
    headers = curl_slist_append(headers, content_type);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Enable verbose mode to dump HTTP request and response information
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // Perform the file upload
    CURLcode res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK)
    {
        fprintf(stderr, "\n CLOUD_HOOK_INFO: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else
    {
        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200 || response_code == 201)
        {
            printf("\n CLOUD_HOOK_INFO: Upload successful (HTTP response code %ld)\n", response_code);
        }
        else
        {
            printf("\n CLOUD_HOOK_INFO: Upload failed with HTTP response code %ld\n", response_code);
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    fclose(file);
}

/*******************************************************************************
 * @name   file_already_processed
 * @brief  Function checks the file is already uploaded to cloud
 * @param
 * @retval
 ********************************************************************************/

int file_already_processed(const char *file_name)
{
    FILE *file = fopen(PROCESSED_FILES, "r");

    // file is not already processed
    if (!file)
    {
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        if (strcmp(line, file_name) == 0)
        {
            fclose(file);
            printf("\n CLOUD_HOOK_INFO: %s - file is already processed", file_name);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

/*******************************************************************************
 * @name   record_processed_file
 * @brief  Function records the uploaded file into another file
 * @param
 * @retval
 ********************************************************************************/

void record_processed_file(const char *file_name)
{
    FILE *file = fopen(PROCESSED_FILES, "a");
    if (file)
    {
        printf("\n CLOUD_HOOK_INFO: %s - file is processed and recorded", file_name);
        // fprintf(file, "%s\n", file_name);
        fclose(file);
    }
}

/*******************************************************************************
 * @name   is_file_complete
 * @brief  Function checks the file upload is completed
 * @param
 * @retval
 ********************************************************************************/

int is_file_complete(const char *folder_path, const char *file_name)
{
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, file_name);

    struct stat file_stat;
    int result = stat(file_path, &file_stat);

    if (result != 0)
    {
        printf("\n CLOUD_HOOK_INFO: is_file_complete = NO");
        return 0; // Error getting file info
    } else {
        printf("\n CLOUD_HOOK_INFO: is_file_complete = YES");
    }

    time_t now;
    time(&now);
    double seconds = difftime(now, file_stat.st_mtime);

    return seconds > CHECK_INTERVAL; // File is considered complete if it was last modified longer ago than CHECK_INTERVAL
}

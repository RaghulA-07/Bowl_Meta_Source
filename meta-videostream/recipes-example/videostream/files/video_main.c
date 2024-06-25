
/*
 * @file    video_main.c
 * @author  Nestle Firmware Team
 * @version V1.0.0
 * @date
 * @brief   Source file for Video harvesting interface
 */

/* Standard Includes --------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

/* Project Includes ---------------------------------------------------------*/

#include "video_main.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

#define MAX_MESSAGE_SIZE 100

/* Private macro ------------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

struct message
{
    long message_type;
    uint8_t message_text[MAX_MESSAGE_SIZE];
};

key_t key;
int msgid;
struct message videohook_msg;

int status;

/* Private function prototypes -----------------------------------------------*/

/*******************************************************************************
 * @name   main
 * @brief  Function handles the  communication
 * @param
 * @retval
 ********************************************************************************/

int main()
{
    LPRINTF("\n VIDEO_HOOK: Started");

    const char *script_name = "./videostream.sh 0 D10 &";

    // const char *kill_command = "kill $(ps | grep 'gst-launch-1.0' | awk '{print $2}')";

    // const char *kill_command = "kill -15 $(echo $(pidof gst-launch-1.0))";

    const char *kill_command = "kill -SIGINT $(echo $(pidof gst-launch-1.0))";

    // Command to terminate the script using pkill
    // char command[256];
    // snprintf(command, sizeof(command), "kill $(ps aux | grep 'gst-launch-1.0' | awk '{print $2}')");

    // Generate unique key
    // key = ftok("VIDEO_HOOK", 12);
    key = 2945;

    // Create message queue and return id
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1)
    {
        perror("VIDEO_HOOK: Video HOOK msgget failed");
        LPRINTF("\n VIDEO_HOOK: Video HOOK msgget failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {

        // Clearing the msg
        memset(&videohook_msg.message_text, 0, sizeof(videohook_msg.message_text));
        // Receive message
        if (msgrcv(msgid, &videohook_msg, sizeof(videohook_msg.message_text), 1, IPC_NOWAIT) == -1)
        {
            if (errno == ENOMSG)
            {
                // LPRINTF("\n CLOUD_HOOK_INFO:IPC No message available");
            }
            else
            {
                LPRINTF("\n VIDEO_HOOK: IPC Error");
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        else
        {

            uint8_t command_id = videohook_msg.message_text[0];
            uint8_t *data = (uint8_t *)videohook_msg.message_text + 1;

            switch (command_id)
            {

            case HARVEST_ENABLE_DISABLE_CMD: /* Start & stop harvesting */

                if (data[0] == 1)
                {
                    printf("\n VIDEO_HOOK:Session Start Command received from Harvest main Hook");

                    status = system(script_name);

                    if (status == -1)
                    {
                        // system() returns -1 on error
                        perror("Error running script");
                    }
                    else
                    {
                        // Print the exit status of the script
                        printf("\n VIDEO_HOOK: Script exited with status = %d", status);
                    }
                }
                else if (data[0] == 0)
                {
                    printf("\n VIDEO_HOOK: Session Stop Command received from Harvest main Hook");

                    status = system(kill_command);

                    if (status == -1)
                    {
                        perror("\n VIDEO_HOOK: Error terminating script");
                        return 1;
                    }

                    printf("\n VIDEO_HOOK: Script '%s' terminated.\n", script_name);
                }
                break;

            case A55_SHUTDOWN_CMD: /* A55 Core shutdown*/

                printf("\n VIDEO_HOOK: Shutdown Command received from Harvest main Hook");

                break;

            default:
                // printf("\n RPMSG_INFO:invalid Command received from BLE & sending to M33");
                break;
            }
        }
    }

    LPRINTF("\n VIDEO_HOOK: Ended");

    return 0;
}
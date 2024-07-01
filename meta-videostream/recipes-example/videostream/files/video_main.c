
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

uint64_t  current_session_uid;

/* Private function prototypes -----------------------------------------------*/

static void bytes_to_uint64(const uint8_t *data, uint64_t *ertc);


/*******************************************************************************
 * @name   main
 * @brief  Function handles the  communication
 * @param
 * @retval
 ********************************************************************************/

int main()
{
    LPRINTF("\n VIDEO_HOOK: Started");

    const char *script_name = "./videostream.sh 0 D70 &";

    // const char *kill_command = "kill $(ps | grep 'gst-launch-1.0' | awk '{print $2}')";

    const char *kill_command = "kill -15 $(echo $(pidof gst-launch-1.0))";

    // Generate unique key
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

                current_session_uid=0;
                bytes_to_uint64(&data[1], &current_session_uid);
                printf("\n VIDEO_HOOK: Current session  id to start session : %d\n", current_session_uid);

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

            case BOWL_RESTART_CMD: /* A55 Core restart*/   

            printf("\n VIDEO_HOOK:Reboot Command received from Harvest main Hook");    

            break;
            
            case M33_RPMSG_PIR_TRIGGER_CMD: /* PIR Trigger from M33 Core */

            printf("\n VIDEO_HOOK:PIR Sensor Trigger Command received from Harvest main Hook");    

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
/*******************************************************************************
* @name   bytes_to_uint64
* @brief  Function to convert 'bytes stream of 8 bytes' to
*         a unsigned long long integer (64 bit integer).
* @param  const uint8_t *data, uint64_t *ert
* @retval none
*****************************************************************************/
static void bytes_to_uint64(const uint8_t *data, uint64_t *ertc)
{
	uint8_t *ptr;
	ptr = (uint8_t *)ertc;

	ptr[0] = data[0];
	ptr[1] = data[1];
	ptr[2] = data[2];
	ptr[3] = data[3];
	ptr[4] = data[4];
	ptr[5] = data[5];
	ptr[6] = data[6];
	ptr[7] = data[7];
}
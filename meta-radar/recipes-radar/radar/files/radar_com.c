
/*
* @file    Radar_com.c
* @author  Nestle Firmware Team
* @version V1.0.0
* @date
* @brief   Source file for  Video harvesting  interface
*/

/* Standard Includes --------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdint.h>
#include <string.h>


/* Project Includes ---------------------------------------------------------*/

#include "radar_com.h"
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

key_t  key;
int    msgid;
struct message msg;

static int session_active = 0;
uint8_t    Harvestmode;

uint64_t  current_session_uid;

/* Private function prototypes -----------------------------------------------*/

void process_ipc_queue_reception(void);
static void bytes_to_uint64(const uint8_t *data, uint64_t *ertc);


/*******************************************************************************
* @name   main 
* @brief  Function handles the  communication 
* @param   
* @retval  
********************************************************************************/

int main() 
{

  LPRINTF("\n RADAR_HOOK: Starting the Radar application...");

    // Get unique key   
    key = RADAR_IPC_MSG_KEY;

    // Create message queue and return id
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) 
    {
            // perror("RADAR_HOOK: Video HOOK msgget failed");
            LPRINTF("\n RADAR_HOOK:Video HOOK msgget failed");
            // exit(EXIT_FAILURE);
    }

    Harvestmode = HARVEST_STOP;

    while (1)
     {

        process_ipc_queue_reception();

        switch(Harvestmode)
            {
            case HARVEST_RUN:

                if(session_active==1)
                {

                }
    
             break;
            case HARVEST_STOP_INITATED:

                if(session_active==0)
                 {
                    printf("\n RADAR_HOOK: Harvest stop initiated, checking remaining files upload status.......");
                   //stop Radar task here ..................

                    printf("\n RADAR_HOOK:Harvest sensor file upload done.......");
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

    LPRINTF("\n RADAR_HOOK: Application code Ended");
    return 0;
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
         memset(&msg.message_text, 0, sizeof(msg.message_text));
    
        // reads a message from the queue associated with the message queue identifier specified by msgid
        if (msgrcv(msgid, &msg, sizeof(msg), 1, IPC_NOWAIT) == -1)
        {
            if (errno == ENOMSG)
            {
                // LPRINTF("\n RADAR_HOOK:IPC No message available");
            }
            else
            {
                LPRINTF("\n RADAR_HOOK: IPC  reception Error");
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // LPRINTF("\n RADAR_HOOK: Received Data from BLE HOOK = %s", msg.message_text);

            uint8_t command_id = msg.message_text[0];
            uint8_t *data = (uint8_t *)msg.message_text + 1;

            switch (command_id)
            {

            case HARVEST_ENABLE_DISABLE_CMD: /* Start & stop harvesting */

                current_session_uid=0;
                bytes_to_uint64(&data[1], &current_session_uid);
                printf("\n RADAR_HOOK: Current session  id to start session : %d\n", current_session_uid);
                
                if (data[0] == 1)
                {
                    session_active = 1;   
                    Harvestmode=HARVEST_RUN; 
                    printf("\n RADAR_HOOK:Session Start Command received from Harvest main Hook");

                    //Start MIC task here ..................
                }
                else if (data[0] == 0)
                {
                    session_active = 0;    
                    Harvestmode=HARVEST_STOP_INITATED;  
                    printf("\n RADAR_HOOK:Session Stop Command received from Harvest main Hook");
                
                }
                break;


            case BOWL_RESTART_CMD: /* A55 Core restart*/   

                    printf("\n RADAR_HOOK:Reboot Command received from Harvest main Hook");    

                break;

            case M33_RPMSG_PIR_TRIGGER_CMD: /* PIR Trigger from M33 Core */

            printf("\n RADAR_HOOK:PIR Sensor Trigger Command received from Harvest main Hook");    

            break;

            /* Future commands need to be added here ..................*/

            default:
                // printf("\n RADAR_HOOK:invalid Command received from Harvest main Hook");
                break;
            }
        }
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



/*
 * @file    harvest_main.c
 * @author  Nestle Firmware Team
 * @version V1.0.0
 * @date
 * @brief   Source file for BLE App parser interface
 */

/* Standard Includes --------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Project Includes ---------------------------------------------------------*/

#include "harvest_main.h"

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

int bowl_harvestingstatus = 0;

key_t blekey;
int blehook_msgid;
struct message blehook_msg;
char blehook_ipcactive = 0;

key_t rpmsgkey;
int rpmsghook_msgid;
struct message rpmsghook_msg;
char rpmsghook_ipcactive = 0;

key_t mickey;
int michook_msgid;
struct message michook_msg;
char michook_ipcactive = 0;

key_t radarkey;
int radarhook_msgid;
struct message radarhook_msg;
char radarhook_ipcactive = 0;

key_t videokey;
int videohook_msgid;
struct message videohook_msg;
char videohook_ipcactive = 0;

key_t cloudkey;
int cloudhook_msgid;
struct message cloudhook_msg;
char cloudhook_ipcactive = 0;

key_t edgekey;
int edgehook_msgid;
struct message edgehook_msg;
char edgehook_ipcactive = 0;

struct message common_msg;

/* Private function prototypes -----------------------------------------------*/

void process_blehook_receptionmsg(void);
char generate_videohookmsgid(void);
char generate_radarhookmsgid(void);
char generate_michookmsgid(void);
char generate_blehookmsgid(void);
char generate_cloudhookmsgid(void);
char generate_edgehookmsgid(void);
char generate_rpmsghookmsgid(void);

void send_harvestcmd(char harvestdata);

/*******************************************************************************
 * @name   main
 * @brief  Function handles the communication
 * @param
 * @retval
 ********************************************************************************/

int main(void)
{
    char status = 0;

    LPRINTF("\n HARVEST_HOOK: starting...");
    status = generate_blehookmsgid();
    status = generate_rpmsghookmsgid();
    status  = generate_michookmsgid();
    status  = generate_radarhookmsgid();
    status = generate_videohookmsgid();
    status = generate_cloudhookmsgid();

    while (1)
    {
        process_blehook_receptionmsg();
    }

    return 0;
}

/*******************************************************************************
 * @name   process_blehook_receptionmsg
 * @brief  Function handles the  communication
 * @param
 * @retval
 ********************************************************************************/
void process_blehook_receptionmsg(void)
{

    // Clearing the msg
    memset(&blehook_msg.message_text, 0, sizeof(blehook_msg.message_text));

    if (msgrcv(blehook_msgid, &blehook_msg, sizeof(blehook_msg), 1, IPC_NOWAIT) == -1)
    {
        if (errno == ENOMSG)
        {
            // LPRINTF("\n HARVEST_HOOK:IPC No message available");
        }
        else
        {
            printf("\n HARVEST_HOOK:IPC Error\n");
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("\n HARVEST_HOOK: Received Data from BLE HOOK = %s", blehook_msg.message_text);
    // int c =  blehook_msg.message_text[0];
    // printf("\n HARVEST_HOOK: Command id str = %d\n", c);

    uint8_t command_id = blehook_msg.message_text[0];
    uint8_t *data = (uint8_t *)blehook_msg.message_text + 1;

    // if(command_id == 1) {
    //     printf("\n HARVEST_HOOK: Command id = %ld\n", command_id);
    // }

    printf("\n HARVEST_HOOK: Command id = %u\n", command_id);

    switch (command_id)
    {

    case HARVEST_ENABLE_DISABLE_CMD: /* Start & stop harvesting */
        printf("\n HARVEST_HOOK: HARVEST_ENABLE_DISABLE_CMD\n");
        send_harvestcmd(data[0]);
        // if ( msgctl(blehook_msgid,IPC_RMID,0) < 0 )
        // {
        //         perror("msgctl");
        //         return 0;
        // } else {
        //     printf("\n HARVEST_HOOK: blehook_msgid queues removed\n");
        // }
        break;

    case LOADCELL_TARE_SCALE_CMD: /* Load cell tare function */

        rpmsghook_msg.message_type = 1;                          // Message type can be used to differentiate messages if needed
        rpmsghook_msg.message_text[0] = LOADCELL_TARE_SCALE_CMD; // First byte is the command ID
        rpmsghook_msg.message_text[1] = 0;

        if (msgsnd(rpmsghook_msgid, &rpmsghook_msg, sizeof(rpmsghook_msg.message_text), 0) == -1)
        {
            printf("\n HARVEST_HOOK: Error, Not able to send load cell tare command to RPMSG HooK.\n");

            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n HARVEST_HOOK: send load cell tare command  sent to RPMSG HooK = %s\n", rpmsghook_msg.message_text);

            // for (int i = 0; i < (int)sizeof(rpmsghook_msg.message_text); i++)
            // {
            // printf("\n BLE_HOOK_INFO :%d ", rpmsghook_msg.message_text[i]); // Print the element
            // }
            printf("\n");
        }

        break;

    case LOADCELL_CALIB_WEIGHT_CMD: /* Load cell Calibration function */

        rpmsghook_msg.message_type = 1;                          // Message type can be used to differentiate messages if needed
        rpmsghook_msg.message_text[0] = LOADCELL_TARE_SCALE_CMD; // First byte is the command ID
        rpmsghook_msg.message_text[1] = data[1];                 // MSB
        rpmsghook_msg.message_text[2] = data[2];                 // LSB

        if (msgsnd(rpmsghook_msgid, &rpmsghook_msg, sizeof(rpmsghook_msg.message_text), 0) == -1)
        {
            printf("\n HARVEST_HOOK :Error, Not able to send load cell tare command to RPMSG HOOK.\n");

            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n HARVEST_HOOK : send load cell tare command  sent to RPMSG HOOK = %s\n", rpmsghook_msg.message_text);
            // for (int i = 0; i < (int)sizeof(rpmsghook_msg.message_text); i++)
            // {
            // printf("\n BLE_HOOK_INFO :%d ", rpmsghook_msg.message_text[i]); // Print the element
            // }
            printf("\n");
        }

        break;

    case A55_SHUTDOWN_CMD: /* A55 Core shutdown*/
                           // NA

        break;

    case BOWL_RESTART_CMD: /* BOWL restart command*/

        break;

    default:
        // NA
        break;
    }
    }
}

/*******************************************************************************
 * @name    send_harvestcmd
 * @brief   Function handles the  sending harvest cmd to all hooks
 * @param   None
 * @retval  None
 ********************************************************************************/
void send_harvestcmd(char harvestdata)
{
    common_msg.message_type = 1;                             // Message type can be used to differentiate messages if needed
    common_msg.message_text[0] = HARVEST_ENABLE_DISABLE_CMD; // First byte is the command ID
    common_msg.message_text[1] = harvestdata;                // Start /Stop

    if (msgsnd(rpmsghook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to RPMSG HOOK.\n");
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to RPMSG HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }

    if (msgsnd(michook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to MIC HOOK.\n");
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command sent to MIC HOOK = %s\n", common_msg.message_text);
        // for (int i = 0; i < (int)sizeof(common_msg.message_text); i++)
        // {
        // printf("\n BLE_HOOK_INFO :%d ", common_msg.message_text[i]); // Print the element
        // }
        printf("\n");
    }

    if (msgsnd(radarhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to radar HOOK.\n");
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to Radar HOOK = %s\n", common_msg.message_text);
        // for (int i = 0; i < (int)sizeof(common_msg.message_text); i++)
        // {
        // printf("\n BLE_HOOK_INFO :%d ", common_msg.message_text[i]); // Print the element
        // }
        printf("\n");
    }

    if (msgsnd(videohook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to video HOOK.\n");
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to video HOOK = %s\n", common_msg.message_text);
        // for (int i = 0; i < (int)sizeof(common_msg.message_text); i++)
        // {
        // printf("\n BLE_HOOK_INFO :%d ", common_msg.message_text[i]); // Print the element
        // }
        printf("\n");
    }

    if (msgsnd(cloudhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to cloud HOOK.\n");
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command sent to cloud HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }
}

/*******************************************************************************
 * @name    generate_blehookmsgid
 * @brief   Function handles the  generation of Msgid
 * @param   None
 * @retval  None
 ********************************************************************************/
char generate_blehookmsgid(void)
{
    // Generate unique key
    // blekey = ftok("BLE_HOOK", 65);
    blekey = 1234;

    // Create message queue and return id
    blehook_msgid = msgget(blekey, 0666 | IPC_CREAT);

    if (blehook_msgid == -1)
    {
        perror("HARVEST_HOOK: BLE HOOK msgget failed");
        printf("\n HARVEST_HOOK:BLE HOOK msgget failed");
        // exit(EXIT_FAILURE);
        blehook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: BLE HOOK created");
    }

    // Clearing the msg
    memset(&blehook_msg.message_text, 0, sizeof(blehook_msg.message_text));

    blehook_ipcactive = 1;
    return 1;
}

/*******************************************************************************
 * @name   generate_michookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_michookmsgid(void)
{

    // Generate unique key
    // mickey = ftok("MIC_HOOK", 10);
    mickey = 1393;

    // Create message queue and return id
    michook_msgid = msgget(mickey, 0666 | IPC_CREAT);

    if (michook_msgid == -1)
    {
        perror("HARVEST_HOOK: MIC HOOK msgget failed");
        printf("\n HARVEST_HOOK:MIC HOOK msgget failed");
        michook_ipcactive = 0;
        // exit(EXIT_FAILURE);
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: MIC HOOK created");
    }

    // Clearing the msg
    memset(&michook_msg.message_text, 0, sizeof(michook_msg.message_text));
    michook_ipcactive = 1;
    return 1;
}

/*******************************************************************************
 * @name   generate_radarhookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_radarhookmsgid(void)
{

    // Generate unique key
    // radarkey = ftok("RADAR_HOOK", 11);
    radarkey = 1814;

    // Create message queue and return id
    radarhook_msgid = msgget(radarkey, 0666 | IPC_CREAT);

    if (radarhook_msgid == -1)
    {
        perror("HARVEST_HOOK: Radar HOOK msgget failed");
        printf("\n HARVEST_HOOK:Radar HOOK msgget failed");
        // exit(EXIT_FAILURE);
        radarhook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: RADAR HOOK created");
    }

    // Clearing the msg
    memset(&radarhook_msg.message_text, 0, sizeof(radarhook_msg.message_text));
    radarhook_ipcactive = 1;
    return 1;
}

/*******************************************************************************
 * @name   generate_videohookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_videohookmsgid(void)
{

    // Generate unique key
    // videokey = ftok("VIDEO_HOOK", 12);
    videokey = 2945;
    // Create message queue and return id
    videohook_msgid = msgget(videokey, 0666 | IPC_CREAT);

    if (videohook_msgid == -1)
    {
        perror("HARVEST_HOOK: Video HOOK msgget failed");
        printf("\n HARVEST_HOOK:Video HOOK msgget failed");
        // exit(EXIT_FAILURE);
        videohook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: VIDEO HOOK created");
    }

    // Clearing the msg
    memset(&videohook_msg.message_text, 0, sizeof(videohook_msg.message_text));
    videohook_ipcactive = 1;
    return 1;
}

/*******************************************************************************
 * @name   generate_cloudhookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_cloudhookmsgid(void)
{

    // Generate unique key
    // cloudkey = ftok("CLOUD_HOOK", 13);
    cloudkey = 3254;

    // Create message queue and return id
    cloudhook_msgid = msgget(cloudkey, 0666 | IPC_CREAT);

    if (cloudhook_msgid == -1)
    {
        perror("HARVEST_HOOK: Cloud HOOK msgget failed");
        printf("\n HARVEST_HOOK:Cloud HOOK msgget failed");
        // exit(EXIT_FAILURE);
        cloudhook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: CLOUD HOOK created");
    }

    // Clearing the msg
    memset(&cloudhook_msg.message_text, 0, sizeof(cloudhook_msg.message_text));
    cloudhook_ipcactive = 1;
    return 1;
}

/*******************************************************************************
 * @name   generate_edgehookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_edgehookmsgid(void)
{

    // Generate unique key
    edgekey = ftok("EDGE_HOOK", 14);

    // Create message queue and return id
    edgehook_msgid = msgget(edgekey, 0666 | IPC_CREAT);

    if (edgehook_msgid == -1)
    {
        perror("HARVEST_HOOK: Egde HOOK msgget failed");
        printf("\n HARVEST_HOOK:Egde HOOK msgget failed");
        // exit(EXIT_FAILURE);
        edgehook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: EDGE HOOK created");
    }

    // Clearing the msg
    memset(&edgehook_msg.message_text, 0, sizeof(edgehook_msg.message_text));
    edgehook_ipcactive = 1;
    return 1;
}

key_t rpmsgkey;
// int   ;
struct message;
// char    =0;

/*******************************************************************************
 * @name   generate_rpmsghookmsgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_rpmsghookmsgid(void)
{

    // Generate unique key
    // rpmsgkey = ftok("RPMSG_HOOK", 15);
    rpmsgkey = 5678;

    // Create message queue and return id
    rpmsghook_msgid = msgget(rpmsgkey, 0666 | IPC_CREAT);

    if (rpmsghook_msgid == -1)
    {
        perror("HARVEST_HOOK: RPMSG HOOK msgget failed");
        printf("\n HARVEST_HOOK:RPMSG HOOK msgget failed");
        // exit(EXIT_FAILURE);
        rpmsghook_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: RPMSG HOOK created");
    }

    // Clearing the msg
    memset(&rpmsghook_msg.message_text, 0, sizeof(rpmsghook_msg.message_text));
    rpmsghook_ipcactive = 1;
    return 1;
}


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
#include "ble_commands.h"
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

key_t  blekey;
int    blehook_msgid;
struct message blehook_msg;
char   blehook_ipcactive = 0;

/* From  HARVEST TO RPMSG*/
key_t  rpmsg_tx_key    =RPMSG_TX_MSG_KEY;
int    rpmsghook_tx_msgid;
struct message rpmsghook_tx_msg;
char   rpmsghook_ipcactive = 0;


/* From RPMSG TO HARVEST*/
key_t  rpmsg_rx_key    =RPMSG_RX_MSG_KEY;
int    rpmsghook_rx_msgid;
struct message rpmsghook_rx_msg;
char   rpmsghook_rx_ipcactive = 0;

key_t  mickey           =MIC_TX_IPC_MSG_KEY;
int    michook_msgid;
struct message michook_msg;
char   michook_ipcactive = 0;

key_t  radarkey        =RADAR_TX_IPC_MSG_KEY;
int    radarhook_msgid;
struct message radarhook_msg;
char   radarhook_ipcactive = 0;

key_t  videokey        =VIDEO_TX_IPC_MSG_KEY;
int    videohook_msgid;
struct message videohook_msg;
char   videohook_ipcactive = 0;

key_t  cloudkey       =CLOUD_TX_IPC_MSG_KEY;
int    cloudhook_msgid;
struct message cloudhook_msg;
char   cloudhook_ipcactive = 0;

key_t  edgekey       =EDGE_TX_IPC_MSG_KEY;
int    edgehook_msgid;
struct message edgehook_msg;
char   edgehook_ipcactive = 0;

struct message common_msg;

uint64_t  current_session_uid;

/* Private function prototypes -----------------------------------------------*/

void process_blehook_receptionmsg(void);
void process_rpmsghook_receptionmsg(void);
char generate_videohookmsgid(void);
char generate_radarhookmsgid(void);
char generate_michookmsgid(void);
char generate_blehookmsgid(void);
char generate_cloudhookmsgid(void);
char generate_edgehookmsgid(void);
char generate_rpmsghook_tx_msgid(void);
char generate_rpmsghook_rx_msgid(void);

void send_harvestcmd(const uint8_t *data);
static void bytes_to_uint64(const uint8_t *data, uint64_t *ertc);
void send_cmd_to_hooks( uint8_t hookname ,uint8_t cmd,const uint8_t *data);

/*******************************************************************************
 * @name   main
 * @brief  Function handles the communication
 * @param
 * @retval
 ********************************************************************************/

int main(void)
{
    char status = 0;

    LPRINTF("\n HARVEST_HOOK: Starting harvest main application.......");

    /*Transmission Hook IDS */
    status  = generate_blehookmsgid();
    status  = generate_rpmsghook_tx_msgid();
    status  = generate_michookmsgid();
    status  = generate_radarhookmsgid();
    status  = generate_videohookmsgid();
    status  = generate_cloudhookmsgid();
    status  = generate_rpmsghook_rx_msgid();

    /*Reception Hook IDS */

    while (1)
    {
        process_blehook_receptionmsg();
        process_rpmsghook_receptionmsg();
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
        

            uint8_t Start_of_header   =blehook_msg.message_text[0];
            uint8_t Frame_Length      =blehook_msg.message_text[1];
            uint8_t Frame_Type        =blehook_msg.message_text[2];
            uint8_t command_id        =blehook_msg.message_text[3];

             uint8_t *data = (uint8_t *)blehook_msg.message_text + 4;


            printf("\n HARVEST_HOOK: Command id = %u\n", command_id);

            if (MOBILE_APP_A55CORE_COMM_SOH==Start_of_header)
            {
                switch (command_id)
                {

                    case CMD_HARVEST_ENABLE_DISABLE: /* Start & stop harvesting */

                    printf("\n HARVEST_HOOK: HARVEST_ENABLE_DISABLE_CMD\n");

                    send_harvestcmd(&data[0]);

                    break;

                    case CMD_DO_LOADCELL_TARE: /* Load cell tare function */

                    rpmsghook_tx_msg.message_type = 1;                          // Message type can be used to differentiate messages if needed
                    rpmsghook_tx_msg.message_text[0] = LOADCELL_TARE_SCALE_CMD; // First byte is the command ID
                    rpmsghook_tx_msg.message_text[1] = 0;

                    if (msgsnd(rpmsghook_tx_msgid, &rpmsghook_tx_msg, sizeof(rpmsghook_tx_msg.message_text), 0) == -1)
                    {
                    printf("\n HARVEST_HOOK: Error, Not able to send load cell tare command to RPMSG HooK.\n");

                   // perror("msgsnd failed");
                    //exit(EXIT_FAILURE);
                    }
                    else
                    {
                    printf("\n HARVEST_HOOK: send load cell tare command  sent to RPMSG HooK = %s\n", rpmsghook_tx_msg.message_text);
                    printf("\n");
                    }

                    break;

                    case CMD_DO_LOADCELL_CALIBRATION: /* Load cell Calibration function */

                    rpmsghook_tx_msg.message_type = 1;                          // Message type can be used to differentiate messages if needed
                    rpmsghook_tx_msg.message_text[0] = LOADCELL_CALIB_WEIGHT_CMD; // First byte is the command ID
                    rpmsghook_tx_msg.message_text[1] = data[1];                 // MSB
                    rpmsghook_tx_msg.message_text[2] = data[2];                 // LSB

                    if (msgsnd(rpmsghook_tx_msgid, &rpmsghook_tx_msg, sizeof(rpmsghook_tx_msg.message_text), 0) == -1)
                    {
                    printf("\n HARVEST_HOOK :Error, Not able to send load cell tare command to RPMSG HOOK.\n");

                   // perror("msgsnd failed");
                    //exit(EXIT_FAILURE);
                    }
                    else
                    {
                    printf("\n HARVEST_HOOK : send load cell tare command  sent to RPMSG HOOK = %s\n", rpmsghook_tx_msg.message_text);
                    printf("\n");
                    }

                    break;

                    case CMD_FACTORY_RESET: /*Factory reset parameters to default */
                    // NA

                    break;

                    case CMD_DEVICE_REBOOT: /* BOWL reboot command*/

                    break;

                    case CMD_GET_SESSION_DETAILS:
                    case CMD_GET_SESSION_DATA:
                    case CMD_GET_ACTIVITY_SESSION_DETAILS:
                    case CMD_GET_ACTIVITY_SESSION_DATA:
                    case CMD_GET_DASHBOARD_INFO:
                    case CMD_SET_DOG_SIZE:
                    case CMD_GET_TOTAL_SESSION_COUNT:
                    case CMD_GET_SESSION_BY_INDEX:
                    case CMD_SET_SENSOR_BOWL:
                    case CMD_START_OTA:


                    printf("\n HARVEST_HOOK :Command Not implemented .............\n");
                    break;

                    default:
                    // NA
                    break;
                }
            }
            else
            {
                printf("\n HARVEST_HOOK: Start of header mismatch  data from  ble hook \n");
            }
    }
}

/*******************************************************************************
 * @name   rpmsg_queue_reception
 * @brief  Function handles the
 * @param
 * @retval
 ********************************************************************************/
void rpmsg_queue_reception(void)
{
     // Clearing the msg
    memset(&rpmsghook_rx_msg.text, 0, sizeof(rpmsghook_rx_msg.text));

    if (msgrcv(rpmsghook_rx_msgid, &rpmsghook_rx_msg, sizeof(rpmsghook_rx_msg), 1, IPC_NOWAIT) == -1)
    {
        if (errno == ENOMSG)
        {
            // LPRINTF("\n RPMSG_INFO:IPC No message available");
        }
        else
        {
            LPRINTF("\n HARVEST_HOOK: RPMSG RX IPC Error");
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
    }
    else
    {

    uint8_t command_id = rpmsghook_rx_msg.text[0];
    uint8_t *data      = (uint8_t *)rpmsghook_rx_msg.text + 1;


    switch (command_id)
    {

    case M33_RPMSG_PIR_TRIGGER_CMD: /* PIR Trigger from M33 Core */

    printf("\n HARVEST_HOOK: PIR Sensor Trigger Command received from RPMSG Hook");        

    send_cmd_to_hooks( RADAR_HOOK ,M33_RPMSG_PIR_TRIGGER_CMD,&data[1]);
    send_cmd_to_hooks( CLOUD_HOOK ,M33_RPMSG_PIR_TRIGGER_CMD,&data[1]);
    send_cmd_to_hooks( MIC_HOOK   ,M33_RPMSG_PIR_TRIGGER_CMD,&data[1]);
    send_cmd_to_hooks( VIDEO_HOOK ,M33_RPMSG_PIR_TRIGGER_CMD,&data[1]);

    break;

    default:
        // printf("\n RPMSG_INFO:invalid Command received from BLE & sending to M33");
    break;
    }

       
    }

   
}


/*******************************************************************************
 * @name    send_harvestcmd
 * @brief   Function handles the  sending harvest cmd to all hooks
 * @param   const uint8_t *data
 * @retval  None
 ********************************************************************************/
void send_harvestcmd(const uint8_t *data)
{

   // Clearing the msg
    memset(&common_msg.message_text, 0, sizeof(common_msg.message_text));
    current_session_uid=0;
    bytes_to_uint64(&data[1], &current_session_uid);
    printf("\n HARVEST_HOOK: Current session  id to start session : %d\n", current_session_uid);

    common_msg.message_type    = 1;                          // Message type can be used to differentiate messages if needed
    common_msg.message_text[0] = HARVEST_ENABLE_DISABLE_CMD; // First byte is the command ID
    common_msg.message_text[1] = data[0];                    // Start /Stop
    common_msg.message_text[2] = data[1];                    //Session ID Byte 0
	common_msg.message_text[3] = data[2];                    //Session ID Byte 1
	common_msg.message_text[4] = data[3];                    //Session ID Byte 2
	common_msg.message_text[5] = data[4];                    //Session ID Byte 3
	common_msg.message_text[6] = data[5];                    //Session ID Byte 4
	common_msg.message_text[7] = data[6];                    //Session ID Byte 5
	common_msg.message_text[8] = data[7];                    //Session ID Byte 6
    common_msg.message_text[9] = data[8];                    //Session ID Byte 7


    if (msgsnd(rpmsghook_tx_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to RPMSG HOOK.\n");
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to RPMSG HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }

    if (msgsnd(michook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to MIC HOOK.\n");
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command sent to MIC HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }

    if (msgsnd(radarhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to radar HOOK.\n");
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to Radar HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }

    if (msgsnd(videohook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to video HOOK.\n");
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command  sent to video HOOK = %s\n", common_msg.message_text);

        printf("\n");
    }

    if (msgsnd(cloudhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
    {
        printf("\n HARVEST_HOOK :Error, Not able to send harvest start/stop command to cloud HOOK.\n");
    }
    else
    {
        printf("\n HARVEST_HOOK : send harvest start/stop command sent to cloud HOOK = %s\n", common_msg.message_text);
        printf("\n");
    }
}

/*******************************************************************************
 * @name    send_cmd_to_hooks
 * @brief   Function handles the  sending harvest cmd to all hooks
 * @param   const uint8_t *data
 * @retval  None
 ********************************************************************************/
void send_cmd_to_hooks( uint8_t hookname ,uint8_t cmd,const uint8_t *data)
{

   // Clearing the msg
    memset(&common_msg.message_text, 0, sizeof(common_msg.message_text)); 

    common_msg.message_type    = 1;                          // Message type can be used to differentiate messages if needed
    common_msg.message_text[0] = cmd;                       // First byte is the command ID
    common_msg.message_text[1] = data[0];                     
    common_msg.message_text[2] = data[1];                     
	common_msg.message_text[3] = data[2];                     
	common_msg.message_text[4] = data[3];                      
	common_msg.message_text[5] = data[4];                     
	common_msg.message_text[6] = data[5];                     
	common_msg.message_text[7] = data[6];                     
	common_msg.message_text[8] = data[7];                     
    common_msg.message_text[9] = data[8];     


        switch (hookname)
        {

        case  RPMSG_HOOK :

            if (msgsnd(rpmsghook_tx_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
            {
            printf("\n HARVEST_HOOK :Error, Not able to send command to RPMSG HOOK.\n");
            }
            else
            {
            printf("\n HARVEST_HOOK : sent command to RPMSG HOOK = %s\n", common_msg.message_text);
            printf("\n");
            }

        break;

        case  MIC_HOOK :

            if (msgsnd(michook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
            {
            printf("\n HARVEST_HOOK :Error, Error, Not able to send command to MIC HOOK.\n");
            }
            else
            {
            printf("\n HARVEST_HOOK : sent command to MIC HOOK = %s\n", common_msg.message_text);
            printf("\n");
            }

        break;

        case  RADAR_HOOK :

            if (msgsnd(radarhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
            {
            printf("\n HARVEST_HOOK :Error,  Not able to send command to radar HOOK.\n");
            }
            else
            {
            printf("\n HARVEST_HOOK : sent command to Radar HOOK = %s\n", common_msg.message_text);
            printf("\n");
            }

        break;

        case  VIDEO_HOOK :

            if (msgsnd(videohook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
            {
            printf("\n HARVEST_HOOK :Error, Not able to send command to video HOOK.\n");
            }
            else
            {
            printf("\n HARVEST_HOOK : sent command to video HOOK = %s\n", common_msg.message_text);

            printf("\n");
            }

        break;

        case  CLOUD_HOOK :

            if (msgsnd(cloudhook_msgid, &common_msg, sizeof(common_msg.message_text), 0) == -1)
            {
            printf("\n HARVEST_HOOK :Error, Not able to send command tocloud HOOK.\n");
            }
            else
            {
            printf("\n HARVEST_HOOK : sent command to cloud HOOK = %s\n", common_msg.message_text);
            printf("\n");
            }

        break;

        case  EDGE_HOOK :
          printf("\n HARVEST_HOOK :Error, Not applicable to edge hook as of now .\n");
        break;

        case  BLE_HOOK :
         printf("\n HARVEST_HOOK :Error, Not applicable to BLE hook as of now .\n");
        break;

        case  HARVEST_HOOK :
            printf("\n HARVEST_HOOK :Error, Not applicable to the same harvesting hook   .\n");
        break;

        default :
        printf("\n HARVEST_HOOK :Error, sending to undefined  hook   .\n");
        break;

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

/*******************************************************************************
 * @name    generate_blehookmsgid
 * @brief   Function handles the  generation of Msgid
 * @param   None
 * @retval  None
 ********************************************************************************/
char generate_blehookmsgid(void)
{
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
/*******************************************************************************
 * @name   generate_rpmsghook_tx_msgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_rpmsghook_tx_msgid(void)
{
    // Create message queue and return id
    rpmsghook_tx_msgid = msgget(rpmsg_tx_key, 0666 | IPC_CREAT);

    if (rpmsghook_tx_msgid == -1)
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
    memset(&rpmsghook_tx_msg.message_text, 0, sizeof(rpmsghook_tx_msg.message_text));
    rpmsghook_ipcactive = 1;
    return 1;
}



 /*******************************************************************************
 * @name   generate_rpmsghook_rx_msgid
 * @brief  Function handles the  generation of Msgid
 * @param
 * @retval
 ********************************************************************************/
char generate_rpmsghook_rx_msgid(void)
{
    // Create message queue and return id
    rpmsghook_rx_msgid = msgget(rpmsg_rx_key, 0666 | IPC_CREAT);

    if (rpmsghook_rx_msgid == -1)
    {
        perror("HARVEST_HOOK: RPMSG HOOK rx  msgget failed");
        printf("\n HARVEST_HOOK:RPMSG HOOK  rx msgget failed");
        // exit(EXIT_FAILURE);
        rpmsghook_rx_ipcactive = 0;
        return 0;
    }
    else
    {
        LPRINTF("\nHARVEST_HOOK: RPMSG HOOK rx  created");
    }

    // Clearing the msg
    memset(&rpmsghook_rx_msg.message_text, 0, sizeof(rpmsghook_rx_msg.message_text));
    rpmsghook_rx_ipcactive = 1;
    return 1;
}

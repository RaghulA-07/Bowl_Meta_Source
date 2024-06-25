

 /*
* @file    Mic_com.c
* @author  Nestle Firmware Team
* @version V1.0.0
* @date
* @brief   Source file for Mic_com interface
*/

/* Standard Includes --------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdint.h>
#include <string.h>


/* Project Includes ---------------------------------------------------------*/

#include "mic_com.h"
/* Private typedef ----------------------------------------------------------*/


/* Private define -----------------------------------------------------------*/
#define MAX_MESSAGE_SIZE 100

/* Private macro ------------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/



/* Private variables --------------------------------------------------------*/

struct message {
    long message_type;
    uint8_t message_text[MAX_MESSAGE_SIZE];
};

/* Private function prototypes -----------------------------------------------*/



/*******************************************************************************
* @name   main 
* @brief  Function handles the  communication 
* @param   
* @retval  
********************************************************************************/
int main()
 {
    LPRINTF("\n MIC_HOOK: Started");

    key_t key;
    int msgid;
    struct message msg;

    // Generate unique key
    //key = ftok("MIC_HOOK", 10);
    key = 1393;

    // Create message queue and return id
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("MIC_HOOK:msgget failed");
        LPRINTF("\n MIC_HOOK:MIC  msgget failed");
        exit(EXIT_FAILURE);
    }

    while (1)
     {
         // Clearing the msg
        memset(&msg.message_text, 0, sizeof(msg.message_text));

        // Receive message
        // if (msgrcv(msgid, &msg, sizeof(msg.message_text), 1, IPC_NOWAIT) == -1) 
        if (msgrcv(msgid, &msg, sizeof(msg.message_text), 1, 0) == -1) {
            perror("MIC_HOOK: msgrcv failed");
            // exit(EXIT_FAILURE);
        }

        // Print the received message
        printf("\n MIC_HOOK:Received message: ");
        for (size_t i = 0; i < strlen((char*)msg.message_text); i++) {
            printf("MIC_HOOK :%02X ", msg.message_text[i]);
        }
        printf("\n");

        // Optionally, you can add further processing of the received message here
    }

    LPRINTF("\n MIC_HOOK: Ended");
    
    return 0;
}

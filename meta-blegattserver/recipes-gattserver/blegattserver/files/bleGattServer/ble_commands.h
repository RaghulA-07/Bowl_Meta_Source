
/******************************************************************************
* @file     ble_commands.h
* @author   Nestle Firmware Team
* @version  V1.0.0
* @date
* @brief   Header file for  BLE mobile application communication commands 
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __COMMANDS__H
#define __COMMANDS__H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/*Commands between BLE Mobile application to  A55 BLE Hook*/

#define	CMD_GET_DEVICE_INFO                 (0x02)
#define	CMD_GET_EPOCH_CLOCK                 (0x05)
#define	CMD_SET_EPOCH_CLOCK                 (0x06)
#define HARVEST_ENABLE_DISABLE_CMD          (0x10)   //start Or Stop Harvesting
#define	CMD_GET_DASHBOARD_INFO              (0x16)
#define CMD_SET_WIFI_CREDENTIAL             (0x1C) 
#define	CMD_GET_WIFI_CREDENTIAL             (0x1D)

#define CMD_SET_DOG_SIZE                    (0x1A)
#define	CMD_GET_SESSION_DETAILS             (0x11) //Need to check for BOWL
#define	CMD_GET_SESSION_DATA                (0x12) //Need to check for BOWL
#define	CMD_GET_ACTIVITY_SESSION_DETAILS    (0x13) //Need to check for BOWL
#define CMD_GET_ACTIVITY_SESSION_DATA       (0x14) //Need to check for BOWL


#define	 CMD_GET_TOTAL_SESSION_COUNT        (0x1E)
#define	 CMD_GET_SESSION_BY_INDEX           (0x1F)


#define	 CMD_SET_SENSOR_BOWL                (0x20)
 

#define	 CMD_DO_LOADCELL_TARE               (0x21)
#define	 CMD_DO_LOADCELL_CALIBRATION        (0x22)
#define	 CMD_START_OTA                      (0x23)

/*------------Header Types------------------------*/

#define MOBILE_APP_A55CORE_COMM_SOH          0x01 /*Mobile Application to A55CORE */
#define A55CORE_MOBILE_APP_COMM_SOH          0x02 /*A55CORE to Mobile Application  */


/*------------Frame Types------------------------*/
#define COMM_REQUEST_FRAME                   0xF1
#define COMM_REPORT_FRAME                    0xF2
#define COMM_NOTIFICATION_FRAME              0xF3
#define COMM_DATA_FRAME                      0xF4

 /*----Error Management-Exception Code-----------*/

#define eMSG_SUCCESS_ACK                     0x11
#define eMSG_ACK_Failure                     0x12
#define BAD_CHECKSUM                         0x13
#define INVALID_FRAME_LENGTH                 0x14
#define INVALID_START_BYTE                   0x15
#define eMSG_ACK_invalidframetype            0x16
#define INVALID_CMD                          0x17
#define INVALID_DATA                         0x18
#define UNABLE_TO_PROCESS                    0x19
#define SLAVE_DEVICE_FAILURE                 0x1A
#define TIMEOUT                              0x1B
#define eMSG_RESPONSE_FRAME_HEAP_ERR         0x1C
#define eMSG_MEM_WRITE_FAILED                0x1D
#define eMSG_MEM_READ_FAILED                 0x1E
#define eMSG_INVALID_PARAMETERS              0x1F
#define RESERVED5                            0x20
#define QUEUE_BUFFER_FULL                    0x21
#define WIFI_NOT_ESTABLISHED                 0x22
#define BLE_NOT_ESTABLISHED                  0x23
#define AWS_NOT_ESTABLISHED                  0x24
#define SENSOR_COMM_ERROR                    0x25
#define eMSG_RECORDS_NOT_FOUND               0x26
#define eMSG_ID_NOT_FOUND                    0x27
#define eMSGACK_SESSION_IN_PROGRESS          0x28


 /*----Start/ stop harvesting feedback----------*/

#define eMSG_HARVESTINVALID                 0x00
#define eMSG_NEW_HARVEST_SESSION_STARTED    0x01
#define eMSG_SESSION_IN_PROGRESS            0x02
#define eMSG_SESSION_NOT_IN_PROGRESS        0x03
#define eMSG_SESSION_STOPPED                0x04
#define eMSG_MAX_SESSION_REACHED            0x05
#define eMSG_ALLSESSION_DB_ERASED           0x06
#define eMSG_SENSOR_ERROR                   0x07
#define eMSG_NOT_APPLICABLE                 0x08
#define eMSG_ALLSESSION_DB_NOT_ERASED       0x09
#define eMSG_ERASE_IN_PROGRESS              0x0A
#define eMSG_SESSION_IN_PROGRESS_NOT_ERASE  0x0B
#define eMSG_SESSION_ID_ALREADY_EXISTS      0x0C

/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */


 

#ifdef __cplusplus
}
#endif

#endif /* __COMMANDS__H */



/******************************************************************************
* @file     harvest_main.h
* @author   Nestle Firmware Team
* @version  V1.0.0
* @date
* @brief   Header file for harvest_main interface
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __HARVEST_MAIN_H
#define __HARVEST_MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/*Hook Number def*/
#define RADAR_HOOK                         (0x01)
#define CLOUD_HOOK                         (0x02)
#define RPMSG_HOOK                         (0x03)
#define MIC_HOOK                           (0x04)
#define VIDEO_HOOK                         (0x05)
#define HARVEST_HOOK                       (0x06)
#define EDGE_HOOK                          (0x07)
#define BLE_HOOK                           (0x08)



/*Commands between BLE Hook to Harvest main Hook*/
#define HARVEST_ENABLE_DISABLE_CMD         (0x01)
#define LOADCELL_TARE_SCALE_CMD            (0x02)//Tare the scale
#define LOADCELL_CALIB_WEIGHT_CMD          (0x03)
#define SET_WIFI_CREDEN_CMD                (0x04)
#define GET_WIFI_CREDEN_CMD                (0x05)
#define A55_SHUTDOWN_CMD                   (0x06)
#define BOWL_RESTART_CMD                   (0x07)
#define GET_SESSION_DETAILS_CMD            (0x08)
#define GET_SESSION_DATA_CMD               (0x09)
#define GET_SESSION_DETAILS_ACTIVITY_CMD   (0x0A)
#define GET_SESSION_DATA_ACTIVITY_CMD      (0x0B)
#define GET_TOTALSESSION_CNT_CMD           (0x0C)
#define SET_SENSOR_BOWL_CMD                (0x0D)
#define M33_RPMSG_PIR_TRIGGER_CMD          (0x0E)

/*IPC Harvest Transmission message Keys from harvest to other hooks */
 
#define MIC_TX_IPC_MSG_KEY                 1393
#define CLOUD_TX_IPC_MSG_KEY               3254
#define VIDEO_TX_IPC_MSG_KEY               2945
#define RADAR_TX_IPC_MSG_KEY               1814
#define BLE_TX_IPC_MSG_KEY                 1234
#define EDGE_TX_IPC_MSG_KEY                7171
#define RPMSG_TX_MSG_KEY                   5678 

/*IPC Harvest Reception  message Keys  from other hooks*/
 
#define RPMSG_RX_MSG_KEY                   1678 

/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define LPRINTF(format, ...) printf(format "\n", ##__VA_ARGS__)

/* Exported functions ------------------------------------------------------- */


 

#ifdef __cplusplus
}
#endif

#endif /* __HARVEST_MAIN_H */

 
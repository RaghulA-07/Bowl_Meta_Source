/******************************************************************************
* @file     main.h
* @author   Nestle Firmware Team
* @version  V1.0.0
* @date
* @brief   Header file for  Harvest_main  interface
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

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

 
#define SENSOR_DATA_FRAME                  0x01
#define PIR_WAKEUP_FRAME                   0x02

/*IPC Harvest Transmission message Keys from harvest to rpmsg hooks */
#define RPMSG_TX_MSG_KEY                   5678

#define RPMSG_RX_MSG_KEY                   1678 /*Transmission from RMSP hook to Harvest hook*/

/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void ble_queue_reception();

 

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

 
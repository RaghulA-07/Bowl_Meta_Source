
/******************************************************************************
* @file     Mic_com.h
* @author   Nestle Firmware Team
* @version  V1.0.0
* @date
* @brief   Header file for  Mic comm  interface
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MIC_COMM_H
#define __MIC_COMM_H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

#define MIC_IPC_MSG_KEY              1393

#define HARVEST_RUN                    1
#define HARVEST_STOP_INITATED          2
#define HARVEST_STOP                   3


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


/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define LPRINTF(format, ...) printf(format "\n", ##__VA_ARGS__)

/* Exported functions ------------------------------------------------------- */


 

#ifdef __cplusplus
}
#endif

#endif /* __MIC_COMM_H */


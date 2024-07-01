/******************************************************************************
* @file     CommandParser.h
* @author   Nestle Firmware Team
* @version  V1.0.0
* @date
* @brief   Header file for  BLE mobile application communication commands parser
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __COMMAND_PARSER__H
#define __COMMAND_PARSER__H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

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


/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */

void parseCommand(const uint8_t *data, int data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
void startOrStopRecording_report(uint8_t data, uint8_t harvestingState,  void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
void setWifiConfig(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void getWifiConfig(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void getDashboardInfoBowl(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void getTotalSessionsCount(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void getSessionByIndex(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void sendAck(uint8_t cmd, const uint8_t *data, size_t data_length, void (*readOrNotifyCallback)(const uint8_t *, size_t));
void setTimeSync(const uint8_t *data, size_t dataSize, void (*readCallback)(const uint8_t *, size_t));
void getTimeSync(const uint8_t *data, size_t dataSize, void (*readCallback)(const uint8_t *, size_t));
void setDogSize(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void ReadRecordingSessionDetails(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void ReadRecordingSessionData(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void ReadRecordingSessionDataActivity(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void ReadRecordingSessionDetailsActivity(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
void setSensorForBowl(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t));
uint16_t calculateChecksum(const uint8_t *data, size_t data_length);
void appendChecksumAndUpdateLength(uint8_t *packet, size_t packet_size);
const char **decodeSelection(int number, int *selectedCount);
 

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_PARSER__H */



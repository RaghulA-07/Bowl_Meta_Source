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





/* Exported Variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */

void parseCommand(const uint8_t *data, int data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
void startOrStopRecording(uint8_t data, uint8_t harvestingState,  void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
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



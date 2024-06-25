#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

void parseCommand(const uint8_t *data, int data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
void startOrStopRecording(const uint8_t *data, size_t data_length, void (*readCallback)(const uint8_t *, size_t), void (*notifyCallback)(const uint8_t *, size_t));
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
#endif /* COMMAND_PARSER_H */

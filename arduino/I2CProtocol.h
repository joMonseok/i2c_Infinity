#ifndef I2C_PROTOCOL_H
#define I2C_PROTOCOL_H

#include <Arduino.h>
#include <Wire.h>
#include <SoftwareWire.h>
#include <stdint.h>

// Protocol Commands
#define CMD_STRING_RECEIVE          0xB0    // 마스터가 보내는 문자열 받기
#define CMD_REGISTER_NEW_ID         0xA0    // 새로운 ID 등록
#define CMD_REQUEST_DATA_LENGTH     0xA1    // 마스터에 데이터 길이 요청
#define CMD_REQUEST_DATA            0xA2    // 마스터에 데이터 요청
#define CMD_REQUEST_NAME            0xA4    // 센서 이름 요청
#define CMD_ALIVE_CHECK             0xC0    // 마스터에 alive 요청

// Response Codes
#define ACK                         0x06    // Acknowledge
#define NACK                        0x15    // Not Acknowledge

// Protocol Constants
#define MAX_I2C_BUFFER_SIZE         32      // I2C 최대 버퍼 크기

extern SoftwareWire sensorWire;


void startI2CsMaster();
void startI2CsSlave(uint8_t slaveAddress);

//===================== Master ====================
/**
 * @brief ACK 대기
 */
bool I2C_chkNewBoard();
/**
 * @brief ACK 대기
 */
bool I2C_waitForACK(uint8_t slaveAddr);
/**
 * @brief 0xA0: 슬레이브에 새로운 ID 등록 및 센서 이름 길이 수신
 */
uint16_t I2C_registerNewID(uint8_t slaveAddr, uint8_t newAddress, bool readSensorNameLength);
/**
 * @brief 0xA1: 슬레이브에 데이터 길이 요청
 */
uint16_t I2C_requestDataLength(uint8_t slaveAddr);
/**
 * @brief 0xA2: 슬레이브에 데이터 요청(쵀대 32바이트)
 * 32바이트 이상 받으려면 여러 번 호출 필요
 */
int I2C_requestData(uint8_t slaveAddr, String *buffer, uint16_t maxLength);
/**
 * @brief 0xA4: 슬레이브에 센서 이름 요청
 */
bool I2C_requestSensorName(uint8_t slaveAddr, String *nameBuffer, uint16_t bufferSize);
/**
 * @brief 0xC0: 슬레이브 alive 확인
 */
bool I2C_checkAlive(uint8_t slaveAddr);

//===================== Slave ====================
/**
 * @brief 0xB0: 문자열 수신 처리 (슬레이브)
 */
void I2C_handleStringReceive(String* buffer);
/**
 * @brief 0xA0: 새 ID 등록 처리 (슬레이브)
 */
void I2C_handleRegisterNewID(uint8_t newAddress);
/**
 * @brief 0xA1: 데이터 길이 반환 (슬레이브)
 */
void I2C_sendDataLength(uint16_t length);
/**
 * @brief 0xA2: 데이터 전송 (슬레이브)
 */
void I2C_sendData(String* data, uint16_t startIndex, uint16_t length);
/**
 * @brief 0xA4: 센서 이름 전송 (슬레이브)
 */
void I2C_sendSensorName(const char* sensorName);
/**
 * @brief 0xC0: Alive 응답 (슬레이브)
 */
void I2C_sendAliveResponse();
/**
 * @brief ACK 응답
 */
void I2C_sendACK();
/**
 * @brief NACK 응답
 */
void I2C_sendNACK();
#endif // I2C_PROTOCOL_H
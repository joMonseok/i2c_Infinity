#include "I2CProtocol.h"
#include <SoftwareWire.h>

SoftwareWire sensorWire(2, 3);

void startI2CsMaster()
{
    sensorWire.begin();
}
void startI2CsSlave(uint8_t slaveAddress)
{
    Wire.begin(slaveAddress);
}
//===================== Master ====================

bool I2C_chkNewBoard()
{
    return I2C_checkAlive(0x01);
}

bool I2C_waitForACK(uint8_t slaveAddr)
{
    sensorWire.requestFrom(slaveAddr, (uint8_t)1);
    unsigned long startTime = millis();
    while (sensorWire.available() < 1) {
        if (millis() - startTime > 100) {
            return false;
        }
    }
    uint8_t response = sensorWire.read();
    return (response == ACK);
}

uint16_t I2C_registerNewID(uint8_t slaveAddr, uint8_t newAddress, bool readSensorNameLength)
{
    sensorWire.beginTransmission(slaveAddr);
    sensorWire.write(CMD_REGISTER_NEW_ID);
    sensorWire.write(newAddress);
    sensorWire.endTransmission();
    uint16_t leng=0;
    if(readSensorNameLength)
    {
        sensorWire.requestFrom(slaveAddr, (uint8_t)2);
        
        if (sensorWire.available()<2) {
            while(sensorWire.available()>0)
                sensorWire.read();
            return 0;
        }

        uint8_t hi = sensorWire.read();
        uint8_t lo = sensorWire.read();
        leng = (hi << 8) | lo;
    }
    return leng;
}
uint16_t I2C_requestDataLength(uint8_t slaveAddr)
{
    sensorWire.beginTransmission(slaveAddr);
    sensorWire.write(CMD_REQUEST_DATA_LENGTH);
    sensorWire.endTransmission();
    
    sensorWire.requestFrom(slaveAddr, (uint8_t)2);
    
    if (sensorWire.available()<2) {
        while(sensorWire.available()>0)
            sensorWire.read();
        return 0;
    }

    uint8_t hi = sensorWire.read();
    uint8_t lo = sensorWire.read();
    uint16_t length = (hi << 8) | lo;
    
    return length;
}

int I2C_requestData(uint8_t slaveAddr, String *buffer, uint16_t maxLength)
{
    sensorWire.beginTransmission(slaveAddr);
    sensorWire.write(CMD_REQUEST_DATA);
    sensorWire.endTransmission();
    
    uint16_t bytesRead = 0;
    unsigned long startTime = millis();
    
    while (bytesRead < maxLength) {
        uint16_t chunkSize = maxLength-bytesRead>MAX_I2C_BUFFER_SIZE ? MAX_I2C_BUFFER_SIZE : maxLength-bytesRead;
        uint16_t readByte = 0;
        sensorWire.requestFrom(slaveAddr, chunkSize);
        
        unsigned long chunkStartTime = millis();
        while (sensorWire.available() == 0) {
            if (millis() - chunkStartTime > 100) {
                return bytesRead;
            }
        }
        
        while (sensorWire.available() && readByte < chunkSize) {
            char c = sensorWire.read();
            buffer->concat(c);
            readByte++;
        }
        bytesRead += readByte;

        if (millis() - startTime > 1000) {
            break;
        }
    }
    
    return bytesRead;
}

bool I2C_requestSensorName(uint8_t slaveAddr, String *nameBuffer, uint16_t bufferSize)
{
    sensorWire.beginTransmission(slaveAddr);
    sensorWire.write(CMD_REQUEST_NAME);
    sensorWire.endTransmission();
    
    uint16_t bytesRead = 0;
    unsigned long startTime = millis();
    
    while (bytesRead < bufferSize) {
        uint16_t chunkSize = bufferSize-bytesRead>MAX_I2C_BUFFER_SIZE ? MAX_I2C_BUFFER_SIZE : bufferSize-bytesRead;
        uint16_t readByte = 0;
        sensorWire.requestFrom(slaveAddr, chunkSize);
        
        unsigned long chunkStartTime = millis();
        while (sensorWire.available() == 0) {
            if (millis() - chunkStartTime > 100) {
                return false;
            }
        }
        
        while (sensorWire.available() && readByte < chunkSize) {
            uint8_t c = sensorWire.read();
            nameBuffer->concat((char)c);
            Serial.print("read char: ");
            Serial.print(c);
            Serial.print(" ");
            Serial.println((char)c);
            readByte++;
        }
        bytesRead += readByte;

        if (millis() - startTime > 1000) {
            break;
        }
    }
    
    return true;
}

bool I2C_checkAlive(uint8_t slaveAddr)
{
    sensorWire.beginTransmission(slaveAddr);
    sensorWire.write(CMD_ALIVE_CHECK);
    sensorWire.endTransmission();
    
    return I2C_waitForACK(slaveAddr);
}

//===================== Slave ====================

void I2C_handleStringReceive(String* buffer)
{
    byte hi = Wire.read();       // 상위 바이트
    byte lo = Wire.read();       // 하위 바이트
    int leng = (hi << 8) | lo;  // 다시 int로 합치기
    Serial.print("leng: ");
    Serial.println(leng);

    int readLeng=0;

    delay(10);
    unsigned long startTime = millis();
    while((millis() - startTime < 1000) && readLeng<leng)
    {
        if(Wire.available() > 0 && readLeng<leng)
        {
            char c = Wire.read();
            buffer->concat(c);
            readLeng++;
        }
    }
}

void I2C_handleRegisterNewID(uint8_t newAddress)
{
    Wire.end();
    delay(10);
    Wire.begin(newAddress);
}

void I2C_sendDataLength(uint16_t length)
{
    uint8_t hi = (length >> 8) & 0xFF;
    uint8_t lo = length & 0xFF;
    Wire.write(hi);
    Wire.write(lo);
}

void I2C_sendData(String* data, uint16_t startIndex, uint16_t length)
{
    // for (uint16_t i = 0; i < length; i++) {
    //     Wire.write(data[i]);
    // }
    Wire.write(data->c_str()+startIndex, length);
}

void I2C_sendSensorName(const char* sensorName)
{
    Wire.write(sensorName, strlen(sensorName));
}

void I2C_sendAliveResponse()
{
    Wire.write(ACK);
}

void I2C_sendACK()
{
    Wire.write(ACK);
}

void I2C_sendNACK()
{
    Wire.write(NACK);
}
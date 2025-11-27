#include <Wire.h>
#include "I2CProtocol.h"
#include <ArduinoJson.h>

#define MAX_SENSOR_NUM 5

byte myAddress = 1;
byte newAddr = 1;
byte command = 0x00;
int sensorId[MAX_SENSOR_NUM];
String sensorName[MAX_SENSOR_NUM];
int sensorErrorChk[MAX_SENSOR_NUM];
byte idNum = 0x08;
int data = 0;
String buf;
String json;

unsigned long prevChkMs = 0;
unsigned long prevAliveMs = 0;
unsigned long prevRequestMs = 0;

void setup() {
  startI2CsMaster();
  startI2CsSlave(0x01);
  Serial.begin(115200);
  for(int i=0;i<MAX_SENSOR_NUM;i++)
  {
    sensorId[i]=0;
    sensorName[i]="";
  }
  
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  for(int i=0;i<MAX_SENSOR_NUM;i++)
  {
    sensorId[i]=0;
    sensorName[i]="";
    sensorErrorChk[i]=0;
  }
}

void loop() {
  unsigned long now = millis();
  if(now - prevChkMs > 5000) {
    prevChkMs = now;
    if(I2C_chkNewBoard()) {
      Serial.println("New Board Detected!");
      uint16_t nameLength = I2C_registerNewID(0x01, idNum, true);
      if(nameLength > 0) {
        String nameBuffer = "";
        if(I2C_requestSensorName(0x01, &nameBuffer, nameLength)) {
          Serial.print("Registered Sensor Name: ");
          Serial.println(nameBuffer);
          if(idNum > 0) {
            sensorId[idNum-0x08] = idNum;
            sensorName[idNum-0x08] = nameBuffer;
            idNum++;
          }
        }
      }
    }
  }
  if(now - prevAliveMs > 3000) {
    prevAliveMs = now;
    for(int i=0; i<MAX_SENSOR_NUM; i++) {
      if(sensorId[i] != 0 && I2C_checkAlive(sensorId[i]) == false) {
        if(sensorErrorChk[i]++ < 3) {
          Serial.print("Warning: Sensor ID ");
          Serial.print(sensorId[i]);
          Serial.println(" is not responding.");
          continue;
        }
        Serial.print("Sensor ID ");
        Serial.print(sensorId[i]);
        Serial.println(" is not alive.");
        sensorId[i] = 0;
        sensorName[i] = "";
      }
      sensorErrorChk[i] = 0;
    }
  }
  if(now -  prevRequestMs> 2000){
    prevRequestMs=now;
    String _json="{";
    _json+="\"id\" : ";
    _json+=String(myAddress);
    for(int i=0; i<MAX_SENSOR_NUM; i++) {
      if(sensorId[i] != 0) {
        Serial.println("inti");
        uint16_t dataLen = I2C_requestDataLength(sensorId[i]);
        if(dataLen == 0) {
          Serial.print("Failed to get data length from Sensor ID ");
          Serial.println(sensorId[i]);
          continue;
        }
        _json+=",";
        I2C_requestData(sensorId[i], &_json, dataLen);
        delay(10);
      }
    }
    _json+="}";
    json = _json;
    Serial.print("json : ");
    Serial.println(json);
  }  
  if(myAddress!=newAddr)
  {
    myAddress=newAddr;
  	Wire.begin(myAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    Serial.print(myAddress);
  	Serial.println("Im Friend!!");
  }
  delay(1);
}


void receiveEvent(int cnt) {
  command = Wire.read();
  
  
  if(command==CMD_STRING_RECEIVE)
  {
    String json = "";

    I2C_handleStringReceive(&json);

    Serial.print("read json : ");
    Serial.println(json);
    
    StaticJsonDocument<200> doc;
    deserializeJson(doc, json);

    
    for(int i=0;i<MAX_SENSOR_NUM;i++)
    {
      if(sensorName[i]=="")
          continue;
      if(doc.containsKey(sensorName[i])==false)
          continue;
      int readData = doc[sensorName[i]];
      Serial.print("sensorName: ");
      Serial.print(sensorName[i]);
      Serial.print(", data: ");
      Serial.println(readData);

      sensorWire.beginTransmission(sensorId[i]);
      sensorWire.write(0xB0);
      sensorWire.write(highByte(readData)); // 상위 바이트
      sensorWire.write(lowByte(readData));  // 하위 바이트
      sensorWire.endTransmission();
    }
  }
}

int sendLeng=0;
String sendData = "";
void requestEvent()
{
  if (command == CMD_REGISTER_NEW_ID && Wire.available()) {
    newAddr = Wire.read();
    Wire.end();
  }  
  else if(command == CMD_REQUEST_DATA_LENGTH)
  {
    sendLeng=0;
    sendData=json;
    int len = sendData.length();

    Wire.write(highByte(len)); // 상위 바이트
    Wire.write(lowByte(len));  // 하위 바이트
  }
  else if(command == CMD_REQUEST_DATA)
  {
  	//Serial.println(sendData);
  	//Serial.println(sendData.length());
    
    for (int i = 0; i < (sendData.length()-sendLeng>32?32:sendData.length()-sendLeng); i++) {
      Wire.write(sendData[i+sendLeng]);
    }
    sendLeng+=32;
  }
  else if(command == CMD_ALIVE_CHECK)
  {
    I2C_sendACK();
    Serial.println("Alive check - ACK sent");
  }
  // 알 수 없는 명령
  else {
    I2C_sendNACK();
    Serial.println("Unknown command - NACK sent");
  }
  
  command=0x00;
  while (Wire.available()) Wire.read(); 
}


#include "I2CProtocol.h"

byte newAddr = 1;
byte myAddress = 1;
int data = 1000;
String buf;
byte command = 0x00;
String sensorName = "led";
bool changeAddress = false;
int sendIndex = 0;
unsigned long addressChangeTime = 0;  // 추가

void setup() {
  Wire.begin(myAddress);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.begin(115200);
  pinMode(6, INPUT);
  analogWrite(6, data);
  Serial.println("Sensor led Started");
}

unsigned long chkTime = 0;
void loop() {
  unsigned long now = millis();
  
  // 주소 변경 처리 - 1초 대기
  if (changeAddress && (now - addressChangeTime > 1000)) {
    changeAddress = false;
    myAddress = newAddr;
    Wire.end();
    delay(10);
    Wire.begin(myAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    Serial.print("Address changed to: 0x");
    Serial.println(myAddress, HEX);
    Serial.println("I'm Friend!!");
  }
  
  delay(1);
}

void receiveEvent(int cnt) {
  command = Wire.read();
  Serial.print("Command received: 0x");
  Serial.println(command, HEX);
  
  // 0xB0: 문자열 수신 (마스터가 센서에 제어 명령 전송)
  if (command == CMD_STRING_RECEIVE) {
        uint8_t hi = Wire.read();
        uint8_t lo = Wire.read();
        int _data = (hi << 8) | lo;
        data = _data;
        analogWrite(6, data);
        Serial.print("Received control data: ");
        Serial.println(data);
  }
}

void requestEvent() {
  Serial.print("Request for command: 0x");
  Serial.println(command, HEX);
  
  // 0xA0: 새로운 ID 등록 및 센서 이름 길이 반환
  if (command == CMD_REGISTER_NEW_ID && Wire.available()) {
    newAddr = Wire.read();
    Serial.print("New address assigned: 0x");
    Serial.println(newAddr, HEX);
    
    int len = sensorName.length();
    I2C_sendDataLength(len);
    
    // 0xA0 명령 후에 주소 변경 예약
    addressChangeTime = millis();
    changeAddress = true;
  }  
  // 0xA4: 센서 이름 전송
  else if (command == CMD_REQUEST_NAME) {
    I2C_sendSensorName(sensorName.c_str());
    Serial.print("Sent sensor name: ");
    Serial.println(sensorName);
  }
  // 0xA1: 데이터 길이 반환
  else if (command == CMD_REQUEST_DATA_LENGTH) {
    sendIndex = 0;
    buf = "";
    buf = "\"" + sensorName + "\" : " + String(data);
    int len = buf.length();
    
    I2C_sendDataLength(len);
    
    Serial.print("Data prepared: ");
    Serial.print(buf);
    Serial.print(", length: ");
    Serial.println(len);
  }
  // 0xA2: 데이터 전송
  else if (command == CMD_REQUEST_DATA) {
    Serial.print("Sending data chunk from index ");
    Serial.print(sendIndex);
    Serial.print(": ");
    
    int remainingBytes = buf.length() - sendIndex;
    int chunkSize = (remainingBytes > MAX_I2C_BUFFER_SIZE) ? MAX_I2C_BUFFER_SIZE : remainingBytes;
    
    I2C_sendData(&buf, sendIndex, chunkSize);
    
    Serial.print(buf.substring(sendIndex, sendIndex + chunkSize));
    Serial.print(" (");
    Serial.print(chunkSize);
    Serial.println(" bytes)");
    
    sendIndex += chunkSize;
  }
  // 0xC0: Alive 체크
  else if (command == CMD_ALIVE_CHECK) {
    I2C_sendACK();
    Serial.println("Alive check - ACK sent");
  }
  // 알 수 없는 명령
  else {
    I2C_sendNACK();
    Serial.println("Unknown command - NACK sent");
  }
  
  command = 0x00;
  while (Wire.available()) Wire.read(); 
}

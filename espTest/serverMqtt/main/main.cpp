#include "Arduino.h"
#include <WiFi.h>
#include <Wire.h>

extern "C" {
    #include "esp_event.h"
    #include "esp_netif.h"
    #include "nvs_flash.h"
    #include "esp_wifi.h"
    #include "esp_websocket_client.h"
}

static const char* WIFI_SSID     = "WIFI";
static const char* WIFI_PASS     = "PW";

// WebSocket 서버
static const char* WS_SERVER_URI = "URI";  // WebSocket 서버 URI

static esp_websocket_client_handle_t ws_client = nullptr;
static unsigned long last_publish = 0;
static const unsigned long PUBLISH_INTERVAL = 10000; // 10초
static bool wifi_connected = false;
static bool ws_connected = false;



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


#define MAX_SENSOR_NUM 5

int plantId[MAX_SENSOR_NUM];
int plantErrorChk[MAX_SENSOR_NUM];
String sensorName[MAX_SENSOR_NUM];
byte idNum = 0x08;
String buf;
String json;

unsigned long prevChkMs = 0;
unsigned long prevAliveMs = 0;
unsigned long prevRequestMs = 0;


void startI2CsMaster()
{
    Wire.begin(8,9);
}
//===================== Master ====================


bool I2C_waitForACK(uint8_t slaveAddr)
{
    Wire.requestFrom(slaveAddr, (uint8_t)1);
    unsigned long startTime = millis();
    while (Wire.available() < 1) {
        if (millis() - startTime > 100) {
            return false;
        }
    }
    uint8_t response = Wire.read();
    return (response == ACK);
}

bool I2C_checkAlive(uint8_t slaveAddr)
{
    Wire.beginTransmission(slaveAddr);
    Wire.write(CMD_ALIVE_CHECK);
    Wire.endTransmission();
    
    return I2C_waitForACK(slaveAddr);
}

bool I2C_chkNewBoard()
{
    return I2C_checkAlive(0x01);
}

uint16_t I2C_registerNewID(uint8_t slaveAddr, uint8_t newAddress, bool readSensorNameLength)
{
    Wire.beginTransmission(slaveAddr);
    Wire.write(CMD_REGISTER_NEW_ID);
    Wire.write(newAddress);
    Wire.endTransmission();
    uint16_t leng=0;
    if(readSensorNameLength)
    {
        Wire.requestFrom(slaveAddr, (uint8_t)2);
        
        if (Wire.available()<2) {
            while(Wire.available()>0)
                Wire.read();
            return 0;
        }

        uint8_t hi = Wire.read();
        uint8_t lo = Wire.read();
        leng = (hi << 8) | lo;
    }
    return leng;
}
uint16_t I2C_requestDataLength(uint8_t slaveAddr)
{
    Wire.beginTransmission(slaveAddr);
    Wire.write(CMD_REQUEST_DATA_LENGTH);
    Wire.endTransmission();
    
    Wire.requestFrom(slaveAddr, (uint8_t)2);
    
    if (Wire.available()<2) {
        while(Wire.available()>0)
            Wire.read();
        return 0;
    }

    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    uint16_t length = (hi << 8) | lo;
    
    return length;
}

int I2C_requestData(uint8_t slaveAddr, String *buffer, uint16_t maxLength)
{
    Wire.beginTransmission(slaveAddr);
    Wire.write(CMD_REQUEST_DATA);
    Wire.endTransmission();
    
    uint16_t bytesRead = 0;
    unsigned long startTime = millis();
    
    while (bytesRead < maxLength) {
        uint16_t chunkSize = maxLength-bytesRead>MAX_I2C_BUFFER_SIZE ? MAX_I2C_BUFFER_SIZE : maxLength-bytesRead;
        uint16_t readByte = 0;
        Wire.requestFrom(slaveAddr, chunkSize);
        
        unsigned long chunkStartTime = millis();
        while (Wire.available() == 0) {
            if (millis() - chunkStartTime > 100) {
                return bytesRead;
            }
        }
        
        while (Wire.available() && readByte < chunkSize) {
            char c = Wire.read();
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
    Wire.beginTransmission(slaveAddr);
    Wire.write(CMD_REQUEST_NAME);
    Wire.endTransmission();
    
    uint16_t bytesRead = 0;
    unsigned long startTime = millis();
    
    while (bytesRead < bufferSize) {
        uint16_t chunkSize = bufferSize-bytesRead>MAX_I2C_BUFFER_SIZE ? MAX_I2C_BUFFER_SIZE : bufferSize-bytesRead;
        uint16_t readByte = 0;
        Wire.requestFrom(slaveAddr, chunkSize);
        
        unsigned long chunkStartTime = millis();
        while (Wire.available() == 0) {
            if (millis() - chunkStartTime > 100) {
                return false;
            }
        }
        
        while (Wire.available() && readByte < chunkSize) {
            uint8_t c = Wire.read();
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



// ====== Wi-Fi 이벤트 핸들러 ======
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("[WiFi] STA_START - Attempting to connect...\n");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("[WiFi] Disconnected, retrying...\n");
        wifi_connected = false;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("[WiFi] Connected! IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

// ====== Wi-Fi 초기화 (ESP-IDF API 사용) ======
void wifi_init_sta()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Wi-Fi 이벤트 핸들러 등록
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    Serial.println("[WiFi] Connecting...");
}

// ====== WebSocket 이벤트 핸들러 ======
static void ws_event_handler(void* handler_args,
                             esp_event_base_t base,
                             int32_t event_id,
                             void* event_data)
{
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*)event_data;
    
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            printf("[WebSocket] Connected\n");
            ws_connected = true;
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            printf("[WebSocket] Disconnected\n");
            ws_connected = false;
            break;
        case WEBSOCKET_EVENT_DATA:
            printf("[WebSocket] Received: %.*s\n", data->data_len, (char*)data->data_ptr);
            break;
        case WEBSOCKET_EVENT_ERROR:
            printf("[WebSocket] Error\n");
            break;
        default:
            break;
    }
}

// ====== WebSocket 초기화 및 시작 ======
void ws_start()
{
    esp_websocket_client_config_t ws_cfg = {};
    ws_cfg.uri = WS_SERVER_URI;

    ws_client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, ws_event_handler, nullptr);
    esp_websocket_client_start(ws_client);
}

// ====== Arduino-style setup / loop ======
void setup()
{
    Serial.begin(115200);
    delay(100);  // 시리얼 안정화 대기
    

    startI2CsMaster();
    for(int i=0;i<MAX_SENSOR_NUM;i++)
    {
        plantId[i]=0;
    }

    for(int i=0;i<MAX_SENSOR_NUM;i++)
    {
        plantId[i]=0;
        plantErrorChk[i]=0;
    }

    
    printf("\n\n");
    printf("=================================\n");
    printf("[APP] Arduino + WebSocket Starting\n");
    printf("=================================\n");

    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        printf("[NVS] Erasing flash...\n");
        nvs_flash_erase();
        nvs_flash_init();
    }
    printf("[NVS] Flash initialized\n");

    // Wi-Fi 시작 (WebSocket은 Wi-Fi 연결 후 loop에서 시작)
    printf("[WiFi] Initializing WiFi...\n");
    printf("[WiFi] SSID: %s\n", WIFI_SSID);
    wifi_init_sta();
    printf("[WiFi] WiFi started\n");
}

void loop()
{
    // Wi-Fi 연결 완료 후 WebSocket 시작
    if (wifi_connected && !ws_connected && ws_client == nullptr) {
        printf("[WebSocket] Starting WebSocket client...\n");
        ws_start();
    }

    unsigned long now = millis();
    // 10초마다 데이터 전송 (Wi-Fi와 WebSocket이 모두 연결된 경우에만)
    // if (wifi_connected && ws_connected && ws_client) {
        
    //     if (now - last_publish >= PUBLISH_INTERVAL) {
    //         last_publish = now;

    //         // JSON 데이터 생성
    //         int right_value = random(0, 1024);  // 0~1023 랜덤
    //         int led_value = random(0, 1001);    // 0~1000 랜덤
            
    //         char json_msg[128];
    //         snprintf(json_msg, sizeof(json_msg), 
    //                  "{\"id\":2,\"right\":%d,\"LED\":%d}", 
    //                  right_value, led_value);
            
    //         // WebSocket으로 전송
    //         if (esp_websocket_client_is_connected(ws_client)) {
    //             int sent = esp_websocket_client_send_text(ws_client, json_msg, strlen(json_msg), portMAX_DELAY);
    //             printf("[WebSocket] Sent %d bytes: %s\n", sent, json_msg);
    //         } else {
    //             printf("[WebSocket] Not connected, skipping send\n");
    //         }
    //     }
    // }
    if(now - prevChkMs > 100) {
        prevChkMs = now;
        if(I2C_chkNewBoard()) {
            printf("New Board Detected!\n");
            uint16_t nameLength = I2C_registerNewID(0x01, idNum, false);
        }
    }
    if(now - prevAliveMs > 3000) {
        prevAliveMs = now;
        for(int i=0; i<MAX_SENSOR_NUM; i++) {
            if(plantId[i] != 0 && I2C_checkAlive(plantId[i]) == false) {
                if(plantErrorChk[i]++ < 3) {
                    printf("Warning: Sensor ID ");
                    printf("%d", plantId[i]);
                    printf(" is not responding.\n");
                    continue;
                }
                printf("Sensor ID ");
                printf("%d", plantId[i]);
                printf(" is not alive.\n");
                plantId[i] = 0;
                sensorName[i] = "";
            }
            plantErrorChk[i] = 0;
        }
    }
    if(now -  prevRequestMs> 10000){
        prevRequestMs=now;
        for(int i=0; i<MAX_SENSOR_NUM; i++) {
            if(plantId[i] != 0) {
                printf("inti\n");
                uint16_t dataLen = I2C_requestDataLength(plantId[i]);
                if(dataLen == 0) {
                    printf("Failed to get data length from Sensor ID ");
                    printf("%d\n", plantId[i]);
                    continue;
                }
                String str="";
                I2C_requestData(plantId[i], &str, dataLen);

                const char* json_msg = str.c_str();
                
                // WebSocket으로 전송
                if (esp_websocket_client_is_connected(ws_client)) {
                    int sent = esp_websocket_client_send_text(ws_client, json_msg, strlen(json_msg), portMAX_DELAY);
                    printf("[WebSocket] Sent %d bytes: %s\n", sent, json_msg);
                } else {
                    printf("[WebSocket] Not connected, skipping send\n");
                }

                printf("Received from Sensor ID %d: %s\n", plantId[i], str.c_str());
                delay(10);
            }
        }
    }

    // Arduino loop는 그냥 짧게 한 바퀴 돌고 yield
    delay(10);
}

// ====== ESP-IDF 진입점: app_main ======
extern "C" void app_main()
{
    // Arduino 런타임 초기화
    initArduino();

    setup();
    while (true) {
        loop();
        vTaskDelay(1); // FreeRTOS 양보
    }
}

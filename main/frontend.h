#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>

#elif defined(ARDUINO_ARCH_ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)

// ESP-IDF environment - no Arduino headers needed
// Using native ESP-IDF components

#else

#pragma message ("Unknown Device!")

#endif

#ifndef FRONTEND_H
#define FRONTEND_H

#include <string>
#include "esp_http_server.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "id_open.h"

// NVS Key 常量统一定义
static const char* NVS_NAMESPACE    = "rid_config";
static const char* NVS_KEY_LAT      = "latitude";
static const char* NVS_KEY_LON      = "longitude";
static const char* NVS_KEY_DRONES   = "num_drones";
static const char* NVS_KEY_INIT     = "initialized";
static const char* NVS_KEY_CAAC_REG = "caac_reg";

class Frontend {
  private:
    httpd_handle_t server = NULL;
    struct FrontendContext* context = NULL;  // 持有上下文指针用于释放
    
    std::string HTML();
    esp_err_t handleOnConnect(httpd_req_t *req);
    esp_err_t handleSetCoords(httpd_req_t *req);
    esp_err_t handleNumDrones(httpd_req_t *req);
    void startSpoof();
    unsigned long maxtime = 0;
    unsigned long timer = 0;

  public:
    Frontend(unsigned long idletime);
    ~Frontend();
    void handleClient();
    bool do_spoof = false;
    double latitude = 52.439100;
    double longitude = -1.503900;
    int num_drones = 16;
    
#if ID_CHINA
    char caac_registration[32] = "123456789012345";  // Default CAAC registration
#endif
};

#endif
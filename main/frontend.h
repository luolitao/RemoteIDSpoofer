#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>

#elif defined(ARDUINO_ARCH_ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

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

class Frontend {
  private:
    httpd_handle_t server = NULL;
    
    std::string HTML();
    esp_err_t handleOnConnect(httpd_req_t *req);
    esp_err_t handleSetCoords(httpd_req_t *req);
    esp_err_t handleNumDrones(httpd_req_t *req);
    void startSpoof();
    unsigned long maxtime = 0;
    unsigned long timer = 0;

    const char* nvs_namespace = "rid_config";
    const char* latitude_key = "latitude";
    const char* longitude_key = "longitude";
    const char* num_drones_key = "num_drones";

  public:
    Frontend(unsigned long idletime);
    ~Frontend();
    void handleClient();
    bool do_spoof = false;
    double latitude = 52.439100;
    double longitude = -1.503900;
    int num_drones = 16;
};

#endif
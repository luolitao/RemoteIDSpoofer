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

// NVS Key 常量统一定义（仅 frontend.cpp 使用，用 extern 声明避免多文件重复定义）
extern const char* NVS_NAMESPACE;
extern const char* NVS_KEY_LAT;
extern const char* NVS_KEY_LON;
extern const char* NVS_KEY_DRONES;
extern const char* NVS_KEY_INIT;

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
};

#endif
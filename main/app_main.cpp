// main/app_main.cpp
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "spoofer.h"
#include "frontend.h"
#include "arduino_compat.h"

static const char *TAG = "RemoteIDSpoofer";

// 最大欺骗器数量
#define MAX_SPOOFERS 16

// 全局变量
int num_spoofers = 0;
Spoofer spoofers[MAX_SPOOFERS];

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting RemoteID Spoofer");

    // 初始化 NVS (替代 EEPROM)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 启动前端并等待用户输入或超时
    Frontend frontend(120000); // 2分钟超时
    
    while (!frontend.do_spoof) {
        frontend.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延迟避免占用过多CPU
    }

    ESP_LOGI(TAG, "Starting Spoofers");
    
    // 初始化欺骗器并更新位置
    num_spoofers = frontend.num_drones;
    for (int i = 0; i < num_spoofers; i++) {
        spoofers[i].init();
        spoofers[i].updateLocation(frontend.latitude, frontend.longitude);
    }

    // 主循环执行欺骗
    while (true) {
        for (int i = 0; i < num_spoofers; i++) {
            spoofers[i].update();
        }
        vTaskDelay(pdMS_TO_TICKS(200 / num_spoofers)); // 动态延迟确保总周期为200ms
    }
}
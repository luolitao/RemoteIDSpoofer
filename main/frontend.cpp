#include "frontend.h"
#include <sstream>
#include <iomanip>
#include "arduino_compat.h"

static const char *TAG = "Frontend";

// HTTP 请求处理器的辅助结构体
struct FrontendContext {
    Frontend* frontend;
};

Frontend::Frontend(unsigned long idletime) 
{
    // 初始化 NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS found, reusing old values...");
        
        size_t required_size = sizeof(double);
        nvs_get_blob(nvs_handle, latitude_key, &latitude, &required_size);
        nvs_get_blob(nvs_handle, longitude_key, &longitude, &required_size);
        
        nvs_get_i32(nvs_handle, num_drones_key, (int32_t*)&num_drones);
        
        nvs_close(nvs_handle);
    } else {
        ESP_LOGI(TAG, "NVS data not written before...");
    }

    // 设置 SoftAP
    ESP_LOGI(TAG, "Setting soft-AP ... ");
    
    // 初始化 TCP/IP 栈和事件循环
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 配置 AP
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "ESP_RIDS");
    strcpy((char*)wifi_config.ap.password, "makkauhijau");
    wifi_config.ap.ssid_len = strlen("ESP_RIDS");
    wifi_config.ap.channel = 6;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Access Point started");

    // 配置 HTTP 服务器
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;

    // 创建上下文
    FrontendContext* context = new FrontendContext();
    context->frontend = this;

    // 注册 URI 处理器
    httpd_uri_t uri_root = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = [](httpd_req_t *req) -> esp_err_t {
            FrontendContext* ctx = (FrontendContext*)req->user_ctx;
            return ctx->frontend->handleOnConnect(req);
        },
        .user_ctx  = context
    };

    httpd_uri_t uri_location = {
        .uri       = "/getlocation",
        .method    = HTTP_GET,
        .handler   = [](httpd_req_t *req) -> esp_err_t {
            FrontendContext* ctx = (FrontendContext*)req->user_ctx;
            return ctx->frontend->handleSetCoords(req);
        },
        .user_ctx  = context
    };

    httpd_uri_t uri_drones = {
        .uri       = "/numdrones",
        .method    = HTTP_GET,
        .handler   = [](httpd_req_t *req) -> esp_err_t {
            FrontendContext* ctx = (FrontendContext*)req->user_ctx;
            return ctx->frontend->handleNumDrones(req);
        },
        .user_ctx  = context
    };

    httpd_uri_t uri_start = {
        .uri       = "/start",
        .method    = HTTP_GET,
        .handler   = [](httpd_req_t *req) -> esp_err_t {
            FrontendContext* ctx = (FrontendContext*)req->user_ctx;
            ctx->frontend->startSpoof();
            httpd_resp_send(req, "Started", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        },
        .user_ctx  = context
    };

    // 启动 HTTP 服务器并注册 URI
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_location);
        httpd_register_uri_handler(server, &uri_drones);
        httpd_register_uri_handler(server, &uri_start);
    }

    // 记录启动时间和超时时间
    timer = esp_timer_get_time() / 1000; // 转换为毫秒
    maxtime = timer + idletime;
}

Frontend::~Frontend() {
    if (server) {
        httpd_stop(server);
    }
}

void Frontend::handleClient() {
    static unsigned long last_log = 0;
    unsigned long now = millis();
    
    if (now - last_log > 10000) {  // 每10秒打印一次
        ESP_LOGI("Frontend", "Waiting for client connection...");
        last_log = now;
    }    // 在 ESP-IDF 中，HTTP 服务器在自己的线程中运行
    // 这里只需要检查超时
    
    unsigned long current_time = esp_timer_get_time() / 1000;
    if (current_time > maxtime && !do_spoof) {
        startSpoof();
    }
}

esp_err_t Frontend::handleOnConnect(httpd_req_t *req) {
    ESP_LOGI(TAG, "Client Connected");
    std::string html = HTML();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

esp_err_t Frontend::handleSetCoords(httpd_req_t *req) {
    char param[256];
    
    // 获取查询字符串
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        char lat_value[32], lon_value[32];
        
        // 解析纬度
        if (httpd_query_key_value(param, "latitude", lat_value, sizeof(lat_value)) == ESP_OK) {
            latitude = atof(lat_value);
        }
        
        // 解析经度
        if (httpd_query_key_value(param, "longitude", lon_value, sizeof(lon_value)) == ESP_OK) {
            longitude = atof(lon_value);
        }
        
        // 保存到 NVS
        nvs_handle_t nvs_handle;
        if (nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle) == ESP_OK) {
            nvs_set_blob(nvs_handle, latitude_key, &latitude, sizeof(latitude));
            nvs_set_blob(nvs_handle, longitude_key, &longitude, sizeof(longitude));
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
        }
    }
    
    std::string html = HTML();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

esp_err_t Frontend::handleNumDrones(httpd_req_t *req) {
    char param[256];
    
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        char drones_value[8];
        
        if (httpd_query_key_value(param, "numdrones", drones_value, sizeof(drones_value)) == ESP_OK) {
            num_drones = atoi(drones_value);
            
            // 保存到 NVS
            nvs_handle_t nvs_handle;
            if (nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle) == ESP_OK) {
                nvs_set_i32(nvs_handle, num_drones_key, num_drones);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
            }
        }
    }
    
    std::string html = HTML();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

void Frontend::startSpoof() {
    do_spoof = true;
    
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    
    // 断开 WiFi AP
    esp_wifi_stop();
    
    // 保存标志到 NVS
    nvs_handle_t nvs_handle;
    if (nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        int32_t initialized = 42;
        nvs_set_i32(nvs_handle, "initialized", initialized);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    
    ESP_LOGI(TAG, "Starting spoofing mode");
}

std::string Frontend::HTML() {
    std::ostringstream msg;
    
    msg << R"rawliteral(
    <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
        <title>Remote ID Spoofer</title>
        <style>
          html{font-family:Helvetica; display:inline-block; margin:0px auto; text-align:center;}
          body{margin-top: 50px;}
          h1{color: #444444; margin: 50px auto 30px; font-size:6em;}
          h3{color:#444444; margin-bottom: 50px;}
          .configurator{font-size:4.0em; margin-bottom: 50px;}
          .selection{font-size:1.0em;}
          .button{display:block; background-color:#f48100; border:none; color:white; padding: 13px 30px; text-decoration:none; font-size:6em; margin: 0px auto 35px; cursor:pointer; border-radius:4px;}
          .button-on{background-color:#f48100;}
          .button-on:active{background-color:#f48100;}
          .button-off{background-color:#26282d;}
          .button-off:active{background-color:#26282d;}
        </style>
      </head>
      <body onload="updateTime();">
        <h1>Remote ID Spoofer</h1>
        <form class="configurator" action="/getlocation">
          Latitude: <input class="selection" type="text" name="latitude">
          <br>
          Longitude: <input class="selection" type="text" name="longitude">
          <br>
          <input class="selection" type="submit" value="Submit">
        </form>
        <form class="configurator" action="/numdrones">
          No. of Drones:
          <select class="selection" name="numdrones">
    )rawliteral";

    for (int i = 1; i <= 16; i++) {
        msg << "<option value=\"" << i << "\">" << i << "</option>\n";
    }

    msg << R"rawliteral(
          </select>
          <input class="selection" type="submit" value="Submit">
        </form>
    )rawliteral";

    msg << "<p><b>Current Coordinates:</b><br>";
    msg << std::fixed << std::setprecision(10) << latitude;
    msg << ", ";
    msg << std::fixed << std::setprecision(10) << longitude;
    msg << "</p>\n";

    msg << "<p><b>Current No. of Drones:</b>";
    msg << num_drones;
    msg << "</p>\n";

    msg << R"rawliteral(
        <a class="button button-on" href="/start">Start Spoofing</a>
        <p>Pressing this button will cause the device to turn off the web server and enter spoofing only mode.
        Please confirm your GPS coordinates before doing so.
        You will not be able to reconnect to this page without a power cycle.</p>
      <script>
        function updateTime() {
          var today = new Date();
          return [today.getHours(), today.getMinutes(), today.getSeconds()];
        }
      </script>
    </body>
    </html>
    )rawliteral";

    return msg.str();
}
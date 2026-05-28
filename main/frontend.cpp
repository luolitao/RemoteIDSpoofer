#include "frontend.h"
#include <sstream>
#include <iomanip>
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "Frontend";

// HTTP 请求处理器的辅助结构体
struct FrontendContext {
    Frontend* frontend;
};

Frontend::Frontend(unsigned long idletime) 
{
    // 初始化 NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS found, reusing old values...");
        
        size_t required_size = sizeof(double);
        nvs_get_blob(nvs_handle, NVS_KEY_LAT, &latitude, &required_size);
        nvs_get_blob(nvs_handle, NVS_KEY_LON, &longitude, &required_size);
        
        nvs_get_i32(nvs_handle, NVS_KEY_DRONES, (int32_t*)&num_drones);
        
#if ID_CHINA
        char caac_reg[32];
        size_t reg_size = sizeof(caac_reg);
        if (nvs_get_str(nvs_handle, NVS_KEY_CAAC_REG, caac_reg, &reg_size) == ESP_OK) {
            strncpy(caac_registration, caac_reg, sizeof(caac_registration) - 1);
            caac_registration[sizeof(caac_registration) - 1] = '\0';
        }
#endif
        
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

    // 创建上下文（析构函数中释放）
    context = new FrontendContext();
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
    if (context) {
        delete context;
        context = NULL;
    }
}

void Frontend::handleClient() {
    // 在 ESP-IDF 中，HTTP 服务器在自己的线程中运行
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
        
#if ID_CHINA
        char caac_reg_value[32];
        if (httpd_query_key_value(param, "caac_reg", caac_reg_value, sizeof(caac_reg_value)) == ESP_OK) {
            strncpy(caac_registration, caac_reg_value, sizeof(caac_registration) - 1);
            caac_registration[sizeof(caac_registration) - 1] = '\0';
        }
#endif
        
        // 保存到 NVS
        nvs_handle_t nvs_handle;
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
            nvs_set_blob(nvs_handle, NVS_KEY_LAT, &latitude, sizeof(latitude));
            nvs_set_blob(nvs_handle, NVS_KEY_LON, &longitude, sizeof(longitude));
            
#if ID_CHINA
            nvs_set_str(nvs_handle, NVS_KEY_CAAC_REG, caac_registration);
#endif
            
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
            if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
                nvs_set_i32(nvs_handle, NVS_KEY_DRONES, num_drones);
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
    
    // 完全停止并反初始化 WiFi，后续 Spoofer 会重新初始化
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // 保存标志到 NVS
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        int32_t initialized = 42;
        nvs_set_i32(nvs_handle, NVS_KEY_INIT, initialized);
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
    )rawliteral";

#if ID_CHINA
    msg << R"rawliteral(
          CAAC Registration: <input class="selection" type="text" name="caac_reg" maxlength="15" pattern="[0-9]{15}">
          <br>
    )rawliteral";
#endif

    msg << R"rawliteral(
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

#if ID_CHINA
    msg << "<p><b>CAAC Registration:</b>";
    msg << caac_registration;
    msg << "</p>\n";
#endif

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
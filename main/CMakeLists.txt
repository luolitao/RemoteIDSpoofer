# main/CMakeLists.txt
idf_component_register(SRCS 
    "app_main.cpp"
    "spoofer.cpp"
    "frontend.cpp"
    "id_open.cpp"
    "id_open_beacon.cpp"
    "id_open_esp32.cpp"
    "utm.cpp"
    "opendroneid.c"
    "wifi.c"
    "alt_unix_time.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_http_server nvs_flash esp_netif esp_event esp_timer)
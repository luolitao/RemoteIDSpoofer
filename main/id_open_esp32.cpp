

#define DIAGNOSTICS 1

//

#pragma GCC diagnostic warning "-Wunused-variable"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"

#include "id_open.h"

#if ID_OD_WIFI

#include "esp_system.h"
#include "esp_event.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx,const void *buffer,int len,bool en_sys_seq);

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#if ESP32_WIFI_OPTION == 0
static const char          *password = "password";
#endif

#endif // WIFI

#if ID_OD_BT

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

static esp_ble_adv_data_t   advData;
static esp_ble_adv_params_t advParams;

#endif // BT

static void              *Debug_Serial = NULL;

/*
 *
 */

void construct2() {

#if ID_OD_BT

  memset(&advData,0,sizeof(advData));

  advData.set_scan_rsp        = false;
  advData.include_name        = false;
  advData.include_txpower     = false;
  advData.min_interval        = 0x0006;
  advData.max_interval        = 0x0050;
  advData.flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

  memset(&advParams,0,sizeof(advParams));

  advParams.adv_int_min       = 0x0020;
  advParams.adv_int_max       = 0x0040;
  advParams.adv_type          = ADV_TYPE_IND;
  advParams.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
  advParams.channel_map       = ADV_CHNL_ALL;
  advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
  advParams.peer_addr_type    = BLE_ADDR_TYPE_PUBLIC;

#endif // ID_OD_BT

  return;
}

/*
 *
 */

void init2(char *ssid, int channel, uint8_t *mac, uint8_t power) {

  int  status;
  char text[128];

  status  = 0;
  text[0] = text[63] = 0;

#if DIAGNOSTICS
  // Debug_Serial = &Serial;
#endif

#if ID_OD_WIFI

  int8_t                wifi_power;
  wifi_config_t         ap_config;
  static wifi_country_t country = {"CN",1,13,20,WIFI_COUNTRY_POLICY_AUTO};

  memset(&ap_config,0,sizeof(ap_config));
  
#if ESP32_WIFI_OPTION

  // WiFi.softAP(ssid,"password",wifi_channel);

  esp_wifi_get_config(WIFI_IF_AP,&ap_config);
  
  // ap_config.ap.ssid_hidden = 1;
  status = esp_wifi_set_config(WIFI_IF_AP,&ap_config);

#else
  
  // Frontend 已通过 esp_wifi_deinit() 释放资源，现在重新初始化
  wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGW("ID_OPEN", "Event loop create: %s (may already exist)", esp_err_to_name(err));
  }
  
  err = esp_wifi_init(&init_cfg);
  if (err != ESP_OK) {
      ESP_LOGE("ID_OPEN", "esp_wifi_init failed: %s", esp_err_to_name(err));
  }
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);

  strcpy((char *) ap_config.ap.ssid,ssid);
  ap_config.ap.ssid_len        = strlen(ssid);
  
  // 确保频道在有效范围内 (1-13 for CN)
  if (channel < 1 || channel > 13) {
      ESP_LOGW("ID_OPEN", "Invalid channel %d, using default channel 6", channel);
      channel = 6;
  }
  ap_config.ap.channel         = (uint8_t) channel;
  ap_config.ap.authmode        = WIFI_AUTH_OPEN;
  ap_config.ap.ssid_hidden     = 0;   // SSID 必须可见，Remote ID Beacon 依赖完整帧结构
  ap_config.ap.max_connection  = 0;
  ap_config.ap.beacon_interval = 100; // 100 TU = 102.4ms，与自构造 Beacon 帧保持一致
  
  esp_wifi_set_config(WIFI_IF_AP,&ap_config);
  esp_wifi_start();
  esp_wifi_set_ps(WIFI_PS_NONE);

#endif

  esp_wifi_set_country(&country);
  status = esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW20);

  // esp_wifi_set_max_tx_power(78);
  esp_wifi_get_max_tx_power(&wifi_power);
  
  ESP_LOGI("ID_OPEN", "WiFi AP initialized: channel=%d ssid=%s tx_power=%d dBm country=%s",
           channel, ssid, (int)((wifi_power + 2) / 4), country.cc);

  if (Debug_Serial) {
    
    sprintf(text,"mac address:     %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    // Debug_Serial->print(text);
// power <= 72, dbm = power/4, but 78 = 20dbm. 
    sprintf(text,"max_tx_power():  %d dBm\r\n",(int) ((wifi_power + 2) / 4));
    // Debug_Serial->print(text);
    sprintf(text,"wifi country:    %s\r\n",country.cc);
    // Debug_Serial->print(text);
  }

#endif // WIFI

#if ID_OD_BT

  int               power_db; 
  esp_power_level_t power;

  // BLEDevice::init(ssid);
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();

  // Using BLEDevice::setPower() seems to have no effect. 
  // ESP_PWR_LVL_N12 ...  ESP_PWR_LVL_N0 ... ESP_PWR_LVL_P9

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P9); 
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,ESP_PWR_LVL_P9); 

  power    = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
  power_db = 3 * ((int) power - 4); 

#endif

  return;
}

/*
 * Processor dependent bits for the wifi frame header.
 */

uint8_t *capability() {

  // 0x21 = ESS | Short preamble
  // 0x04 = Short slot time

  static uint8_t capa[2] = {0x21,0x04};
  
  return capa;
}

//

int tag_rates(uint8_t *beacon_frame,int beacon_offset) {

  beacon_frame[beacon_offset++] = 0x01;
  beacon_frame[beacon_offset++] = 0x08;
  beacon_frame[beacon_offset++] = 0x8b; //  5.5
  beacon_frame[beacon_offset++] = 0x96; // 11
  beacon_frame[beacon_offset++] = 0x82; //  1
  beacon_frame[beacon_offset++] = 0x84; //  2
  beacon_frame[beacon_offset++] = 0x0c; //  6
  beacon_frame[beacon_offset++] = 0x18; // 12 
  beacon_frame[beacon_offset++] = 0x30; // 24
  beacon_frame[beacon_offset++] = 0x60; // 48

  return beacon_offset;
}

//

int tag_ext_rates(uint8_t *beacon_frame,int beacon_offset) {

  beacon_frame[beacon_offset++] = 0x32;
  beacon_frame[beacon_offset++] = 0x04;
  beacon_frame[beacon_offset++] = 0x6c; // 54 
  beacon_frame[beacon_offset++] = 0x12; //  9 
  beacon_frame[beacon_offset++] = 0x24; // 18 
  beacon_frame[beacon_offset++] = 0x48; // 36 

  return beacon_offset;
}

//

int misc_tags(uint8_t *beacon_frame,int beacon_offset) {

  // HT Capabilities (tag 45) - 很多接收方期望看到此 tag
  beacon_frame[beacon_offset++] = 0x2d; // Tag: HT Capabilities
  beacon_frame[beacon_offset++] = 0x1a; // Length: 26
  beacon_frame[beacon_offset++] = 0x6e; // HT Capabilities Info (L)
  beacon_frame[beacon_offset++] = 0x00; // HT Capabilities Info (H)
  beacon_frame[beacon_offset++] = 0x11; // A-MPDU Parameters
  // Supported MCS Set (16 bytes)
  beacon_frame[beacon_offset++] = 0xff; // Rx MCS Bitmask (bytes 0-3)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00; // Rx highest supported data rate (L)
  beacon_frame[beacon_offset++] = 0x00; // Rx highest supported data rate (H)
  beacon_frame[beacon_offset++] = 0x00; // Tx Parameters
  beacon_frame[beacon_offset++] = 0x00; // Reserved
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  // HT Extended Capabilities (2 bytes)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  // Transmit Beamforming Capabilities (4 bytes)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  // ASEL Capabilities (1 byte)
  beacon_frame[beacon_offset++] = 0x00;

  // HT Information (tag 61)
  beacon_frame[beacon_offset++] = 0x3d; // Tag: HT Information
  beacon_frame[beacon_offset++] = 0x16; // Length: 22
  beacon_frame[beacon_offset++] = 0x06; // Primary Channel (channel 6)
  // HT Information Set 1 (1 byte)
  beacon_frame[beacon_offset++] = 0x01;
  // HT Information Set 2 (2 bytes)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  // HT Information Set 3 (2 bytes)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  // Basic MCS Set (16 bytes, same as above)
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;

  return beacon_offset;
}

/*
 *
 */

int transmit_wifi2(uint8_t *buffer,int length) {

  esp_err_t wifi_status = 0;

#if ID_OD_WIFI

  if (length) {

    wifi_status = esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true);  
  }

#endif

  return (int) wifi_status;
}

/*
 *
 */

int transmit_ble2(uint8_t *ble_message,int length) {

  esp_err_t  ble_status = 0;

#if ID_OD_BT

  static int advertising = 0; 

  if (advertising) {

    ble_status = esp_ble_gap_stop_advertising();
  }

  ble_status = esp_ble_gap_config_adv_data_raw(ble_message,length); 
  ble_status = esp_ble_gap_start_advertising(&advParams);

  advertising = 1;

#endif // BT

  return (int) ble_status;
}

/*
 *
 */

#if ID_OD_WIFI

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  // no-op handler (kept for compatibility)
}

#endif

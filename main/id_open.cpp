
#define DIAGNOSTICS 1

//

#pragma GCC diagnostic warning "-Wunused-variable"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"

extern "C" {
  int      clock_gettime(clockid_t,struct timespec *);
  uint64_t alt_unix_secs(int,int,int,int,int,int);
}

#include "id_open.h"

// Disable all Debug_Serial calls for ESP-IDF
#define DEBUG_PRINT(x) do {} while(0)

/*
 *
 */

ID_OpenDrone::ID_OpenDrone() {

  int                i;
  static const char *dummy = "";

  //

  UAS_operator = (char *) dummy;

#if ID_OD_WIFI

  // scrambled, not poached
  // Nodemcu doesn't like certain mac addresses
  // setting the first value to 0 seems to solve this
  WiFi_mac_addr[0] = 0;
  for (int i = 1; i < 6; i++) {
    // WiFi_mac_addr[i] = (uint8_t) (rand() % 100 + 100);
    WiFi_mac_addr[i] = (uint8_t) (esp_random() % 256);
  }
  
  memset(ssid,0,sizeof(ssid));

  strcpy(ssid,"UAS_ID_OPEN");

  beacon_interval = 10;
  
#if ID_OD_WIFI_BEACON

  // If ODID_PACK_MAX_MESSAGES == 10, then the potential size of the beacon message is > 255.
  
#if ODID_PACK_MAX_MESSAGES > 9
#undef ODID_PACK_MAX_MESSAGES
#define ODID_PACK_MAX_MESSAGES 9
#endif
  
  memset(beacon_frame,0,BEACON_FRAME_SIZE);

#if !USE_BEACON_FUNC

  beacon_counter   =
  beacon_length    =
  beacon_timestamp =
  beacon_seq       =
  beacon_payload   = beacon_frame;

#endif

#endif

#endif

  memset(msg_counter,0,sizeof(msg_counter));
  
  //
  // Below '// 0' indicates where we are setting 0 to 0 for clarity.
  //
  
  memset(&UAS_data,0,sizeof(ODID_UAS_Data));

  basicID_data    = &UAS_data.BasicID[0];
  location_data   = &UAS_data.Location;
  selfID_data     = &UAS_data.SelfID;
  system_data     = &UAS_data.System;
  operatorID_data = &UAS_data.OperatorID;

  for (i = 0; i < ODID_AUTH_MAX_PAGES; ++i) {

    auth_data[i] = &UAS_data.Auth[i];

    auth_data[i]->DataPage = i;
    auth_data[i]->AuthType = ODID_AUTH_NONE; // 0
  }
  
  UAS_data.BasicID[0].IDType        = ODID_IDTYPE_NONE; // 0
  UAS_data.BasicID[0].UAType        = ODID_UATYPE_NONE; // 0
  UAS_data.BasicID[1].IDType        = ODID_IDTYPE_NONE; // 0
  UAS_data.BasicID[1].UAType        = ODID_UATYPE_NONE; // 0

  odid_initLocationData(location_data);

  location_data->Status             = ODID_STATUS_UNDECLARED; // 0
  location_data->SpeedVertical      = INV_SPEED_V;
  location_data->HeightType         = ODID_HEIGHT_REF_OVER_TAKEOFF;
  location_data->HorizAccuracy      = ODID_HOR_ACC_10_METER;
  location_data->VertAccuracy       = ODID_VER_ACC_10_METER;
  location_data->BaroAccuracy       = ODID_VER_ACC_10_METER;
  location_data->SpeedAccuracy      = ODID_SPEED_ACC_10_METERS_PER_SECOND;
  location_data->TSAccuracy         = ODID_TIME_ACC_1_5_SECOND;

  selfID_data->DescType             = ODID_DESC_TYPE_TEXT;
  strcpy(selfID_data->Desc,"Recreational");

  odid_initSystemData(system_data);

  system_data->OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
  system_data->ClassificationType   = ODID_CLASSIFICATION_TYPE_EU;
  system_data->AreaCount            = 1;
  system_data->AreaRadius           = 500;
  system_data->AreaCeiling          =
  system_data->AreaFloor            = -1000.0;
  system_data->CategoryEU           = ODID_CATEGORY_EU_SPECIFIC;
  system_data->ClassEU              = ODID_CLASS_EU_UNDECLARED;
  system_data->OperatorAltitudeGeo  = -1000.0;

  operatorID_data->OperatorIdType   = ODID_OPERATOR_ID;

  //

  construct2();
  
  return;
}

/*
 *
 */

void ID_OpenDrone::init(struct UTM_parameters *parameters) {

  int  i;
  char text[128];

  text[0] = text[63] = 0;

#if DIAGNOSTICS
  // Debug_Serial is disabled in ESP-IDF version
#endif

  // operator

  UAS_operator = parameters->UAS_operator;

  strncpy(operatorID_data->OperatorId,parameters->UAS_operator,ODID_ID_SIZE);
  operatorID_data->OperatorId[sizeof(operatorID_data->OperatorId) - 1] = 0;

  // basic

  UAS_data.BasicID[0].UAType        = (ODID_uatype_t) parameters->UA_type;
  UAS_data.BasicID[1].UAType        = (ODID_uatype_t) parameters->UA_type;

#if ID_NATIONAL

  init_national(parameters);
  
#else
  
  UAS_data.BasicID[0].IDType        = (ODID_idtype_t) parameters->ID_type;
  UAS_data.BasicID[1].IDType        = (ODID_idtype_t) parameters->ID_type2;

  switch(basicID_data->IDType) {

  case ODID_IDTYPE_SERIAL_NUMBER:

    strncpy(basicID_data->UASID,parameters->UAV_id,ODID_ID_SIZE);
    break;

  case ODID_IDTYPE_CAA_REGISTRATION_ID:

    strncpy(basicID_data->UASID,parameters->UAS_operator,ODID_ID_SIZE);
    break;
    
  case ODID_IDTYPE_NONE:
  case ODID_IDTYPE_UTM_ASSIGNED_UUID:
  case ODID_IDTYPE_SPECIFIC_SESSION_ID:
  default:
    // Handle other cases or leave as default
    break;
  }
  
  basicID_data->UASID[sizeof(basicID_data->UASID) - 1] = 0;

#endif
  
  // system

  if (parameters->region < 2) {

    system_data->ClassificationType = (ODID_classification_type_t) parameters->region;
  }

  if (parameters->EU_category < 4) {

    system_data->CategoryEU = (ODID_category_EU_t) parameters->EU_category;
  }

  if (parameters->EU_class < 8) {

    system_data->ClassEU = (ODID_class_EU_t) parameters->EU_class;
  }

  //

  encodeBasicIDMessage(&basicID_enc[0],&UAS_data.BasicID[0]);
  encodeBasicIDMessage(&basicID_enc[1],&UAS_data.BasicID[1]);
  encodeLocationMessage(&location_enc,location_data);
  encodeAuthMessage(&auth_enc,auth_data[0]);
  encodeSelfIDMessage(&selfID_enc,selfID_data);
  encodeSystemMessage(&system_enc,system_data);
  encodeOperatorIDMessage(&operatorID_enc,operatorID_data);

  // 关键修复：encode 函数不会设置 Valid 标志，但 odid_message_build_pack 需要它们
  // 才能将消息打包到 Beacon 帧中
  UAS_data.BasicIDValid[0]  = (basicID_data->IDType != ODID_IDTYPE_NONE) ? 1 : 0;
  UAS_data.BasicIDValid[1]  = (UAS_data.BasicID[1].IDType != ODID_IDTYPE_NONE) ? 1 : 0;
  UAS_data.LocationValid    = 1;
  UAS_data.SelfIDValid      = (selfID_data->DescType != ODID_DESC_TYPE_TEXT || selfID_data->Desc[0]) ? 1 : 0;
  UAS_data.SystemValid      = 1;
  UAS_data.OperatorIDValid  = (operatorID_data->OperatorId[0] != 0) ? 1 : 0;
  for (int i = 0; i < ODID_AUTH_MAX_PAGES; ++i) {
    UAS_data.AuthValid[i]   = (auth_data[i]->AuthType != ODID_AUTH_NONE) ? 1 : 0;
  }

  // 添加调试日志
  ESP_LOGI("ODID", "Init: UAV_id=[%s] UAS_op=[%s] ID_type=%d UA_type=%d",
           parameters->UAV_id, parameters->UAS_operator, 
           parameters->ID_type, parameters->UA_type);
  ESP_LOGI("ODID", "BasicID[0]: IDType=%d UASID=[%s]",
           UAS_data.BasicID[0].IDType, UAS_data.BasicID[0].UASID);

  //

  if (UAS_operator[0]) {

    strncpy(ssid,UAS_operator,i = sizeof(ssid)); ssid[i - 1] = 0;
  }

  ssid_length = strlen(ssid);

  init2(ssid, wifi_channel, WiFi_mac_addr, 0);

#if ID_OD_WIFI

#if ID_OD_WIFI_BEACON && !USE_BEACON_FUNC

  init_beacon();

  // payload
  beacon_payload      = &beacon_frame[beacon_offset];
  beacon_offset      += 7;

  *beacon_payload++   = 0xdd;
  beacon_length       = beacon_payload++;

  *beacon_payload++   = 0xfa;
  *beacon_payload++   = 0x0b;
  *beacon_payload++   = 0xbc;

  *beacon_payload++   = 0x0d;
  beacon_counter      = beacon_payload++;

  beacon_max_packed   = BEACON_FRAME_SIZE - beacon_offset - 2;

  if (beacon_max_packed > (ODID_PACK_MAX_MESSAGES * ODID_MESSAGE_SIZE)) {

    beacon_max_packed = (ODID_PACK_MAX_MESSAGES * ODID_MESSAGE_SIZE);
  }
  
#endif

#endif

  return;
}

/*
 *
 */

void ID_OpenDrone::set_self_id(char *self_id) {

  memset(selfID_data->Desc,0,ODID_STR_SIZE + 1);
  strncpy(selfID_data->Desc,self_id,ODID_STR_SIZE);

  encodeSelfIDMessage(&selfID_enc,selfID_data);

  return;
}

/*
 *  These authentication functions need reviewing to make sure that they 
 *  comply with opendroneid release 2.0.
 */

void ID_OpenDrone::set_auth(char *auth) {

  set_auth((uint8_t *) auth,strlen(auth),0x0a);

  return;
}

//

void ID_OpenDrone::set_auth(uint8_t *auth,short int len,uint8_t type) {

  int      i, j;
  char     text[160];
  uint8_t  check[32];

  auth_page_count = 1;

  if (len > MAX_AUTH_LENGTH) {

    len       = MAX_AUTH_LENGTH;
    auth[len] = 0;
  }
  
  auth_data[0]->AuthType = (ODID_authtype_t) type;

  for (i = 0; (i < 17)&&(auth[i]); ++i) {

    check[i]                  =
    auth_data[0]->AuthData[i] = auth[i];
  }
  
  check[i]                  = 
  auth_data[0]->AuthData[i] = 0;
  
  if (Debug_Serial) {

    sprintf(text,"Auth. Code \'%s\' (%d)\r\n",auth,len);
    // // Debug_Serial->print(text);

    sprintf(text,"Page 0 \'%s\'\r\n",check);
    // // Debug_Serial->print(text);
  }

  if (len > 16) {

    for (auth_page_count = 1; (auth_page_count < ODID_AUTH_MAX_PAGES)&&(i < len); ++auth_page_count) {

      auth_data[auth_page_count]->AuthType = (ODID_authtype_t) type;

      for (j = 0; (j < 23)&&(i < len); ++i, ++j) {

        check[j]                                = 
        auth_data[auth_page_count]->AuthData[j] = auth[i];
      }

      if (j < 23) {

        auth_data[auth_page_count]->AuthData[j] = 0;
      }
      
      check[j] = 0;
      
      if (Debug_Serial) {

        sprintf(text,"Page %d \'%s\'\r\n",auth_page_count,check);
        // // Debug_Serial->print(text);
      }
    }

    len = i;
  }

  auth_data[0]->LastPageIndex = (auth_page_count) ? auth_page_count - 1: 0;
  auth_data[0]->Length        = len;

#if not defined(ARDUINO_ARCH_NRF52)
  time_t   secs;

  time(&secs);
  
  auth_data[0]->Timestamp     = (uint32_t) (secs - ID_OD_AUTH_DATUM);
#else
  auth_data[0]->Timestamp     = 0;
#endif

  if (Debug_Serial) {

    sprintf(text,"%d pages\r\n",auth_page_count);
    // // Debug_Serial->print(text);
  }

  return;
}

/*
 *
 */

int ID_OpenDrone::transmit(UTM_data *utm) {
  int status = 0;

  UAS_data.Location.Status = ODID_STATUS_AIRBORNE;
  UAS_data.Location.Direction = (float)utm->heading;
  UAS_data.Location.SpeedHorizontal = (float)utm->speed_kn * 0.514444f;
  UAS_data.Location.SpeedVertical = (float)utm->vel_D_cm / 100.0f;
  UAS_data.Location.Latitude = utm->latitude_d;
  UAS_data.Location.Longitude = utm->longitude_d;
  UAS_data.Location.AltitudeGeo = utm->alt_msl_m;
  UAS_data.Location.AltitudeBaro = utm->alt_agl_m;
  UAS_data.Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
  UAS_data.Location.Height = utm->alt_agl_m;
  UAS_data.Location.HorizAccuracy = ODID_HOR_ACC_10_METER;
  UAS_data.Location.VertAccuracy = ODID_VER_ACC_10_METER;
  UAS_data.Location.BaroAccuracy = ODID_VER_ACC_10_METER;
  UAS_data.Location.SpeedAccuracy = ODID_SPEED_ACC_10_METERS_PER_SECOND;
  UAS_data.Location.TSAccuracy = ODID_TIME_ACC_1_5_SECOND;
  UAS_data.Location.TimeStamp = (float)(utm->seconds % 3600) + (float)utm->csecs / 100.0f;

  static int tx_count = 0;
  tx_count++;
  if (tx_count % 50 == 0) {
    ESP_LOGI("ODID", "Beacon TX #%d: Alt=%.1fm Speed=%.1fm/s ID=[%s] IDType=%d",
             tx_count, utm->alt_msl_m, UAS_data.Location.SpeedHorizontal,
             UAS_data.BasicID[0].UASID, UAS_data.BasicID[0].IDType);
  }

#if ID_OD_WIFI_BEACON
  status = transmit_wifi(utm, 0);
#endif

  return status;
}

/*
 *
 */

int ID_OpenDrone::transmit_wifi(struct UTM_data *utm_data,int prepacked) {

#if ID_OD_WIFI

  int             length = 0, wifi_status = 0;
  uint64_t        usecs = 0;
  char text[128];

  text[0] = 0;
  
  //
  
  if (++sequence > 0xfff) {

    sequence = 1;
  }

  msecs         = (uint32_t)(esp_timer_get_time() / 1000);
  wifi_interval = msecs - last_wifi;
  last_wifi     = msecs;
  
#if not (defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_NRF52))
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME,&ts);
  usecs = (uint64_t)((double) ts.tv_sec * 1e6 + (double) ts.tv_nsec * 1e-3);
#else
  usecs = (uint64_t)esp_timer_get_time();
#endif

#if ID_OD_WIFI_NAN

  uint8_t        buffer[1024];
  static uint8_t send_counter = 0;

  if ((length = odid_wifi_build_nan_sync_beacon_frame((char *) WiFi_mac_addr,
                                                      buffer,sizeof(buffer))) > 0) {

    wifi_status = transmit_wifi2(buffer,length);
  }
    
  if ((Debug_Serial)&&((length < 0)||(wifi_status != 0))) {

    sprintf(text,"odid_wifi_build_nan_sync_beacon_frame() = %d, transmit_wifi2() = %d\r\n",
            length,(int) wifi_status);
    // // Debug_Serial->print(text);
  }

  if ((length = odid_wifi_build_message_pack_nan_action_frame(&UAS_data,(char *) WiFi_mac_addr,
                                                              ++send_counter,
                                                              buffer,sizeof(buffer))) > 0) {

    wifi_status = transmit_wifi2(buffer,length);
  }

  if (Debug_Serial) {

    if ((length < 0)||(wifi_status != 0)) {

      sprintf(text,"odid_wifi_build_message_pack_nan_action_frame() = %d, transmit_wifi2() = %d\r\n",
              length,(int) wifi_status);
      // // Debug_Serial->print(text);

#if DIAGNOSTICS

    } else {

      sprintf(text,"ID_OpenDrone::%s ... ",__func__);
      // // Debug_Serial->print(text);
      
      for (int i = 0; i < 32; ++i) {

        sprintf(text,"%02x ",buffer[16 + i]);
        // // Debug_Serial->print(text);      
      }

      // // Debug_Serial->print(" ... \r\n");

#endif
    }
  }

#endif // NAN
  
#if ID_OD_WIFI_BEACON

#if USE_BEACON_FUNC

  if ((length = odid_wifi_build_message_pack_beacon_frame(&UAS_data,(char *) WiFi_mac_addr,
                                                          ssid,ssid_length,
                                                          beacon_interval,++beacon_counter,
                                                          beacon_frame,BEACON_FRAME_SIZE)) > 0) {

    wifi_status = transmit_wifi2(beacon_frame,length);
  }

#if DIAGNOSTICS && 1

  if (Debug_Serial) {

    sprintf(text,"ID_OpenDrone::%s * %02x ... ",__func__,beacon_frame[0]);
    // // Debug_Serial->print(text);

    for (int i = 0; i < 20; ++i) {

      sprintf(text,"%02x ",beacon_frame[22 + i]);
      // // Debug_Serial->print(text);      
    }

    // // Debug_Serial->print(" ... *\r\n");
  }

#endif // DIAG

#else
  
  int i, len2 = 0;

  ++*beacon_counter;

  for (i = 0; i < 8; ++i) {

    beacon_timestamp[i] = (usecs >> (i * 8)) & 0xff;
  }

#if 1
  beacon_seq[0] = (uint8_t) (sequence << 4);
  beacon_seq[1] = (uint8_t) (sequence >> 4);
#endif

  length = (prepacked > 0) ? prepacked:
                             odid_message_build_pack(&UAS_data,beacon_payload,beacon_max_packed);

  if (length > 0) {
    *beacon_length = length + 5;
    wifi_status = transmit_wifi2(beacon_frame,len2 = beacon_offset + length);
  }

  // 详细诊断日志 - 每50次发送打印一次帧结构
  static int diag_count = 0;
  if (++diag_count % 50 == 0) {
    int vendor_ie_offset = beacon_offset - 7;  // beacon_payload 实际写入的起始位置
    ESP_LOGI("ODID", "Beacon frame: total_len=%d payload_len=%d wifi_status=%d", 
             len2, length, wifi_status);
    ESP_LOGI("ODID", "Beacon header[0]=0x%02x (should be 0x80 for Beacon)", beacon_frame[0]);
    ESP_LOGI("ODID", "BSSID: %02x:%02x:%02x:%02x:%02x:%02x (should be our MAC)",
             beacon_frame[16], beacon_frame[17], beacon_frame[18],
             beacon_frame[19], beacon_frame[20], beacon_frame[21]);
    ESP_LOGI("ODID", "Vendor IE offset=%d: tag=0x%02x len=0x%02x OUI=%02x:%02x:%02x (should be dd:??:fa:0b:bc)",
             vendor_ie_offset,
             beacon_frame[vendor_ie_offset], beacon_frame[vendor_ie_offset + 1],
             beacon_frame[vendor_ie_offset + 2], beacon_frame[vendor_ie_offset + 3],
             beacon_frame[vendor_ie_offset + 4]);
    ESP_LOGI("ODID", "ODID msg type: 0x%02x counter: 0x%02x", 
             beacon_frame[vendor_ie_offset + 5], beacon_frame[vendor_ie_offset + 6]);
    ESP_LOGI("ODID", "Valid flags: BasicID[0]=%d Location=%d SelfID=%d System=%d Oper=%d",
             UAS_data.BasicIDValid[0], UAS_data.LocationValid, 
             UAS_data.SelfIDValid, UAS_data.SystemValid, UAS_data.OperatorIDValid);
  }

#if DIAGNOSTICS && 1

  if (Debug_Serial) {

    sprintf(text,"ID_OpenDrone::%s %d %d+%d=%d ",
            __func__,beacon_max_packed,beacon_offset,length,len2);
    // // Debug_Serial->print(text);

    sprintf(text,"* %02x ... ",beacon_frame[0]);
    // // Debug_Serial->print(text);

    for (int i = 0; i < 16; ++i) {

      if ((i == 3)||(i == 10)) {

        // // Debug_Serial->print("| ");
      }

      sprintf(text,"%02x ",beacon_frame[beacon_offset - 10 + i]);
      // // Debug_Serial->print(text);
    }

    sprintf(text,"... %02x (%2d,%4u,%4u)\r\n",beacon_frame[len2 - 1],
            wifi_status,wifi_interval,ble_interval);
    // // Debug_Serial->print(text);
  }

#endif // DIAG

#endif // FUNC
  
#endif // BEACON

#endif // WIFI

  return 0;
}

/*
 *
 */

int ID_OpenDrone::transmit_ble(uint8_t *odid_msg,int length) {
  
  msecs        = (uint32_t)(esp_timer_get_time() / 1000);
  ble_interval = msecs - last_ble;
  last_ble     = msecs;

#if ID_OD_BT

  int         i, j, k, len, status;
  uint8_t    *a;

  i = j = k = len = 0;
  a = ble_message;

  memset(ble_message,0,sizeof(ble_message));

  //

  ble_message[j++] = 0x1e;
  ble_message[j++] = 0x16;
  ble_message[j++] = 0xfa; // ASTM
  ble_message[j++] = 0xff; //
  ble_message[j++] = 0x0d;

#if 0
  ble_message[j++] = ++counter;
#else
  ble_message[j++] = ++msg_counter[odid_msg[0] >> 4];
#endif

  for (i = 0; (i < length)&&(j < sizeof(ble_message)); ++i, ++j) {

    ble_message[j] = odid_msg[i];
  }

  status = transmit_ble2(ble_message,len = j); 

#if DIAGNOSTICS && 0

  char       text[64], text2[34];
  static int first = 1;

  if (Debug_Serial) {

    if (first) {

      first = 0;

      // // Debug_Serial->print("0000000 00            ");

      for (i = 0; (i < 32); ++i) {

        sprintf(text,"%02d ",i);
        // // Debug_Serial->print(text);
      }

      // // Debug_Serial->print("\r\n");
    }

    sprintf(text,"%7lu %02x (%2d,%2d) .. ",
            (unsigned long)(esp_timer_get_time() / 1000),len - 1,len - 1,length);
    // // Debug_Serial->print(text);

    for (i = 0; (i < len)&&(i < 32); ++i) {

      sprintf(text,"%02x ",a[i]);
      text2[i] = ((a[i] > 31)&&(a[i] < 127)) ? a[i]: '.';
      // // Debug_Serial->print(text);
    }

    text2[i] = 0;

    // // Debug_Serial->print(text2);
    // // Debug_Serial->print("\r\n");
  }

#endif

#endif // BT

  return 0;
}

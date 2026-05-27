#include "spoofer.h"
#include <algorithm>
#include <cmath>

void Spoofer::init() {
  memset(&clock_tm, 0, sizeof(struct tm));
  clock_tm.tm_hour  =  10;
  clock_tm.tm_mday  =  16;
  clock_tm.tm_mon   =  11;
  clock_tm.tm_year  = 122;
  tv.tv_sec =
  time_2 = mktime(&clock_tm);
  settimeofday(&tv, &utc);

  memset(&utm_parameters, 0, sizeof(utm_parameters));
  
  // 生成随机ID
  char id_buffer[24];
  snprintf(id_buffer, sizeof(id_buffer), "ESP32-DEMO-%04X", (unsigned int)(esp_random() & 0xFFFF));
  
  // 设置操作员ID
  strncpy(utm_parameters.UAS_operator, id_buffer, sizeof(utm_parameters.UAS_operator) - 1);
  utm_parameters.UAS_operator[sizeof(utm_parameters.UAS_operator) - 1] = '\0';
  
  // 设置无人机序列号
  strncpy(utm_parameters.UAV_id, id_buffer, sizeof(utm_parameters.UAV_id) - 1);
  utm_parameters.UAV_id[sizeof(utm_parameters.UAV_id) - 1] = '\0';
  
  // 设置 ID 类型为序列号 (ODID_IDTYPE_SERIAL_NUMBER = 1)
  utm_parameters.ID_type = 1;
  
  // 设置无人机类型为多旋翼 (ODID_UATYPE_HELICOPTER_OR_MULTIROTOR = 2)
  utm_parameters.UA_type = 2;
  
  utm_parameters.region      = 1;
  utm_parameters.EU_category = 1;
  utm_parameters.EU_class    = 5;
  
#if ID_CHINA
  strncpy(utm_parameters.caac_registration, "123456789012345", 
          sizeof(utm_parameters.caac_registration) - 1);
  utm_parameters.caac_registration[sizeof(utm_parameters.caac_registration) - 1] = '\0';
  utm_parameters.caac_uom_code = 0x01;
#endif
  
  squitter.init(&utm_parameters);
  
  memset(&utm_data, 0, sizeof(utm_data));
}

void Spoofer::updateLocation(float latitude, float longitude) {
  utm_data.latitude_d =
  utm_data.base_latitude = latitude + (float) ((int)(esp_random() % 10) - 5) / 10000.0;

  utm_data.longitude_d =
  utm_data.base_longitude = longitude + (float) ((int)(esp_random() % 10) - 5) / 10000.0;

  utm_data.base_valid = 1;
  utm_data.base_alt_m = (float) (esp_random() % 1000) / 10.0;

  utm_utils.calc_m_per_deg(utm_data.latitude_d, &m_deg_lat, &m_deg_long);
}

void Spoofer::update() {
  static unsigned long last_update = 0;
  static int update_count = 0;
  unsigned long now = millis();
  
  if (now - last_update < 100) {
    return;
  }
  last_update = now;
  update_count++;
  
  tv.tv_sec = time_2++;
  settimeofday(&tv, &utc);
  
  updateLocation(utm_data.latitude_d, utm_data.longitude_d);
  
  utm_data.alt_msl_m = utm_data.base_alt_m + ((float)(esp_random() % 100) - 50) / 10.0;
  utm_data.alt_agl_m = utm_data.alt_msl_m;
  
  int speed_kn = 10 + (esp_random() % 20);
  utm_data.speed_kn = speed_kn;
  
  utm_data.heading = esp_random() % 360;
  
  float direction_rad = (float)utm_data.heading * M_PI / 180.0;
  float speed_ms = (float)speed_kn * 0.514444;
  
  utm_data.vel_N_cm = (int)(speed_ms * cos(direction_rad) * 100.0);
  utm_data.vel_E_cm = (int)(speed_ms * sin(direction_rad) * 100.0);
  utm_data.vel_D_cm = 0;
  
  squitter.transmit(&utm_data);
  
  if (update_count % 10 == 0) {
    ESP_LOGI("SPOOF", "TX #%d: lat=%.6f lon=%.6f alt=%.1fm speed=%dkn heading=%d",
             update_count, utm_data.latitude_d, utm_data.longitude_d, 
             utm_data.alt_msl_m, utm_data.speed_kn, utm_data.heading);
  }
}
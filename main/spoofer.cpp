#include "spoofer.h"
#include "esp_log.h"
void Spoofer::init() {
  // 时间相关初始化
  memset(&clock_tm, 0, sizeof(struct tm));
  memset(&tv, 0, sizeof(tv));
  memset(&utc, 0, sizeof(utc));
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
  // 仅在首次初始化时设置基准位置
  if (!utm_data.base_valid) {
    utm_data.base_latitude  = latitude;
    utm_data.base_longitude = longitude;
    utm_data.latitude_d     = latitude;
    utm_data.longitude_d    = longitude;
    utm_data.base_valid     = 1;
    utm_data.base_alt_m     = 50.0f;  // 基准高度 50m
    utm_utils.calc_m_per_deg(utm_data.latitude_d, &m_deg_lat, &m_deg_long);
  }
}

void Spoofer::update() {
  static unsigned long last_update = 0;
  static int update_count = 0;
  unsigned long now = (unsigned long)(esp_timer_get_time() / 1000);
  
  if (now - last_update < 100) {
    return;
  }
  
  float dt = (float)(now - last_update) / 1000.0f;  // 秒
  last_update = now;
  update_count++;
  
  tv.tv_sec = time_2++;
  settimeofday(&tv, &utc);
  
  // 首次初始化基准位置
  updateLocation(23.1400f, 113.2580f);  // 广州附近
  
  // --- 平滑的飞行轨迹模拟 ---
  
  // 圆形轨迹：以基准点为中心，半径约 100m 的圆圈
  float total_time_s = (float)(update_count) * 0.1f;  // 每 100ms 一次更新
  float angular_speed = 0.15f;  // 弧度/秒，约 15 秒一圈
  float angle = total_time_s * angular_speed;
  float circle_radius = 100.0f;  // 米
  
  // 计算偏移（经纬度）
  float dlat_m = sin(angle) * circle_radius * 0.5f;  // 北向偏移
  float dlon_m = cos(angle) * circle_radius;           // 东向偏移
  
  utm_data.latitude_d  = utm_data.base_latitude  + dlat_m / (float)m_deg_lat;
  utm_data.longitude_d = utm_data.base_longitude + dlon_m / (float)m_deg_long;
  
  // 高度：50-80m 正弦波动
  float alt = utm_data.base_alt_m + sin(total_time_s * 0.3f) * 15.0f;
  utm_data.alt_msl_m = alt;
  utm_data.alt_agl_m = alt;
  
  // 速度：3-8 节（约 1.5-4 m/s，无人机合理范围）
  float speed_kn_f = 3.0f + sin(total_time_s * 0.5f) * 2.5f;
  utm_data.speed_kn = (int)speed_kn_f;
  
  // 航向：沿切线方向平滑变化
  utm_data.heading = (int)(angle * 180.0f / M_PI + 90.0f) % 360;
  if (utm_data.heading < 0) utm_data.heading += 360;
  
  // 速度分量
  float direction_rad = (float)utm_data.heading * M_PI / 180.0f;
  float speed_ms = speed_kn_f * 0.514444f;
  
  utm_data.vel_N_cm = (int)(speed_ms * cos(direction_rad) * 100.0f);
  utm_data.vel_E_cm = (int)(speed_ms * sin(direction_rad) * 100.0f);
  utm_data.vel_D_cm = (int)(cos(total_time_s * 0.3f) * 15.0f * 0.3f * 100.0f);  // 垂直速度 cm/s
  
  squitter.transmit(&utm_data);
  
  if (update_count % 10 == 0) {
    ESP_LOGI("SPOOF", "TX #%d: lat=%.6f lon=%.6f alt=%.1fm speed=%.1fkn heading=%d",
             update_count, utm_data.latitude_d, utm_data.longitude_d, 
             utm_data.alt_msl_m, speed_kn_f, utm_data.heading);
  }
}
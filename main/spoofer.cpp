#include "spoofer.h"
#include <algorithm>
#include <cmath>

void Spoofer::init() {
  // 时间相关初始化
  memset(&clock_tm, 0, sizeof(struct tm));
  clock_tm.tm_hour  =  10;
  clock_tm.tm_mday  =  16;
  clock_tm.tm_mon   =  11;
  clock_tm.tm_year  = 122;
  tv.tv_sec =
  time_2 = mktime(&clock_tm);
  settimeofday(&tv, &utc);

  // UTM 相关初始化
  memset(&utm_parameters, 0, sizeof(utm_parameters));
  
  // 生成随机ID
  String id = getID();
  strncpy(utm_parameters.UAS_operator, id.c_str(), sizeof(utm_parameters.UAS_operator) - 1);
  utm_parameters.UAS_operator[sizeof(utm_parameters.UAS_operator) - 1] = '\0';
  
  utm_parameters.region      = 1;
  utm_parameters.EU_category = 1;
  utm_parameters.EU_class    = 5;
  
#if ID_CHINA
  // 设置 CAAC 实名登记码示例 (实际使用时应从 NVS 或配置获取)
  // 格式: 15位数字，例如: 123456789012345
  strncpy(utm_parameters.caac_registration, "123456789012345", 
          sizeof(utm_parameters.caac_registration) - 1);
  utm_parameters.caac_registration[sizeof(utm_parameters.caac_registration) - 1] = '\0';
  utm_parameters.caac_uom_code = 0x01;
#endif
  
  squitter.init(&utm_parameters);
  memset(&utm_data, 0, sizeof(utm_data));
}

void Spoofer::updateLocation(float latitude, float longitude) {
  // 定义位置加上一些噪声
  utm_data.latitude_d =
  utm_data.base_latitude = latitude + (float) ((int)(esp_random() % 10) - 5) / 10000.0;

  utm_data.longitude_d =
  utm_data.base_longitude = longitude + (float) ((int)(esp_random() % 10) - 5) / 10000.0;

  utm_data.base_valid = 1;
  utm_data.base_alt_m = (float) (esp_random() % 1000) / 10.0;

  utm_utils.calc_m_per_deg(utm_data.latitude_d, utm_data.longitude_d, &m_deg_lat, &m_deg_long);
}

void Spoofer::update() {
  // FAA 要求最低频率为 1 Hz，这里我们使用 2 Hz
  uint32_t current_time = millis();
  if ((current_time - last_update) < 200) {
    return;
  }

  // 更新时间计算
  double time_elapsed_secs = double(current_time - last_update) / 1000.0;
  last_update = current_time;

  // 随机卫星数量
  utm_data.satellites = esp_random() % 8 + 8;

  // 随机加速度来改变速度
  // 偏向于飞向中心
  speed_m_x += float((int)(esp_random() % (uint32_t)(2 * max_accel)) - max_accel) / 1000.0 - 0.05 * x;
  speed_m_y += float((int)(esp_random() % (uint32_t)(2 * max_accel)) - max_accel) / 1000.0 - 0.05 * y;
  speed_m_x = std::max(-max_speed, std::min(max_speed, static_cast<double>(speed_m_x)));
  speed_m_y = std::max(-max_speed, std::min(max_speed, static_cast<double>(speed_m_y)));

  // 更新实际速度（节）
  double absolute_speed = std::sqrt(static_cast<double>(speed_m_x * speed_m_x + speed_m_y * speed_m_y));
  utm_data.speed_kn = static_cast<int>(speed_ms2kn * absolute_speed);

  // 根据速度计算航向
  double heading_rads = atan2(speed_m_y, speed_m_x);
  int heading_degs = static_cast<int>(heading_rads * angle_rad2deg);
  utm_data.heading = heading_degs % 360;

  // 计算新的 x, y 坐标
  x += speed_m_x * time_elapsed_secs;
  y += speed_m_y * time_elapsed_secs;

  // 计算新的高度
  float climbrate = float((int)(esp_random() % (uint32_t)(2 * max_climbrate)) - max_climbrate) / 1000.0;
  z = std::max(1.0f, std::min(static_cast<float>(max_height), z + climbrate));
  utm_data.alt_msl_m = utm_data.base_alt_m + z;
  utm_data.alt_agl_m = z;

  // 更新经纬度
  utm_data.latitude_d  = utm_data.base_latitude  + (y / m_deg_lat);
  utm_data.longitude_d = utm_data.base_longitude + (x / m_deg_long);

  // 发送数据
  squitter.transmit(&utm_data);
}

String Spoofer::getID() {
  String characters = String("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  String ID = "";
  for (int i = 0; i < 16; i++) {
    ID.concat(characters[(esp_random() % characters.length())]);
  }
  return ID;
}
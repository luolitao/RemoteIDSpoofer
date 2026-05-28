#include "spoofer.h"

void Spoofer::init() {
#if ID_CHINA
  // 默认 CAAC 登记码（无参数版本的回退）
  init("123456789012345");
#else
  _init_common();
  squitter.init(&utm_parameters);
#endif
}

#if ID_CHINA
void Spoofer::init(const char *caac_reg) {
  // 使用传入的 CAAC 登记码
  _init_common();
  strncpy(utm_parameters.caac_registration, caac_reg,
          sizeof(utm_parameters.caac_registration) - 1);
  utm_parameters.caac_registration[sizeof(utm_parameters.caac_registration) - 1] = '\0';
  utm_parameters.caac_uom_code = 0x01;

  // 初始化 squitter（会触发 WiFi 初始化，由 init2 的 static guard 确保只执行一次）
  squitter.init(&utm_parameters);
}
#endif

void Spoofer::_init_common() {
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

  // UTM 相关初始化
  memset(&utm_parameters, 0, sizeof(utm_parameters));
  
  // 生成随机ID
  char id_buf[20];
  getID(id_buf, sizeof(id_buf));
  strncpy(utm_parameters.UAS_operator, id_buf, sizeof(utm_parameters.UAS_operator) - 1);
  utm_parameters.UAS_operator[sizeof(utm_parameters.UAS_operator) - 1] = '\0';
  
  utm_parameters.region      = 1;
  utm_parameters.EU_category = 1;
  utm_parameters.EU_class    = 5;
  
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
  uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
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

void Spoofer::getID(char *buf, size_t buf_size) {
  static const char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const size_t char_len = sizeof(characters) - 1;  // exclude null terminator
  const size_t id_len = 16;
  
  size_t n = (id_len < buf_size - 1) ? id_len : (buf_size - 1);
  for (size_t i = 0; i < n; i++) {
    buf[i] = characters[esp_random() % char_len];
  }
  buf[n] = '\0';
}
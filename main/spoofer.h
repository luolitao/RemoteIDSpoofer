#ifndef SPOOFER_H
#define SPOOFER_H

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include <algorithm>
#include "esp_timer.h"
#include "esp_random.h"

#include "utm.h"
#include "id_open.h"

class Spoofer {
  private:
    ID_OpenDrone squitter;
    UTM_Utilities utm_utils;

    struct UTM_parameters utm_parameters;
    struct UTM_data utm_data;

    // conversion constants and params
    const double angle_rad2deg = 180.0 / M_PI;
    const double speed_ms2kn = 1.9438452;
    const double max_accel = 10 * 1000; // in mms-1
    const double max_speed = 25; // in ms-2
    const double max_climbrate = 10 * 1000; // in mms-1
    const double max_height = 200; // in m over takeoff

    // runtime variables
    float speed_m_x = 0.0, speed_m_y = 0.0;
    double m_deg_lat = 0.0, m_deg_long = 0.0;
    float x = 0.0, y = 0.0, z = 0.0;

    // runtime time variables
    time_t time_2 = 0;
    struct tm clock_tm;
    struct timeval tv;
    struct timezone utc;
    uint32_t last_update = 0;

    // random ID generator (writes to buf, max 20 chars)
    void getID(char *buf, size_t buf_size);
    void _init_common();  // 公共初始化逻辑（不含 CAAC）

  public:
    void init();
#if ID_CHINA
    void init(const char *caac_reg);  // 接收 CAAC 登记码的初始化
#endif
    void updateLocation(float latitude, float longitude);
    void update();
};

#endif
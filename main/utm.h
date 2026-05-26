#ifndef UTM_H
#define UTM_H

#include <stdint.h>
#include "id_open.h"

// Satellite level thresholds
#define SATS_LEVEL_1  4
#define SATS_LEVEL_2  6
#define SATS_LEVEL_3  8

struct UTM_parameters {
  char   UAS_operator[21];
  char   UAV_id[21];
  char   UAV_id2[21];
  char   self_id[24];
  uint8_t UA_type;
  uint8_t ID_type;
  uint8_t ID_type2;
  uint8_t region;
  uint8_t EU_category;
  uint8_t EU_class;
  
#if ID_CHINA
  char   caac_registration[32];  // CAAC real-name registration code (15 digits)
  uint8_t caac_uom_code;         // UOM system code
#endif
};

struct UTM_data {
  int years, months, days, hours, minutes, seconds, csecs;
  double latitude_d, longitude_d;
  float alt_msl_m, alt_agl_m;
  int speed_kn, heading;
  int satellites;
  float base_latitude, base_longitude, base_alt_m;
  int base_valid;
};

class UTM_Utilities {
public:
  UTM_Utilities();
  void calc_m_per_deg(double lat_d, double long_d, double *m_deg_lat, double *m_deg_long);
  int check_EU_op_id(const char *id, const char *secret);
  char luhn36_check(const char *s);
  int luhn36_c2i(char c);
  char luhn36_i2c(int i);
};

#endif
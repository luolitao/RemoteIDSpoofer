/* -*- tab-width: 2; mode: c; -*-
 * 
 * UTM/eID utility functions.
 *
 * Copyright (c) 2020, Steve Jack.
 *
 * Notes
 *
 * 
 */

#pragma GCC diagnostic warning "-Wunused-variable"

#define DIAGNOSTICS  1

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "utm.h"

// WGS84 constants
static const double a = 6378137.0;
static const double e2 = 6.69437999014e-3;

UTM_Utilities::UTM_Utilities() {
    // Default constructor - nothing to initialize
}

void UTM_Utilities::calc_m_per_deg(double lat_d, double long_d, double *m_deg_lat, double *m_deg_long) {
    double lat_r = lat_d * M_PI / 180.0;
    
    // Radius of curvature in the meridian
    double rho = a * (1 - e2) / pow(1 - e2 * sin(lat_r) * sin(lat_r), 1.5);
    
    // Radius of curvature in the prime vertical
    double nu = a / sqrt(1 - e2 * sin(lat_r) * sin(lat_r));
    
    // Meters per degree latitude
    *m_deg_lat = rho * M_PI / 180.0;
    
    // Meters per degree longitude
    *m_deg_long = nu * cos(lat_r) * M_PI / 180.0;
}

int UTM_Utilities::check_EU_op_id(const char *id, const char *secret) {
    int i, j;
    char s[32];
    
    memset(s, 0, sizeof(s));
    
    // Extract operator ID (skip first 3 chars "OP-")
    for (i = 3, j = 0; id[i] && (j < 16); i++) {
        if (id[i] != '-') {
            s[j++] = id[i];
        }
    }
    s[j] = '\0';
    
    // Append secret
    for (i = 0; secret[i] && (j < 31); i++) {
        s[j++] = secret[i];
    }
    s[j] = '\0';
    
    // Calculate and verify checksum
    char check = luhn36_check(s);
    
    return (check == id[strlen(id) - 1]) ? 1 : 0;
}

char UTM_Utilities::luhn36_check(const char *s) {
    int sum = 0;
    int len = strlen(s);
    int factor = 2;
    int i;
    
    for (i = len - 1; i >= 0; i--) {
        int n = luhn36_c2i(s[i]);
        n *= factor;
        n = (n / 36) + (n % 36);
        sum += n;
        factor = (factor == 2) ? 1 : 2;
    }
    
    int remainder = sum % 36;
    int checkdigit = 36 - remainder;
    
    return luhn36_i2c(checkdigit % 36);
}

int UTM_Utilities::luhn36_c2i(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return 0;
}

char UTM_Utilities::luhn36_i2c(int i) {
    if (i >= 0 && i <= 9) {
        return '0' + i;
    } else if (i >= 10 && i <= 35) {
        return 'a' + (i - 10);
    }
    return '0';
}

/*
 *
 */

 
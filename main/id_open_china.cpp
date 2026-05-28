/* -*- tab-width: 2; mode: c; -*-
 * 
 * China CAAC Remote ID implementation
 * Based on CAAC regulations for drone remote identification
 *
 * References:
 * - CAAC Advisory Circular AC-91-FS-2015-31
 * - Interim Rules for Real-name Registration of Civil Unmanned Aircraft
 */

#pragma GCC diagnostic warning "-Wunused-variable"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "id_open.h"

#if ID_CHINA

#include <string.h>
#include <stdio.h>

// CAAC specific constants
#define CAAC_REGISTRATION_LENGTH 15  // 15-digit registration number
#define CAAC_MESSAGE_TYPE        0x0E  // China specific message type

/*
 * Encode CAAC Basic ID message
 * Format: Registration ID (15 digits) + additional info
 */
void ID_OpenDrone::encode_caac_basic_id(const char *registration_id, uint8_t *output) {
    int i;
    
    memset(output, 0, 25);
    
    // Message type indicator for CAAC
    output[0] = CAAC_MESSAGE_TYPE;
    
    // Copy registration ID (15 digits)
    for (i = 0; (i < CAAC_REGISTRATION_LENGTH) && registration_id[i]; i++) {
        output[i + 1] = registration_id[i];
    }
    
    // Pad with zeros if shorter than 15 characters
    for (; i < CAAC_REGISTRATION_LENGTH; i++) {
        output[i + 1] = '0';
    }
    
    // Set UAV type in last byte
    output[16] = UAS_data.BasicID[0].UAType;
}

/*
 * Build complete CAAC message packet
 * Combines Basic ID, Location, and System info in CAAC format
 */
int ID_OpenDrone::build_caac_message(uint8_t *buffer, int max_len) {
    int offset = 0;
    uint8_t temp_buf[32];
    
    if (max_len < 50) {
        return -1;  // Buffer too small
    }
    
    // Header: CAAC OUI + Message Type
    buffer[offset++] = 0xFA;  // ASTM OUI
    buffer[offset++] = 0x0B;
    buffer[offset++] = 0xBC;  // OpenDroneID
    buffer[offset++] = CAAC_MESSAGE_TYPE;
    
    // Basic ID (CAAC registration)
    if (UAS_data.BasicID[0].UASID[0]) {
        encode_caac_basic_id(UAS_data.BasicID[0].UASID, temp_buf);
        memcpy(&buffer[offset], temp_buf, 17);
        offset += 17;
    }
    
    // Location data
    buffer[offset++] = 0x01;  // Location message type
    
    // Status
    buffer[offset++] = (uint8_t)UAS_data.Location.Status;
    
    // Latitude (4 bytes, int32, degrees * 1e7)
    int32_t lat_int = (int32_t)(UAS_data.Location.Latitude * 1e7);
    memcpy(&buffer[offset], &lat_int, 4);
    offset += 4;
    
    // Longitude (4 bytes, int32, degrees * 1e7)
    int32_t lon_int = (int32_t)(UAS_data.Location.Longitude * 1e7);
    memcpy(&buffer[offset], &lon_int, 4);
    offset += 4;
    
    // Altitude (2 bytes, uint16, meters * 10)
    uint16_t alt_int = (uint16_t)(UAS_data.Location.AltitudeGeo * 10);
    memcpy(&buffer[offset], &alt_int, 2);
    offset += 2;
    
    // Speed (2 bytes, uint16, cm/s)
    uint16_t speed_int = (uint16_t)(UAS_data.Location.SpeedHorizontal * 100);
    memcpy(&buffer[offset], &speed_int, 2);
    offset += 2;
    
    // Heading (2 bytes, uint16, degrees * 100)
    uint16_t heading_int = (uint16_t)(UAS_data.Location.Direction * 100);
    memcpy(&buffer[offset], &heading_int, 2);
    offset += 2;
    
    // Timestamp (4 bytes, seconds since midnight UTC)
    uint32_t timestamp = (uint32_t)(UAS_data.Location.TimeStamp);
    memcpy(&buffer[offset], &timestamp, 4);
    offset += 4;
    
    // Simple checksum
    uint8_t checksum = 0;
    for (int i = 3; i < offset; i++) {
        checksum ^= buffer[i];
    }
    buffer[offset++] = checksum;
    
    return offset;
}

/*
 * Pack and optionally encrypt national (CAAC) message
 */
int ID_OpenDrone::pack_encrypt_national(uint8_t *payload) {
    int length = 0;
    
    if (!payload) {
        return 0;
    }
    
    // Build CAAC formatted message
    length = build_caac_message(payload, beacon_max_packed);
    
    if (length <= 0) {
        // Fallback to standard ODID format if CAAC build fails
        length = odid_message_build_pack(&UAS_data, payload, beacon_max_packed);
    }
    
    return length;
}

/*
 * Initialize CAAC-specific parameters
 */
void ID_OpenDrone::init_national(struct UTM_parameters *parameters) {
    // Store CAAC registration ID
    if (parameters->caac_registration[0]) {
        strncpy(UAS_data.BasicID[0].UASID, 
                parameters->caac_registration, 
                ODID_ID_SIZE);
        UAS_data.BasicID[0].UASID[ODID_ID_SIZE - 1] = '\0';
        
        // Also store in operator field
        strncpy(operatorID_data->OperatorId, 
                parameters->caac_registration, 
                ODID_ID_SIZE);
        operatorID_data->OperatorId[ODID_ID_SIZE - 1] = '\0';
    }
    
    // Set Basic ID type to CAA Registration
    UAS_data.BasicID[0].IDType = ODID_IDTYPE_CAA_REGISTRATION_ID;
    
    // Configure system data for China region
    system_data->ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    system_data->OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
}

/*
 * Set authentication key for CAAC (if encryption is required)
 */
void ID_OpenDrone::auth_key_national(uint8_t *key, int key_len, 
                                      uint8_t *iv, int iv_len) {
    auth_key = key;
    key_length = key_len;
    auth_iv = iv;
    iv_length = iv_len;
}

#endif // ID_CHINA
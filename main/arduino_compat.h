#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_timer.h"

// Arduino 类型定义
typedef std::string String;

// Arduino 函数兼容
inline unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000);
}

inline unsigned long micros() {
    return (unsigned long)esp_timer_get_time();
}

inline void delay(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

inline uint32_t random(uint32_t max) {
    return esp_random() % max;
}

inline int32_t random(int32_t min, int32_t max) {
    return min + (esp_random() % (max - min));
}

// String 类简化实现
class ArduinoString {
private:
    char* str;
public:
    ArduinoString() : str(nullptr) {}
    ArduinoString(const char* s) {
        if (s) {
            str = strdup(s);
        } else {
            str = nullptr;
        }
    }
    ~ArduinoString() {
        free(str);
    }
    
    ArduinoString& concat(const char* s) {
        if (s) {
            if (!str) {
                str = strdup(s);
            } else {
                size_t len1 = strlen(str);
                size_t len2 = strlen(s);
                char* new_str = (char*)realloc(str, len1 + len2 + 1);
                if (new_str) {
                    strcat(new_str, s);
                    str = new_str;
                }
            }
        }
        return *this;
    }
    
    ArduinoString& concat(char c) {
        char s[2] = {c, 0};
        return concat(s);
    }
    
    const char* c_str() const { return str ? str : ""; }
    size_t length() const { return str ? strlen(str) : 0; }
    
    operator const char*() const { return c_str(); }
    
    char operator[](int index) const {
        if (str && index >= 0 && index < (int)strlen(str)) {
            return str[index];
        }
        return 0;
    }
};

// 重新定义 String 为 ArduinoString
#define String ArduinoString

// Serial 兼容 - 简化版本，不使用 to_string
class SerialCompat {
public:
    static void begin(unsigned long baud) {
        ESP_LOGI("Serial", "Serial started at %lu baud", baud);
    }
    
    static size_t print(int val) {
        ESP_LOGI("Serial", "%d", val);
        return 0;
    }
    
    static size_t print(long val) {
        ESP_LOGI("Serial", "%ld", val);
        return 0;
    }
    
    static size_t print(unsigned long val) {
        ESP_LOGI("Serial", "%lu", val);
        return 0;
    }
    
    static size_t print(float val) {
        ESP_LOGI("Serial", "%f", val);
        return 0;
    }
    
    static size_t print(double val) {
        ESP_LOGI("Serial", "%f", val);
        return 0;
    }
    
    static size_t print(const char* str) {
        ESP_LOGI("Serial", "%s", str);
        return 0;
    }
    
    static size_t println(int val) {
        ESP_LOGI("Serial", "%d", val);
        return 0;
    }
    
    static size_t println(long val) {
        ESP_LOGI("Serial", "%ld", val);
        return 0;
    }
    
    static size_t println(unsigned long val) {
        ESP_LOGI("Serial", "%lu", val);
        return 0;
    }
    
    static size_t println(float val) {
        ESP_LOGI("Serial", "%f", val);
        return 0;
    }
    
    static size_t println(double val) {
        ESP_LOGI("Serial", "%f", val);
        return 0;
    }
    
    static size_t println(const char* str) {
        ESP_LOGI("Serial", "%s", str);
        return 0;
    }
};

#define Serial SerialCompat

// EEPROM 兼容层（使用 NVS）
#include "nvs.h"
#include "nvs_flash.h"

class EEPROMCompat {
private:
    nvs_handle_t nvs_handle;
    bool initialized;
    uint8_t* buffer;
    size_t buffer_size;
    
public:
    EEPROMCompat() : initialized(false), buffer(nullptr), buffer_size(0) {}
    
    bool begin(size_t size) {
        buffer_size = size;
        buffer = (uint8_t*)malloc(size);
        memset(buffer, 0xFF, size);
        
        esp_err_t err = nvs_open("eeprom_emu", NVS_READWRITE, &nvs_handle);
        if (err == ESP_OK) {
            initialized = true;
            // 从 NVS 加载数据
            size_t required_size = size;
            nvs_get_blob(nvs_handle, "data", buffer, &required_size);
        }
        return initialized;
    }
    
    uint8_t read(int address) {
        if (address >= 0 && address < (int)buffer_size) {
            return buffer[address];
        }
        return 0xFF;
    }
    
    void write(int address, uint8_t value) {
        if (address >= 0 && address < (int)buffer_size) {
            buffer[address] = value;
        }
    }
    
    template<typename T>
    void put(int address, const T& value) {
        if (address + sizeof(T) <= buffer_size) {
            memcpy(&buffer[address], &value, sizeof(T));
        }
    }
    
    template<typename T>
    void get(int address, T& value) {
        if (address + sizeof(T) <= buffer_size) {
            memcpy(&value, &buffer[address], sizeof(T));
        }
    }
    
    bool commit() {
        if (initialized) {
            nvs_set_blob(nvs_handle, "data", buffer, buffer_size);
            return nvs_commit(nvs_handle) == ESP_OK;
        }
        return false;
    }
    
    void end() {
        if (initialized) {
            nvs_close(nvs_handle);
            initialized = false;
        }
        free(buffer);
        buffer = nullptr;
    }
};

static EEPROMCompat EEPROM;

// constrain 宏
#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

#endif // ARDUINO_COMPAT_H
#include "i2c/i2c_drv/i2c_common.h"
#include <sys/time.h>
#include <time.h>

static const char TAG[] = "ds1338";

/**
 * buffer for DS1338
 * position 0 is the address of the register to read or write
 * next 7 positions are the data to read or write
 */
uint8_t dds1338Buf[8]={0};

/**
 * struct for DS1338
 */
i2c_ds1338_config_t ds1338_conf = {
    .conf.scl_speed_hz = I2C_FREQ_HZ,
    .conf.device_address = RTC_ADDR,
    .conf.dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .buff = dds1338Buf,  
    .dl_addr = 1,
    .dl_r = 7,
    .wt_ms = 1,
};

/**
 * @brief Init an ds1338.
 * @param None
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t i2c_ds1338_init(){
    esp_err_t ret;
    ret = i2c_master_bus_add_device(I2C1_bus_handle, &ds1338_conf.conf, &ds1338_conf.handle);
    return ret;
}

// Function to convert an integer to BCD
static inline uint8_t toBCD(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

static inline uint8_t fromBCD(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

/**
 * @brief Read time from DS1338 and update system time.
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t update_system_time_from_ds1338(void){
    // struct tm timeinfo; // Removed unused variable
    esp_err_t ret;
    struct timeval tv = {0};
    struct tm timeinfo;
    time_t time_rtc;
    
    ret = i2c_master_transmit_receive( ds1338_conf.handle, 
                                       ds1338_conf.buff, ds1338_conf.dl_addr,
                                       ds1338_conf.buff+1, ds1338_conf.dl_r, -1);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time from DS1338");
        return ret;
    }

    //tm_sec is 0-59
    timeinfo.tm_sec = fromBCD(dds1338Buf[1]);
    
    //tm_min is 0-59
    timeinfo.tm_min = fromBCD(dds1338Buf[2]);
    
    //tm_hour is 0-23
    dds1338Buf[3] &= 0x3F; // Clear 12/24 bit
    timeinfo.tm_hour = fromBCD(dds1338Buf[3]);

    //tm_mday is 1-31
    timeinfo.tm_mday = fromBCD(dds1338Buf[5]);
    
    //tm_mon is 0-11
    timeinfo.tm_mon = fromBCD(dds1338Buf[6]) - 1;
    // printf("Mes leido: %X\n", dds1338Buf[6]);
       
    //tm_year' is since 1900
    timeinfo.tm_year = fromBCD(dds1338Buf[7]) + 100;
    // printf("Año leido: %X\n", dds1338Buf[7]);
    
    // 'mktime' set automatically daylight saving time
    timeinfo.tm_isdst = 0; 

    time_rtc = mktime(&timeinfo);
    tv.tv_sec = time_rtc;
    tv.tv_usec = 0;

    settimeofday(&tv, NULL);
    ESP_LOGI("DS1338", "DATE: %04d-%02d-%02d %02d:%02d:%02d\n",
        timeinfo.tm_year + 1900, // Año
        timeinfo.tm_mon + 1,     // Mes (0-11, por eso se suma 1)
        timeinfo.tm_mday,        // Día del mes
        timeinfo.tm_hour,        // Hora
        timeinfo.tm_min,         // Minutos
        timeinfo.tm_sec);        // Segundos
    return ret;
}

/**
 * @brief Write the given system time to the DS1338 RTC.
 * @param time_ptr Pointer to the time (time_t) to write.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t ds1338_sync(void)
{
    esp_err_t ret;
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Setting rtc time to %lld", now);

    // Build is DS1338 buffer:
    // Register 0: Seconds (BCD, 0-59)
    dds1338Buf[1] = toBCD(timeinfo.tm_sec);
    
    // Register 1: Minutes
    dds1338Buf[2] = toBCD(timeinfo.tm_min);
        
    // Register 2: Hours (force 24-hour format)
    dds1338Buf[3] = toBCD(timeinfo.tm_hour);
    //force 24-hour format
    dds1338Buf[3] &= 0xBF; 
    
    // Register 3: Day-of-week (tm_wday: 0=Sunday .. 6=Saturday)
    // DS1338 expects 1-7, so convert 0 (Sunday) to 7.
    dds1338Buf[4] = (timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday) & 0x0F;
    
    // Register 4: Date (day of month)
    dds1338Buf[5] = toBCD(timeinfo.tm_mday);
    
    // Register 5: Month (tm_mon is 0-11; add 1)
    dds1338Buf[6] = toBCD(timeinfo.tm_mon + 1);
    
    // Register 6: Year (last two digits; tm_year is years since 1900)
    // For instance, for 2025, tm_year is 125 so value = 25.
    dds1338Buf[7] = toBCD((timeinfo.tm_year + 1900) % 100);

    // Transmit the 7-byte buffer over I2C.
    ds1338_conf.buff[0] = 0;
    ret = i2c_master_transmit(ds1338_conf.handle,
                              ds1338_conf.buff, 8, -1);

    ret = ESP_OK;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "RTC DS1338 updated successfully");
    } else {
        ESP_LOGE(TAG, "Failed to update RTC DS1338");
    }

    return ret;
}

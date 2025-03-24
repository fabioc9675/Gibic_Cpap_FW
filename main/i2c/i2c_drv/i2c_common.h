#ifndef I2C_COMMON_H
#define I2C_COMMON_H

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "sdkconfig.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

/*
 * I2C common functions and definitions
 * GPIO used for I2C1 and I2C2
 * frquency of I2C1 and I2C2
 */
#define SCL1           15    // GPIO number for I2C1 master clock
#define SDA1           16    // GPIO number for I2C1 master data
#define I2C_DEV1       I2C_NUM_0  // I2C port number for master dev
#define SCL2           15    // GPIO number for I2C1 master clock
#define SDA2           16    // GPIO number for I2C1 master data
#define I2C_DEV2       I2C_NUM_1  // I2C port number for master dev
#define I2C_FREQ_HZ    400000     // I2C master clock frequency
#define I2C_TIMEOUT_MS       5

/**
 * Addresses of I2C devices.
 */
#define RTC_ADDR      0x68   // DS1338_ADD RTC 400Khz
#define ADC_ADDR      0x48   // ADC1015_ADD ADC 400Khz
#define SDP810_ADDR   0x25   // SDP810_ADD SDP810 400Khz

/*
 *externals variables for I2C1
 */
extern i2c_master_bus_config_t i2c1_bus_conf;
extern i2c_master_bus_handle_t I2C1_bus_handle;

/** 
 *@brief init I2C1
 *@param None
 *@return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t I2C1_init(void);


/**************************************************************************
 **************************************************************************
 * For rtc DS1338
 */

/**
 * typedef for DS1338 config, handle and buffer
 */
typedef struct {
  i2c_device_config_t conf;         /*!< conf device for eeprom device */
  i2c_master_dev_handle_t handle;   /*!< I2C device handle */
  uint8_t *buff;                    /*!< buffer data exchange */
  uint8_t dl_addr;                  /*!< len add to read */
  uint8_t dl_r;                     /*!< len buf read */
  uint8_t wt_ms;                    /*!< timeout for r/w */
} i2c_ds1338_config_t;
 
/**
 * @brief Init an ds1338.
 * @param None
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t i2c_ds1338_init(void);

/**
 * @brief Read time from DS1338 and update system time.
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t update_system_time_from_ds1338(void);

/**
 * @brief Write the given system time to the DS1338 RTC.
 * @param time_ptr Pointer to the time (time_t) to write.
 * @return ESP_OK: Write success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t ds1338_sync(void);


/**************************************************************************
 **************************************************************************
 * For adc ADS1015
 */

/**
 * typedef for ADS1015 config and handle
 */
typedef struct {
    i2c_device_config_t conf;         /*!< conf device for eeprom device */
    i2c_master_dev_handle_t handle;   /*!< I2C device handle */
    uint8_t p_addr;                   /*!< pointer address */
    uint8_t dl_p_addr;                 /*!< len add to read */
    uint8_t dl_r;                     /*!< len buf read */
    uint8_t *buff;                    /*!< buffer for r/w */
    uint8_t wt_ms;                    /*!< timeout for r/w */
} i2c_adc1015_config_t;

/**
 * @brief Init ads1015.
 * @param None
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t i2c_adc1015_init(void);

/**
 * @brief Get data from ADS1015
 * @param[in] ch Channel of ADS1015
 * @param[out] data Data read from ADS1015 register 1
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t i2c_adc1015_get_ch(uint8_t ch, uint16_t *data);

/**
 * @brief Read data from ADS1015
 * @param[out] data Data read from ADS1015 register 0
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function fail reason.
 * @note This function is used after i2c_adc1015_get_ch
 */
esp_err_t i2c_adc1015_read_ch(uint16_t *data);


/**************************************************************************
 **************************************************************************
 * For SDP810
 */

/**
 * typedef for SDP810 config and handle
 */
typedef struct {
    i2c_device_config_t conf;         /*!< conf device for eeprom device */
    i2c_master_dev_handle_t handle;   /*!< I2C device handle */
    uint8_t *buff;                    /*!< buffer for r/w */
    int8_t wt_ms;                    /*!< timeout for r/w */
} i2c_sdp810_config_t;

/**
 * @brief Enumeration to configure the temperature compensation for
 * measurement.
 */
typedef enum {
  SDP800_TEMPCOMP_MASS_FLOW,
  SDP800_TEMPCOMP_DIFFERNTIAL_PRESSURE
} Sdp800TempComp;

/**
 * @brief Enumeration to configure the averaging for measurement.
 */ 
typedef enum {
  SDP800_AVERAGING_NONE,
  SDP800_AVERAGING_TILL_READ
} Sdp800Averaging;

/**
 * @brief Init Sdp810
 * @param None
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t xSdp810Init(void);

/**
 * @brief Starts the continous mesurement with the specified settings.
 * @param tempComp  Temperature compensation: Mass flow or diff. pressure.
 * @param averaging Averaging: None or average till read.
 * @return ESP_OK: Init success. 
 *         ESP_FAIL: Not success.
 *         ESP_ERR_INVALID_ARG: At least one of the specified parameter
 */
esp_err_t xSdp810_StartContinousMeasurement(Sdp800TempComp  tempComp,
                                            Sdp800Averaging averaging);

/**
 * @brief Reads the measurment result from the continous measurment.
 * @param[out] diffPressure Pointer to return the measured diverential pressur.
 * @param[out] temperature  Pointer to return the measured temperature.
 * @return ESP_OK: Read success.
 *         ESP_FAIL: Not success.
 */
esp_err_t xSdp810_ReadMeasurementResults(float *diffPressure , float *temperature);





#endif // I2C_COMMON_H
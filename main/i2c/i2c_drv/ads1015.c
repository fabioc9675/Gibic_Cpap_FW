#include "i2c/i2c_drv/i2c_common.h"

uint8_t adc1015Buf[2]={0};
static const char TAG[] = "adc1015";

//local functions
esp_err_t read_reg (uint8_t reg);

i2c_adc1015_config_t adc1015_conf = {
    .conf.scl_speed_hz = I2C_FREQ_HZ,
    .conf.device_address = ADC_ADDR,
    .conf.dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .dl_p_addr = 1,
    .dl_r = 2,
    .buff=adc1015Buf,
    .wt_ms = 1,
};

/**
 * @brief Init ADC1015
 * @return esp_err_t ESP_OK if success, otherwise error code
 */
esp_err_t i2c_adc1015_init(){
    esp_err_t ret;
    ret = i2c_master_bus_add_device(I2C1_bus_handle, &adc1015_conf.conf, 
                                    &adc1015_conf.handle);
    if (ret != ESP_OK) return ret;

    // Configurar High Threshold MSB = 1 (Registro 0x03)
    uint8_t hi_thresh[] = {0x03, 0x80, 0x00};
    i2c_master_transmit( adc1015_conf.handle, hi_thresh,3, -1);
    
    // Configurar Low Threshold MSB = 0 (Registro 0x02)
    uint8_t lo_thresh[] = {0x02, 0x00, 0x00};
    i2c_master_transmit( adc1015_conf.handle, lo_thresh,3, -1);

    // Configurar Configuración MSB = 0x03, LSB = 0xE0 (Registro 0x01)
    uint8_t config[] = {0x01, 0x03, 0xE0};
    i2c_master_transmit( adc1015_conf.handle, config,3, -1);
 
    return ret;
}

/**
 * @brief get adc channel value in mode continuous or single shot
 * @param[in] ch channel number 0-3
 * @param[in] mode operation mode: continuous or single shot
 * @return esp_err_t ESP_OK if success, otherwise error code
 * 
 */
esp_err_t i2c_adc1015_get_ch(uint8_t ch, uint8_t mode)
{
    uint8_t adctemp[3];
    esp_err_t ret;

    //set pointer to config register
    adctemp[0] = 0x01;
    // set chanel and rate
    adctemp[1] =(uint8_t)((--ch+4) << 4)|0x03;
    // print esta configuración para debug
    // ESP_LOGI(TAG, "ADC1015 config: %02x,%02X", adctemp[0],adctemp[1]);
    // set sample rate 3300SPS 
    adctemp[2] = 0xE0;

    if (mode == ADS1015_MODE_SINGLE_SHOT) {
        //start single conversion
        adctemp[1] |= 0x80;  
    } else { 
        // continuous mode
        adctemp[1] &= 0xFE; 
        // adctemp[2] |= 0xE0;
    }

    ret = i2c_master_transmit( adc1015_conf.handle, 
                                   adctemp,3, -1);
    return ret;
}

/**
 * @brief read adc channel value. Not check if conversion is ready
 * @param[out] data pointer to store the adc value in mV
 * @return esp_err_t ESP_OK if success, otherwise error code
 */
esp_err_t i2c_adc1015_read_ch(int16_t *data){
    esp_err_t ret;
        adc1015_conf.p_addr = 0x00;
        adc1015Buf[0] = 0;
        adc1015Buf[1] = 0;
        ret = i2c_master_transmit_receive( adc1015_conf.handle, 
                                       &adc1015_conf.p_addr,adc1015_conf.dl_p_addr,
                                       adc1015_conf.buff, adc1015_conf.dl_r, -1);
        *data = ((adc1015Buf[0] << 8) | adc1015Buf[1]) >> 4;
        *data *= 2; // return mv;
    return ret;
}

/**
 * @brief Convert ADC value to pressure in cmH2O
 * @param[in] adc_value ADC value from ADS1015
 * @param[in] offset Offset to be subtracted from the pressure
 * @return float Pressure in cmH2O
 */
float get_pressure(uint16_t adc_value, float offset) {
    float pressure = (((adc_value / 600.0)-1)*10); 
    return pressure - offset;
}

esp_err_t read_reg (uint8_t reg){
    esp_err_t ret;

    //read reg
    adc1015_conf.p_addr = reg;
    adc1015Buf[0] = 0;
    adc1015Buf[1] = 0;
    ret = i2c_master_transmit_receive( adc1015_conf.handle, 
                                       &adc1015_conf.p_addr,adc1015_conf.dl_p_addr,
                                       adc1015_conf.buff, adc1015_conf.dl_r, -1);
    ESP_LOGI(TAG, "ADC1015 read data init: %02x,%02X", adc1015Buf[0],adc1015Buf[1]);
    return ret;
}
#include "i2c/i2c_drv/i2c_common.h"
#include "math.h"

//#define DEBUG

// Sensor Commands
typedef enum{
  /// Undefined dummy command.
  COMMAND_UNDEFINED                       = 0x0000,
  /// Start continous measurement                     \n
  /// Temperature compensation: Mass flow             \n
  /// Averaging: Average till read
  COMMAND_START_MEASURMENT_MF_AVERAGE     = 0x3603,
  /// Start continous measurement                     \n
  /// Temperature compensation: Mass flow             \n
  /// Averaging: None - Update rate 1ms
  COMMAND_START_MEASURMENT_MF_NONE        = 0x3608,
  /// Start continous measurement                     \n
  /// Temperature compensation: Differential pressure \n
  /// Averaging: Average till read
  COMMAND_START_MEASURMENT_DP_AVERAGE     = 0x3615,
  /// Start continous measurement                     \n
  /// Temperature compensation: Differential pressure \n
  /// Averaging: None - Update rate 1ms
  COMMAND_START_MEASURMENT_DP_NONE        = 0x361E,
  // Stop continuous measurement.
  COMMAND_STOP_CONTINOUS_MEASUREMENT      = 0x3FF9
}Command;

static const float scaleFactorTemperature = 200;

uint8_t sdp810Buf[9]={0};
i2c_master_status_t status;

i2c_sdp810_config_t sdp810_conf ={
    .conf.scl_speed_hz = I2C_FREQ_HZ,
    .conf.device_address = SDP810_ADDR,
    .conf.dev_addr_length = I2C_ADDR_BIT_LEN_7,
    //.conf.flags.disable_ack_check = 1,
    .buff=sdp810Buf,
    .wt_ms = -1,
};

/**
 * local function to execute a command
 */
esp_err_t xReadMeasurementRawResults(int16_t *diffPressureTicks,
                                     int16_t *temperatureTicks,
                                     uint16_t *scaleFactor);
esp_err_t xCheckCrc(uint8_t *data, uint8_t size, uint8_t checksum);

/**
 * @brief Init Sdp810
 */
esp_err_t xSdp810Init(void){
    esp_err_t ret = ESP_OK;
    ret = i2c_master_bus_add_device(I2C1_bus_handle, &sdp810_conf.conf, 
                                    &sdp810_conf.handle);

    Command command = COMMAND_STOP_CONTINOUS_MEASUREMENT;

    sdp810Buf[0] = command >> 8;
    sdp810Buf[1] = command & 0xFF;
    ret = i2c_master_transmit(sdp810_conf.handle, 
                              sdp810_conf.buff,2, -1);
  
    return ret;
}

/**
 * @brief Starts the continous mesurement with the specified settings.
 */

 // @todo revisar error en la comunicacion 
esp_err_t xSdp810_StartContinousMeasurement(Sdp800TempComp tempComp,
                                            Sdp800Averaging averaging){
    esp_err_t ret = ESP_FAIL;
    //static uint8_t count = 0;
    Command command = COMMAND_UNDEFINED;

    // determine command code
    switch(tempComp) {
        case SDP800_TEMPCOMP_MASS_FLOW:
            switch(averaging){
                case SDP800_AVERAGING_TILL_READ:
                    command = COMMAND_START_MEASURMENT_MF_AVERAGE;
                    break;  
        
                case SDP800_AVERAGING_NONE:
                    command = COMMAND_START_MEASURMENT_MF_NONE;
                    break;
            }
            break;
    
        case SDP800_TEMPCOMP_DIFFERNTIAL_PRESSURE:
            switch(averaging) {
                case SDP800_AVERAGING_TILL_READ:
                    command = COMMAND_START_MEASURMENT_DP_AVERAGE;
                    break;
        
                case SDP800_AVERAGING_NONE:
                    command = COMMAND_START_MEASURMENT_DP_NONE;
                    break;
            }
            break;
    }
  
    if(COMMAND_UNDEFINED != command) {
        sdp810Buf[0] = command >> 8;
        sdp810Buf[1] = command & 0xFF;
        ret = i2c_master_bus_reset(I2C1_bus_handle);
        ret = i2c_master_transmit(sdp810_conf.handle, 
                              sdp810_conf.buff,2, -1);
        //ret = xExecuteCommand(command);
    } else {
        ret = ESP_ERR_INVALID_ARG;
   }
    //printf("execute command: %04X, count: %d\n",ret, count++);
    return ret;
}

/**
 * @brief Reads the measurment result from the continous measurment.
 */
esp_err_t xSdp810_ReadMeasurementResults(float *diffPressure, float *temperature){

    esp_err_t ret;
    int16_t  diffPressureTicks;
    int16_t  temperatureTicks;
    uint16_t scaleFactorDiffPressure;
    
    ret = xReadMeasurementRawResults(&diffPressureTicks, &temperatureTicks,
                                    &scaleFactorDiffPressure);
    
    #ifdef DEBUG
        printf("error %04x \n", ret);                                        
        printf("diffPressureTicks: %d\n",diffPressureTicks);
        printf("temperatureTicks: %d\n",temperatureTicks);
        printf("scaleFactorDiffPressure: %d\n",scaleFactorDiffPressure);
    #endif
    
    if (ret == ESP_OK){
        *diffPressure = (float)diffPressureTicks / (float)scaleFactorDiffPressure;
        *temperature = (float)temperatureTicks / scaleFactorTemperature;
    }
    return ret;
}

/**
 * @brief Read the raw measurement results from the sensor.
 */
esp_err_t xReadMeasurementRawResults(int16_t *diffPressureTicks,
                                     int16_t *temperatureTicks,
                                     uint16_t *scaleFactor){

    esp_err_t ret;
    ret = i2c_master_receive(sdp810_conf.handle, sdp810_conf.buff, 9, sdp810_conf.wt_ms);
  
    if (ret == ESP_OK){
        //transaction success
        //let check the crc
        for (int i = 0; i < 3; i++){
            if (ret != ESP_OK){
                break;
            }else{
                ret = xCheckCrc(sdp810Buf+(i*3), 2, sdp810Buf[(i*3)+2]);
            } 
        }
        
        if(ret == ESP_OK) {
            *diffPressureTicks = (sdp810Buf[0] << 8) | sdp810Buf[1];
            *temperatureTicks = (sdp810Buf[3] << 8) | sdp810Buf[4];
            *scaleFactor = (sdp810Buf[6] << 8) | sdp810Buf[7];
        }else{
            ret = ESP_FAIL;    
            *diffPressureTicks = 0;
            *temperatureTicks = 0;
            *scaleFactor = 0;
        }
    }
    return ret;
}

/**
 * @brief Read a word from the sensor and check the CRC.
 */
esp_err_t xCheckCrc(uint8_t *data, uint8_t size, uint8_t checksum){
    uint8_t crc = 0xFF;
  
    // calculates 8-Bit checksum with given polynomial 0x31 (x^8 + x^5 + x^4 + 1)
    for(int i = 0; i < size; i++) {
        crc ^= (data[i]);
            for(uint8_t bit = 8; bit > 0; --bit) {
                (crc & 0x80) ? (crc = (crc << 1) ^ 0x31) : (crc = (crc << 1));
            }
    }     
    // verify checksum
  return (crc == checksum) ? ESP_OK : ESP_FAIL;
}


/**
 * @brief get flow measurement from SDP810
 * @param fraw Raw flow value from SDP810
 * @param offset Offset to be applied to flow calculation
 * @return Flow in float format
 * @note This function applies a specific calibration formula based on the raw flow value.
 */
float get_flow(float fraw, float offset) {
    float flow = 0; 
    if(fraw < 0.25) {
        // y = -1 * (4.4427 * sqrt(|x|) + -0.5876) + 0.5388*x + 0.0028*x^2
        float abs_fraw = fabsf(fraw);
        flow = -1 * ((4.4427 * sqrtf(abs_fraw)) - 0.5876) +
                        (0.5388 * fraw) +
                        (0.0028 * fraw * fraw); 
    }else if(fraw > 0.25) {
        // y = (11.0380 * sqrt(|x|) + -10.6718) + -0.4424*x + 0.0037*x^2
        flow = (11.0380 * sqrtf(fraw) + -10.6718) +
                        (-0.4424 * fraw) +
                        (0.0037 * fraw * fraw);  
    }else {
        flow = 0.0;
    }

    return flow - offset;
}
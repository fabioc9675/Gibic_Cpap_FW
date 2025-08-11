#include "i2c/i2c_app.h"

/**
 * @todo:
 * - Implement a function to initialize the I2C bus and devices.
 * - Implement a function to read offsets.
 */


/*
    intercalar lectura del adc con la de los otros dispositivos 
    presentes, teneiendo en cuanta que el adc tiene un pin de 
    interrupcion que se activa cuando cuando la conversion esta lista
*/

i2c_master_bus_handle_t I2C1_bus_handle;
QueueHandle_t i2c_App_queue = NULL;

// for i2c1
i2c_master_bus_config_t i2c1_bus_conf = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_DEV1,
    .scl_io_num = SCL1,
    .sda_io_num = SDA1,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = 1,
};

i2c_master_dev_handle_t ds_handle;

// var maquina estados
i2c_stetes_t i2c_state = st_init;

int16_t adc=0;
float offsetPresion = 0;
float offsetFlujo = 0;

TickType_t xLastWakeTime;

/*
 *@brief I2C1 master initialization
 *       also init rtc ds1338 and system time
 */
esp_err_t I2C1_init(void)
{
    esp_err_t ret;
    ret = i2c_new_master_bus(&i2c1_bus_conf, &I2C1_bus_handle);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = i2c_ds1338_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = update_system_time_from_ds1338();
    if (ret != ESP_OK) {
        return ret;
    }   

    return ret;
}

/**
 * task for sample i2c devices
 */
void i2c_app(void *pvParameters)
{
    esp_err_t ret;
    struct Datos_I2c datos;
    float sdppresiondiff = 0;
    float sdptemperatura = 0;

    //init sdp810
    if(ESP_OK !=xSdp810Init())
    {
        #ifdef DEBUG
            printf("error en sdp810\n");
        #endif
    };
    
    //init adc
    (void)i2c_adc1015_init();

    //leemos offset presion
    (void)i2c_adc1015_get_ch(3, &adc);
    ret = i2c_adc1015_read_ch(&adc);
    while (ret != ESP_OK) {
       ret = i2c_adc1015_read_ch(&adc);    
    }
    offsetPresion = get_pressure(adc, 0); 
    ESP_LOGI("I2C_APP", "offset presion raw: %d", adc);
    ESP_LOGI("I2C_APP", "offset presion: %f", offsetPresion);
    //offsetPresion = (float)(((adc)/(0.2*3000))-1);

    if(ESP_OK !=xSdp810_StartContinousMeasurement(SDP800_TEMPCOMP_MASS_FLOW, SDP800_AVERAGING_TILL_READ)){
        #ifdef DEBUG
        printf("error start continuos sdp810\n");
        #endif
    };
    
    for(;;) 
    {
        switch (i2c_state)
        {
            case st_init:
                i2c_state = st_reqAdc0;
                break;

            case st_reqAdc0: //request presion
                //ESP_LOGI("I2C_APP", "init adc");
                // Initialize the xLastWakeTime variable with the current time
                xLastWakeTime = xTaskGetTickCount();
                (void)i2c_adc1015_get_ch(3, &adc);
                i2c_state = st_rsdp810;
                break;

            case st_rsdp810: //read sdp810
                (void)xSdp810_ReadMeasurementResults(&sdppresiondiff, &sdptemperatura);
                sdppresiondiff -= offsetFlujo;
                datos.fraw = sdppresiondiff;
                datos.tempFlujo = sdptemperatura;
                // de acuerdo a la caracterizacion de la sdp810
                datos.flujo = get_flow (sdppresiondiff, offsetFlujo);
            
                i2c_state = st_rAdc0;
                break;

            case st_rAdc0: //read presion
                ret = i2c_adc1015_read_ch(&adc);
                while (ret != ESP_OK) {
                    ret = i2c_adc1015_read_ch(&adc); 
                    #ifdef DEBUG
                        printf("esperando conversion adc\n");   
                    #endif
                }
                datos.praw = adc;
                datos.presion = get_pressure(adc, offsetPresion);

                i2c_state = st_iddle;
                break;   

            case st_iddle:
                
                xQueueSend(i2c_App_queue, &datos, 0);
                i2c_state = st_reqAdc0;
                //ESP_LOGI("I2C_APP", "fin adc");                
                vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
                break;

            default:
                break;
        }

    }
}

#include "i2c/i2c_app.h"

//  Pin connected to ADS1015 ALERT/RDY
#define ADC_ALERT_PIN GPIO_NUM_7 

/****** vars and handles ******/
i2c_master_bus_handle_t I2C1_bus_handle;
QueueHandle_t i2c_App_queue = NULL;
SemaphoreHandle_t adc_ready_sem = NULL;
i2c_master_dev_handle_t ds_handle;
i2c_stetes_t i2c_state = st_init;
int16_t adc=0; //temp var for adc reading
float offsetPresion = 0;
float offsetFlujo = 0;
TickType_t xLastWakeTime;

// for i2c1
i2c_master_bus_config_t i2c1_bus_conf = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_DEV1,
    .scl_io_num = SCL1,
    .sda_io_num = SDA1,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = 1,
};


// temp
int64_t t_inicio, t_fin;
/**
 * @brief interrup setup for adc alert pin
 */
static void IRAM_ATTR adc_gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(adc_ready_sem, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief I2C1 master initialization
 *       also init rtc ds1338 and system time
 */
esp_err_t I2C1_init(void)
{
    esp_err_t ret;

    // Create semaphore for ADC ready signal
    adc_ready_sem = xSemaphoreCreateBinary();
    if (adc_ready_sem == NULL) {
        ESP_LOGE("Semaphore", "Failed to create semaphore - Insufficient memory");
    }

    // Setup GPIO for ADC ALERT/RDY pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // El ADS1015 baja el pin a 0 cuando está listo
        .pin_bit_mask = (1ULL << ADC_ALERT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1
    };
    gpio_config(&io_conf);
    // Iinstall GPIO ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ADC_ALERT_PIN, adc_gpio_isr_handler, NULL);
    //gpio_dump_io_configuration(stdout, (1ULL << 7)); 

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

    //init i2c devices
    (void)xSdp810Init();
    (void)i2c_adc1015_init();
    
    //start sdp810 continous measurement
    ret = ESP_FAIL;
    while(ESP_OK != ret){
        ret = xSdp810_StartContinousMeasurement(SDP800_TEMPCOMP_MASS_FLOW, SDP800_AVERAGING_TILL_READ);
    }

    // get offset presion
    // ESP_LOGI("ADC", "request offset presion adc:");
    (void)i2c_adc1015_get_ch(3, &adc);
    if (xSemaphoreTake(adc_ready_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
       ret = i2c_adc1015_read_ch(&adc);
    //    ESP_LOGI("ADC", "offset presion adc: %d", adc);
    } else {
        ESP_LOGE("ADC", "Error: No se recibió interrupción");
    }
    offsetPresion = get_pressure(adc, 0); 
    ESP_LOGI("I2C_APP", "offset presion: %0.2f", offsetPresion);

    uint64_t timenow = esp_timer_get_time();
    while (timenow + 3000 > esp_timer_get_time());

    //leemos offset flujo
    ret = ESP_FAIL;
    while(ESP_OK != ret){
        ret = xSdp810_ReadMeasurementResults(&sdppresiondiff, &sdptemperatura);
        printf("error read sdp810 %d \n", ret);
    }
    offsetFlujo = get_flow (sdppresiondiff, 0);
    ESP_LOGI("I2C_APP", "offset flujo: %0.2f", offsetFlujo);
    
    for(;;) 
    {
        switch (i2c_state)
        {
            case st_init:
            // ESP_LOGI("I2C_APP", "I2C cycle start");
            //     t_inicio = esp_timer_get_time();
                xLastWakeTime = xTaskGetTickCount();
                i2c_state = st_reqAdc2;
                break;

            case st_reqAdc2: //request presion
                (void)i2c_adc1015_get_ch(3, &adc); 
                if (xSemaphoreTake(adc_ready_sem, pdMS_TO_TICKS(20)) == pdTRUE) {
                    i2c_state = st_rAdc2; 
                } else {
                    ESP_LOGE("ADC", "Error: Not received interrupt");
                    i2c_state = st_init; 
                }
                break;

            case st_rAdc2: //read presion
                ret = i2c_adc1015_read_ch(&adc); 
                datos.praw = adc;
                datos.presion = get_pressure(adc, offsetPresion);
                i2c_state = st_reqAdc0; 
                break;
                
            case st_reqAdc0: //request temp
                (void)i2c_adc1015_get_ch(1, &adc);
                i2c_state = st_rsdp810; 
                break;
                
            case st_rsdp810: //read sdp810
                (void)xSdp810_ReadMeasurementResults(&sdppresiondiff, &sdptemperatura);
                //sdppresiondiff -= offsetFlujo;
                datos.fraw = sdppresiondiff;
                datos.tempFlujo = sdptemperatura;
                datos.flujo = get_flow (sdppresiondiff, offsetFlujo);
                i2c_state = st_rAdc0;
                break;

            case st_rAdc0: //read temp
                if (xSemaphoreTake(adc_ready_sem, pdMS_TO_TICKS(20)) == pdTRUE) {
                    ret = i2c_adc1015_read_ch(&adc); 
                    datos.temphumV = adc / 1000.0f; //convert to V
                    i2c_state = st_iddle;
                } else {
                    ESP_LOGE("ADC", "Error: Not received interrupt");
                    i2c_state = st_init; 
                }
                break;
                 
            case st_iddle:
                xQueueSend(i2c_App_queue, &datos, pdMS_TO_TICKS(5));
                i2c_state = st_init;
                // t_fin = esp_timer_get_time();
                // ESP_LOGI("I2C_APP", "I2C cycle time: %lld us", (t_fin - t_inicio));
                xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
                break;

            default:
                break;
        }

    }
}

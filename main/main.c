#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt/mqtt.h"

#include "uSD/usdApp.h"
#include "i2c/i2c_app.h"
#include "bldc/bldc_servo.h"
#include "uart/uartapp.h"
#include "control/control.h"

#include "wifi/wifiserver.h"

//#define CARACTERIZACION

/**
 * task handlers
 */
TaskHandle_t thUartApp = NULL;
TaskHandle_t thSdApp = NULL;
TaskHandle_t thI2CApp = NULL;
TaskHandle_t thBldcApp = NULL;

/**
 * flags and variables for the system
 */
uint8_t flProcSen = 0; //flag para procesar sensores se stea en 1 cuando hay datos nuevos
uint8_t setPointPresion = 0;

//todo: verificar si el timestamp esta en segundos o milisegundos.

uint16_t bldc_sp = 100;
uint8_t flag_bldc = 0;
uint8_t spbldctemp = 0;

/**
 * temporal para enviar a la pantalla
 */
uint8_t cnt=10;

/*
 *Variable para manejar los estados del sistema
 */
typedef enum
{
    idle=0,
    initfilesd,
    initSensors,
    initbldc,
    initCpap,
    bldcoff,
    endCpap

} MS_STATES;

MS_STATES msEstados = idle;

void killTask(TaskHandle_t *pxTaskHandle)
{
    if(pxTaskHandle != NULL && *pxTaskHandle != NULL)
    {
        // Verifica el estado de la tarea
        if(eTaskGetState(*pxTaskHandle) != eDeleted)
        {
            vTaskDelete(*pxTaskHandle);
        }
        *pxTaskHandle = NULL;
    }
}

//TaskHandle_t bldcTaskHandle = NULL; // Identificador para la tarea i2c_app
void app_main(void)
{
    /**
     * essential for the system
     */
    nvs_flash_init();
    esp_event_loop_create_default();
    init_sdmmc();
    I2C1_init();
    
    /**
     * LCD comunicatins queeue and task
     */
    uart_app_queue = xQueueCreate(10, sizeof(struct uartDataIn));
    uart_app_queue_rx = xQueueCreate(10, sizeof(struct ToUartData ));
    xTaskCreate(uart_app, "uart_app", 4096, NULL, 10, &thUartApp);
 
    for(;;)
    {   
        // Process LCD data
        while(uxQueueMessagesWaiting(uart_app_queue) > 0){
            struct uartDataIn datos;
            
            xQueueReceive(uart_app_queue, &datos, 0);
 
            switch (datos.command)
            {
            case 'P': //presion objetivo
                setPointPresion = datos.value;
                break;

            case 'S':
                if(datos.value==1){ // inicia tratamiento 

                    if (setPointPresion &&  msEstados == idle){
                        msEstados = initfilesd;
                        spbldctemp = setPointPresion;
                    }
                    
                }else if(datos.value==0){
                    setPointPresion = 0;
                    msEstados = bldcoff;
                }   
                break;
                    
                case 0:
                    vTaskDelete(thUartApp);
                    wifi_init_ap();
                    start_file_server("/sd");//printf("en case 0\n");
                break;

            default:
                break;
            }
            
        }

        switch (msEstados){
            case idle:
                break; //wait for command
            
            case initfilesd:
                //init queue and task sdcard
                sd_App_queue = xQueueCreate(10, sizeof(struct Datos_usd));
                xTaskCreate(sd_App, "sd_App", 4096, NULL, 10, &thSdApp);
                msEstados = initSensors;//iniciar proceso
                break;
            
            case initSensors:
                //init queue and task i2c
                i2c_App_queue = xQueueCreate(10, sizeof(struct Datos_I2c));
                xTaskCreate(i2c_app, "i2c_app", 4096, NULL, 10, &thI2CApp);
                msEstados = initbldc;
                break; 
                
            case initbldc:
                //init queue and task bldc
                bldc_App_queue = xQueueCreate(10, sizeof(uint16_t));
                xTaskCreate(bldc_servo_app, "bldc_servo_app", 4096, NULL, 10, &thBldcApp);
                msEstados = initCpap;
                break;

            case initCpap:
                struct Datos_usd datos_usd;
                while(uxQueueMessagesWaiting(i2c_App_queue) > 0){ //cola con datos de i2c

                /**
                 * todo: la cola de la sd debe complementarse con datos de los 
                 * algoritmos de deteccion de apnea. esta parte es temporal
                 * aunque podria reutilizarse solo para encapsular los datos
                 * para el algoritmo de control
                 */
                
                    struct Datos_I2c datos_i2c;
                    xQueueReceive(i2c_App_queue, &datos_i2c, 0);
                    //datos_usd.timestamp = esp_log_timestamp() + init_time;
                    datos_usd.timestamp = esp_log_timestamp();
                    
                    datos_usd.presion = datos_i2c.presion;
                    datos_usd.flujo = datos_i2c.flujo;
                    //printf("presion %0.2f\n",datos_i2c.presion);
    
                    /**
                     *esto es temporal   
                     */
                //     if(!cnt--){
                //         struct ToUartData touartdata;
                //         cnt=10;
                //         touartdata.command = UPresion;
                //         touartdata.value = (int8_t)datos_i2c.presion;
                //         xQueueSend(uart_app_queue_rx, &touartdata,0);
                //     }
                    datos_usd.tempflujo = datos_i2c.tempFlujo;
                    xQueueSend(sd_App_queue, &datos_usd, 0);
                // //ESP_LOGI("MAIN", "presion: %0.2f, flujo: %0.2f", datos_usd.presion, datos_usd.flujo);
                // flProcSen = 1;
                    bldc_sp = controller(setPointPresion, datos_usd.presion);
        //             //ESP_LOGI("MAIN", "bldc_sp: %d", bldc_sp);
                    xQueueSend(bldc_App_queue, &bldc_sp, 0);
        //             //procesar datos
        //             //enviar a la pantalla
        //             //enviar a la sd
                }
        //     //proceso en ejecucion
                break;
            case bldcoff:
                //turn off bldc
                bldc_sp = 0;
                xQueueSend(bldc_App_queue, &bldc_sp, 0);
                msEstados = endCpap;
                break;

            case endCpap:
                //terminar procesos
               
                killTask(&thSdApp);
                killTask(&thI2CApp);
                killTask(&thBldcApp);
                msEstados = idle;
                break;
     
        //     case wifiserver:
        //         //creo tarea para el servidor wifi
                
        //         break;
            default:
                break;
        }

         vTaskDelay(1);
    }
}
/*
Para pruebas de mqtt 
printf("Run!\n");
mqtt_initVars("NATALIA", "1128418683", "mqtt://3.90.24.183", 8924);
for(;;)
    {   
     MSmqtt();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
*/
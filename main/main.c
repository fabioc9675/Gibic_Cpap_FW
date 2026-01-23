#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt/mqtt.h"

#include "uSD/usdApp.h"
#include "i2c/i2c_app.h"
#include "bldc/bldc_servo.h"
#include "uart/uartapp.h"
#include "control/control.h"
#include "control/filter.h"
#include "control/lut.h"
#include "control/humidificador.h"
#include "proc/fResp.h"

#include "wifi/wifiserver.h"

/**
 * Task Stats:  
 * Task        TiempoEjecución  Porcentaje  
 * 
 * Task	Nombre de la tarea.
 * TiempoEjecución	Tiempo total de ejecución en ticks (unidades dependen de portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()).
 * Porcentaje	Porcentaje del tiempo total de CPU consumido por la tarea.
 */
void print_task_stats(void) {
    char buffer[1024];
    vTaskGetRunTimeStats(buffer);

    printf("Task Stats:\nTask\t\tTicksE\t\t%%\n%s\n", buffer);
}

//temp
uint64_t t_start, t_end;

/**
 * Task List:  
 * Nombre      Estado   Prioridad  StackLibre  NúmTask  
 * 
 * Nombre	Nombre de la tarea (configurado en xTaskCreate()).
 * Estado	R (Ready), B (Blocked), S (Suspended), D (Deleted).
 * Prioridad	Prioridad de la tarea (0 = más baja).
 * StackLibre	Bytes libres en el stack de la tarea (útil para detectar overflow).
 * NúmTask	Identificador numérico de la tarea.
 */
void print_task_list(void) {
    char buffer[1024];
    vTaskList(buffer);
    printf("Task List:\n%s\n", buffer);
}

void task_monitor(void *pvParameters) {
    while(1) {
        print_task_stats();
        print_task_list();
        vTaskDelay(pdMS_TO_TICKS(5000)); // Cada 5 segundos
    }
}

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
uint8_t setPointPresion = 4;
int16_t bldc_sp = 1;
uint8_t spbldctemp = 0;

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
    endCpap,
    wifiserver,
    wifiend
} MS_STATES;
MS_STATES msEstados = idle;

portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
// function to kill a task
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
    // xTaskCreate(task_monitor, "TaskMonitor", 4096, NULL, 5, NULL);
    /**
     * essential for the system
     */
    nvs_flash_init();
    esp_event_loop_create_default();
    init_sdmmc();
    I2C1_init();
    init_lut_p();
    
    /**
     * LCD comunicatins queeue and task
     */
    uart_app_queue = xQueueCreate(10, sizeof(struct uartDataIn));
    uart_app_queue_rx = xQueueCreate(10, sizeof(struct ToUartData ));
    //          xTaskCreate(uart_app, "uart_app", 4096, NULL, 10, &thUartApp);
    xTaskCreatePinnedToCore(uart_app, "uart_app", 4096, NULL, 1, &thUartApp, APP_CPU_NUM);
    filter_t *fl_press = filter_create(3);
    filter_t *fl_flow = filter_create(3);
    fResp_init();
    //zeros_crossings();

    
    for(;;)
    {   
        // Process LCD data
        while(uxQueueMessagesWaiting(uart_app_queue) > 0){
            struct uartDataIn datos;
            
            xQueueReceive(uart_app_queue, &datos, 0);
            ESP_LOGI("LCD", "RAW, c: %d, v: %d", datos.command, datos.value);
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
                    //setPointPresion = 0;
                    msEstados = bldcoff;
                }   
                break;
                    
            case 'F':
                if (datos.value == 1){
                    msEstados = wifiserver;
                }else{
                    esp_restart();
                }   
                    
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
                // xTaskCreate(sd_App, "sd_App", 4096, NULL, 10, &thSdApp);
                xTaskCreatePinnedToCore(sd_App, "sd_App", 4096, NULL, 1, &thSdApp, APP_CPU_NUM);
                filter_init(fl_press, butn3, butd3);
                filter_init(fl_flow, butn3, butd3);
                msEstados = initSensors;//iniciar proceso
                break;
            
            case initSensors:
                //init queue and task i2c
                i2c_App_queue = xQueueCreate(10, sizeof(struct Datos_I2c));
                // xTaskCreate(i2c_app, "i2c_app", 4096, NULL, 10, &thI2CApp);
                xTaskCreatePinnedToCore(i2c_app, "i2c_app", 4096, NULL, 2, &thI2CApp, PRO_CPU_NUM);
                msEstados = initbldc;
                // init humidificador
                inicializarHumidificador();

                break; 
                
            case initbldc:
                //init queue and task bldc
                bldc_App_queue = xQueueCreate(10, sizeof(int16_t));
                // xTaskCreate(bldc_servo_app, "bldc_servo_app", 4096, NULL, 10, &thBldcApp);
                xTaskCreatePinnedToCore(bldc_servo_app, "bldc_servo_app", 4096, NULL, 1, &thBldcApp, PRO_CPU_NUM);
                msEstados = initCpap;

                /**
                 * temporarily start the bldc
                 */
                struct ToUartData touartdata;
                touartdata.command = UPresion;
                touartdata.value = (int8_t)setPointPresion;
                xQueueSend(uart_app_queue_rx, &touartdata,0);
                inicializarHumidificador();

                break;

            case initCpap:
                
                struct Datos_usd datos_usd;
                struct Datos_I2c datos_i2c;
                
                if (xQueueReceive(i2c_App_queue, &datos_i2c, portMAX_DELAY) == pdPASS)
                { 
                    // t_start = esp_timer_get_time();
                    /**
                     * @todo: la cola de la sd debe complementarse con datos de los 
                     * algoritmos de deteccion de apnea. esta parte es temporal
                     * aunque podria reutilizarse solo para encapsular los datos
                     * para el algoritmo de control
                     */
                    // process low pass filter
                    lp_filter(fl_press, datos_i2c.presion, &datos_usd.presionfl);
                    lp_filter(fl_flow, datos_i2c.flujo, &datos_usd.flujofl);
                    
                    // process pid control
                    //bldc_sp = controller(setPointPresion, datos_i2c.presion, datos_i2c.flujo);
                    bldc_sp = controller(setPointPresion, datos_usd.presionfl, datos_usd.flujofl);
                    
                    //xQueueSend(bldc_App_queue, &bldc_sp, 5 / portTICK_PERIOD_MS);
                    if (xQueueSend(bldc_App_queue, &bldc_sp, pdMS_TO_TICKS(20)) != pdPASS) {
                        // Error: La tarea del motor está saturada
                        ESP_LOGE("BLDC", "Error: BLDc queue full");
                    }
                    
                    // data to sd
                    datos_usd.bldc = bldc_sp;
                    datos_usd.presionfl -= lookup_table_get(&lut_p,setPointPresion);

                    // printf(">BLDC:%0.2f,Flujofl:%.4f,Presfl:%.4f\r\n", 
                    //        (datos_usd.bldc/100.0f),
                    //        //setPointPresion, 
                    //        datos_usd.flujofl, 
                    //        datos_usd.presionfl);   
                    // ESP_LOGI("DATA CPAP","BLDC:%0.2f,Flujofl:%.4f,Presfl:%.4f", 
                    //     (datos_usd.bldc/100.0f),
                    //     datos_usd.flujofl,
                    //     datos_usd.presionfl);

                    datos_usd.presionfl -= lookup_table_get(&lut_p,setPointPresion);
                    processed_signal(datos_usd.flujofl, &datos_usd.t_smp, &datos_usd.t_cp);
                    controlarHumidificador(55.0, datos_i2c.temphumV);
                    xQueueSend(sd_App_queue, &datos_usd, pdMS_TO_TICKS(20));
                
                }
                break;
            
            case bldcoff:
                //turn off bldc
                bldc_sp = -1; //inidcamos apagar
                xQueueSend(bldc_App_queue, &bldc_sp, 0);
                desactivarHumidificadorControl();
                vTaskDelay(15);
                // wait until queue empty
                while(uxQueueMessagesWaiting(sd_App_queue)>0); // Corrected the comment typo
                msEstados = endCpap;
                break;

            case endCpap:
                //terminar procesos
                killTask(&thSdApp);
                killTask(&thI2CApp);
                killTask(&thBldcApp);
                vQueueDelete(i2c_App_queue);
                vQueueDelete(bldc_App_queue);
                vQueueDelete(sd_App_queue);
                msEstados = idle;
                break;
     
            case wifiserver:
                wifi_init_ap();
                start_file_server("/sd");
                msEstados = wifiend;
                break;

            case wifiend:
            default:
                break;
        }
         
        // t_end = esp_timer_get_time();
        // printf("Tiempo de procesamiento: %llu us\n", (t_end - t_start));
        vTaskDelay(3);
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

/**
 *     uint16_t bldc;
    int16_t praw;  // sensor raw presion
    float presion; // sensor presion
    float presionfl; // sensor filtrada presion
    float fraw;    // sensor raw flujo
    float flujo;   // sensor flujo
    float flujofl; // sensor filtrada flujo
    float pdata;
    uint8_t fl1;
    uint8_t fl2;
 */


/** 
 * @todo
 * 
 * - algoritmo de nataly
 * - boton de volver al directori anterior. en web
 * - borrado de archivos masivos
 * 
 */


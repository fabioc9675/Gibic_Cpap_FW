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
        //print_task_list();
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
uint8_t flProcSen = 0; //flag para procesar sensores se stea en 1 cuando hay datos nuevos
uint8_t setPointPresion = 4;

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
    //xTaskCreate(task_monitor, "TaskMonitor", 4096, NULL, 5, NULL);
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
    filter_t *fl_press = filter_create(3);
    filter_t *fl_flow = filter_create(3);
    fResp_init();
    
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
                xTaskCreate(sd_App, "sd_App", 4096, NULL, 10, &thSdApp);
                filter_init(fl_press, butn3, butd3);
                filter_init(fl_flow, butn3, butd3);
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
                while(uxQueueMessagesWaiting(i2c_App_queue) > 0)
                { 
                    /**
                     * @todo: la cola de la sd debe complementarse con datos de los 
                     * algoritmos de deteccion de apnea. esta parte es temporal
                     * aunque podria reutilizarse solo para encapsular los datos
                     * para el algoritmo de control
                     */
                    struct Datos_I2c datos_i2c;
                    // get data from i2c
                    xQueueReceive(i2c_App_queue, &datos_i2c, 0);

                    // process low pass filter
                    lp_filter(fl_press, datos_i2c.presion, &datos_usd.presionfl);
                    lp_filter(fl_flow, datos_i2c.flujo, &datos_usd.flujofl);
                    
                    // process pid control
                    //bldc_sp = controller(setPointPresion, datos_i2c.presion, datos_i2c.flujo);
                    bldc_sp = controller(setPointPresion, datos_usd.presionfl, datos_usd.flujofl);
                    
                    xQueueSend(bldc_App_queue, &bldc_sp, 0);
                    
                    // data to sd
                    datos_usd.bldc = bldc_sp;
                    //datos_usd.praw = datos_i2c.praw;
                    //datos_usd.presion = datos_i2c.presion;
                    //datos_usd.fraw = datos_i2c.fraw;
                    //datos_usd.flujo = datos_i2c.flujo;
                    //datos_usd.pdata = 0;
                    //datos_usd.fl1 = 0;
                    //datos_usd.fl2 = 0;  
                    // printf(">BLDC:%0.2f,Flujofl:%.4f,Presfl:%.4f\r\n", 
                    //        (datos_usd.bldc/100.0f),
                    //        //setPointPresion, 
                    //        datos_usd.flujofl, 
                    //        datos_usd.presionfl);   

                    processed_signal(datos_usd.flujofl, &datos_usd.t_smp, &datos_usd.t_cp);

                    xQueueSend(sd_App_queue, &datos_usd, 0);
                    //ESP_LOGI("SETPOINT","SETPOINT: %d", setPointPresion);
                
                }
                break;
            
            case bldcoff:
                //turn off bldc
                bldc_sp = 0;
                xQueueSend(bldc_App_queue, &bldc_sp, 0);
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

         vTaskDelay(1);
         // vTaskDelay(1000 / portTICK_PERIOD_MS);
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
 * - ok. bug de inicio. la pantalla no responde
 * - ok. carpeta por dia
 * - ok. ajustar estructura queue de datos sd
 * - ok. sacar flujo raw a la sd
 * - caracterizacion de flujo
 * - nuevo modelo del sistema y controlador
 * - anexar set pont de presion
 * - algoritmo de nataly
 * - boton de volver al directori anterior. en web
 * - borrado de archivos masivos
 * 
 */


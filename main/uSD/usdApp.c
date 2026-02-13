#include "uSD/usdApp.h"

/**
 * Each file record will contain 20 miniutes of data
 */
#define RECORD_SIZE (FS_LOG * 60 * 60) // 60 minutes of data

//handler de la cola de envio a la sd
QueueHandle_t sd_App_queue = NULL;

//local variables
//char dir_year[10];
//char dir_month[14];
char file_log[30];
char bufferSd[128];
FILE *f_log = NULL;

// local functions
void initfile(void);

void sd_App(void *pvParameters){
    static uint32_t cnt = 0;
    // temp
    int64_t t_inicio, t_fin;
    //esp_timer_get_time();
    initfile();
    for(;;)
    {
        struct Datos_usd datos;
        if (xQueueReceive(sd_App_queue, &datos, portMAX_DELAY) == pdPASS)
        // while(uxQueueMessagesWaiting(sd_App_queue) > 0)
        {
            // xQueueReceive(sd_App_queue, &datos, portMAX_DELAY);
            //esp_timer_get_time();
            t_inicio = esp_timer_get_time();
            // check size of regs
            if (cnt++ >= RECORD_SIZE){
                cnt = 0;
                initfile();
            }
            
            if (f_log == NULL) {
                //enviamos mensaje de error
                ESP_LOGE("SD_APP", "Failed to open file for writing\n");
                ESP_LOGE("SD_APP", "File: %s\n", file_log); 
                // No retornar, intentar en la siguiente iteración o esperar a initfile
            }else{
                //"BLDC,PRAW,Presion,Presionfl,FRAW,Flujo,Flujofl,pdata,fl1,fl2\n"
                // sprintf(bufferSd, "%d, %d, %0.6f, %0.6f, %0.6f, %0.6f, %0.6f, %0.6f, %d, %d\n", 
                //         datos.bldc,
                //         datos.praw,
                //         datos.presion,
                //         datos.presionfl,
                //         datos.fraw,
                //         datos.flujo,
                //         datos.flujofl,
                //         datos.pdata,
                //         datos.fl1,
                //         datos.fl2);
                //"              BLDC, Presion, Flujo, smp  , cp\n"   
                // sprintf(bufferSd, "%d, %0.6f,   %0.6f, %0.6f, %0.6f\n", 
                //         datos.bldc,
                //         datos.presionfl,
                //         datos.flujofl,
                //         datos.t_smp,
                //         datos.t_cp);
                
                // sprintf(bufferSd, "Time,BLDC,Presion,Flujo\n");
                sprintf(bufferSd, "%ld, %lld, %d, %0.6f,   %0.6f\n", 
                        cnt,
                        datos.timestamp/1000,    
                        datos.bldc,
                        datos.presionfl,
                        datos.flujofl
                        );        
                
                /**
                 *  uint16_t bldc;
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
                
                // write data to file
                fprintf(f_log, bufferSd);
                
                // close file
                // fclose(f); // Mantenemos el archivo abierto para velocidad
                t_fin = esp_timer_get_time();
                // ESP_LOGI("USD_APP", "I2C cycle time: %lld us", (t_fin - t_inicio));
            }
        }
    }
}

void initfile(void){
    struct tm timeinfo;
    time_t now;

    // Si hay un archivo abierto, cerrarlo antes de crear uno nuevo
    if (f_log != NULL) {
        fclose(f_log);
        f_log = NULL;
    }

    // Get current time from system 
    time(&now);
    localtime_r(&now, &timeinfo);

    /**
     *  file_log[0] = '/';   1
     *  file_log[1] = 's';   2
     *  file_log[2] = 'd';   3
     *  file_log[3] = '/';   4
     *  file_log[4] = '2';   5
     *  file_log[5] = '0';   6
     *  file_log[6] = '2';   7
     *  file_log[7] = '1';   8
     *  file_log[8] = '/';   9
     *  file_log[9] = '0';   10
     *  file_log[10] = '5';  11
     *  file_log[11] = '/';  12
     *  file_log[12] = '0';  13
     *  file_log[13] = '5';  14
     *  file_log[14] = '_';  15
     *  file_log[15] = 'H';  16
     *  file_log[16] = 'H';  17
     *  file_log[17] = ':';  18
     *  file_log[18] = 'M';  19
     *  file_log[19] = 'M';  20
     *  file_log[20] = '.';  21
     *  file_log[21] = 'c';  22
     *  file_log[22] = 's';  23
     *  file_log[23] = 'v';  24
     *  file_log[24] = '\0'; 25
     */
   
    // Create directory structure
    // Create year directory if it doesn't exist
    snprintf(file_log, sizeof(file_log), "/sd/%04d", 
        ((timeinfo.tm_year + 1900)%10000)
        );
        // printf("%s \n",file_log);
    if (mkdir(file_log, 0777) == -1 && errno != EEXIST) {
        ESP_LOGE("SD_APP", "Error creating directory: %s", file_log);
    }
    
    // Create month directory if it doesn't exist
    snprintf(file_log, sizeof(file_log), "/sd/%04d/%02d", 
        ((timeinfo.tm_year + 1900)%10000),
        ((timeinfo.tm_mon%100)+1)
        );
        // printf("%s \n",file_log);
    if (mkdir(file_log, 0777) == -1 && errno != EEXIST) {
        ESP_LOGE("SD_APP", "Error creating directory: %s", file_log);
    }
    
    // Create day directory if it doesn't exist
    snprintf(file_log, sizeof(file_log), "/sd/%04d/%02d/%02d", 
        ((timeinfo.tm_year + 1900)%10000),
        ((timeinfo.tm_mon%100)+1),
        (timeinfo.tm_mday%100));
    //    printf("%s \n",file_log);
    if (mkdir(file_log, 0777) == -1 && errno != EEXIST) {
        ESP_LOGE("SD_APP", "Error creating directory: %s", file_log);
    }
    
    // Create file name with timestamp
    snprintf(file_log, sizeof(file_log), "/sd/%04d/%02d/%02d/%02d%02d.csv", 
        ((timeinfo.tm_year + 1900)%10000),
        ((timeinfo.tm_mon%100)+1),
        (timeinfo.tm_mday%100), 
        (timeinfo.tm_hour%100), 
        (timeinfo.tm_min%100));  
    //printf("%s \n",file_log);
    
        // open file for writing, mode append
    f_log = fopen(file_log, "a");

    if (f_log == NULL) {
        ESP_LOGE("SD_APP", "Failed to open file for writing\n");
        ESP_LOGE("SD_APP", "File: %s\n", file_log); 
        return;
    }else{
        sprintf(bufferSd, "CPAP - GIBIC - UDEA\n");
        fprintf(f_log, bufferSd);
        sprintf(bufferSd, "Hora de inicio: %04d-%02d-%02d %02d:%02d:%02d\n", 
                timeinfo.tm_year + 1900, 
                timeinfo.tm_mon, 
                timeinfo.tm_mday, 
                timeinfo.tm_hour, 
                timeinfo.tm_min,
                timeinfo.tm_sec);
        fprintf(f_log, bufferSd);
        sprintf(bufferSd, "Tasa de muestreo: 50Hz\n");
        fprintf(f_log, bufferSd);

        sprintf(bufferSd, "\n");
        fprintf(f_log, bufferSd);

        // sprintf(bufferSd, "BLDC,PRAW,Presion,Presionfl,FRAW,Flujo,Flujofl,pdata,fl1,fl2\n");
        // sprintf(bufferSd, "BLDC,Presion,Flujo,smp,cp\n");
        sprintf(bufferSd, "Sequence,Time,BLDC,Presion,Flujo\n");
        fprintf(f_log, bufferSd);
        //ESP_LOGI("SD_APP", "File written\n");    
    }
}

void closefile(void){
    if (f_log != NULL) {
        fclose(f_log);
        f_log = NULL;
    }
}
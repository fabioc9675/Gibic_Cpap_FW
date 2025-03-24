#include "uSD/usdApp.h"

//handler de la cola de envio a la sd
QueueHandle_t sd_App_queue = NULL;


void sd_App(void *pvParameters){

    struct tm timeinfo;
    time_t now;
    char dir_year[10];
    char dir_month[14];
    char file_log[22];
    char bufferSd[100];
    
    // Get current time from system 
    time(&now);
    localtime_r(&now, &timeinfo);
    
    //sprintf(dir_year, "/sd/%04d", timeinfo.tm_year + 1900);
    snprintf(dir_year, sizeof(dir_year), "/sd/%04d", ((timeinfo.tm_year + 1900)%10000));
    snprintf(dir_month, sizeof(dir_month), "%s/%02d", dir_year, ((timeinfo.tm_mon + 1)%100));
    snprintf(file_log, sizeof(file_log), "%s/%02d.csv", dir_month, (timeinfo.tm_mday%100));

    // file_log[0] = '/';   1
    // file_log[1] = 's';   2
    // file_log[2] = 'd';   3
    // file_log[3] = '/';   4
    // file_log[4] = '2';   5
    // file_log[5] = '0';   6
    // file_log[6] = '2';   7
    // file_log[7] = '1';   8
    // file_log[8] = '/';   9
    // file_log[9] = '0';   11
    // file_log[10] = '5';  12
    // file_log[11] = '/';  13
    // file_log[12] = '0';  14
    // file_log[13] = '5';  15
    // file_log[14] = '.';  16
    // file_log[15] = 'c';  17
    // file_log[16] = 's';  18
    // file_log[17] = 'v';  19
    
    // Create year directory if it doesn't exist
    if (mkdir(dir_year, 0777) == -1 && errno != EEXIST) {
        ESP_LOGE("SD_APP", "Error creating directory: %s", dir_year);
    }

    // Create month directory if it doesn't exist
    if (mkdir(dir_month, 0777) == -1 && errno != EEXIST) {
        ESP_LOGE("SD_APP", "Error creating directory: %s", dir_month);
    }

    // open file for writing, mode append
    FILE* f = fopen(file_log, "a");

    if (f == NULL) {
        ESP_LOGE("SD_APP", "Failed to open file for writing\n");
        ESP_LOGE("SD_APP", "File: %s\n", file_log); 
        return;
    }else{
        sprintf(bufferSd, "Timestamp; BLDC %%; Presion mmH2O; Flujo\n");
        fprintf(f, bufferSd);
        fclose(f);
        ESP_LOGI("SD_APP", "File written\n");    
    }

    for(;;)
    {
        struct Datos_usd datos;
        while(uxQueueMessagesWaiting(sd_App_queue) > 0){
            xQueueReceive(sd_App_queue, &datos, portMAX_DELAY);
            FILE* f = fopen(file_log, "a");
            if (f == NULL) {
                //enviamos mensaje de error
                ESP_LOGE("SD_APP", "Failed to open file for writing\n");
                ESP_LOGE("SD_APP", "File: %s\n", file_log); 
                return;
            }else{
                //sprintf(bufferSd, "Timestamp; BLDC %%; Presion mmH2O; Flujo\n");
                sprintf(bufferSd, "%ld; %d; %f; %f \n", 
                                datos.timestamp, 
                                datos.bldc,
                                datos.presion,
                                datos.flujo);
                
                // write data to file
                fprintf(f, bufferSd);
                
                // close file
                fclose(f);
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

}
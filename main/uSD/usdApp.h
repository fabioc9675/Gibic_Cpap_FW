#ifndef USD_APP_H
#define USD_APP_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/common.h"

#include "usdDrv.h"


struct Datos_usd{
    int16_t bldc;
    int16_t praw;  // sensor raw presion
    float presion; // sensor presion
    float presionfl; // sensor filtrada presion
    float fraw;    // sensor raw flujo
    float flujo;   // sensor flujo
    float flujofl; // sensor filtrada flujo
    float t_smp;   // tiempo nataly smoth point
    float t_cp;   // tiempo nataly center point
    float pdata;
    uint8_t fl1;
    uint8_t fl2;
};

void sd_App(void *pvParameters);
extern QueueHandle_t sd_App_queue;

#endif // USD_APP_H
#ifndef I2C_APP_H
#define I2C_APP_H

#include "i2c/i2c_drv/i2c_common.h"
#include <math.h>
#include <time.h>


/**
 * global functions
 */
esp_err_t I2C1_init(void);

extern QueueHandle_t i2c_App_queue;


struct Datos_I2c{
    float fraw;
    float presion;
    float flujo;
    int16_t praw;
    float tempFlujo;
};

void i2c_app(void *pvParameters);
void i2c_app_read(void);

typedef enum {
    st_init,
    st_reqAdc0,
    st_rsdp810,
    st_rAdc0,
    st_iddle
} i2c_stetes_t;









#endif // I2C_APP_H
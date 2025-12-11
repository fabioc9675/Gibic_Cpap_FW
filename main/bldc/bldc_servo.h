#ifndef BLDC_SERVO_H
#define BLDC_SERVO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"

extern QueueHandle_t bldc_App_queue;

void bldc_servo_app(void *pvParameters);



#endif // BLDC_SERVO_H